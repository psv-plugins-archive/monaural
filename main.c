/*
This file is part of Monaural
Copyright 2019 浅倉麗子

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// ＲＩＫＫＡ　ＰＲＯＪＥＣＴ

#include <arm_neon.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <taihen.h>
#include "sce_audio.h"
#include "monaural.h"
#include "config.h"
#include "util.h"

extern int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
extern int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int (*sceAudioOutSetVolume)(int port, int flag, int *vol);

static monaural_config_t config;

#define N_HOOK 3
static SceUID hook_id[N_HOOK];
static tai_hook_ref_t hook_ref[N_HOOK];

#define N_INJECT 3
static SceUID inject_id[N_INJECT];

static char *sce_audio_map_main_name;
static char *sce_audio_map_bgm_name;

void MonauralGetVersion(monaural_version_t *version) {
	monaural_version_t kversion;
	kversion.major = 1;
	kversion.mid = 0;
	kversion.minor = 0;
	ksceKernelMemcpyKernelToUser((uintptr_t)version, &kversion, sizeof(kversion));
}

void MonauralGetConfig(monaural_config_t *uconfig) {
	ksceKernelMemcpyKernelToUser((uintptr_t)uconfig, &config, sizeof(config));
}

void MonauralSetConfig(monaural_config_t *uconfig) {
	ksceKernelMemcpyUserToKernel(&config, (uintptr_t)uconfig, sizeof(config));
	write_config(&config);
}

static int ksceKernelMapUserBlockDefaultType_hook(
		const char *name, int permission, const void *ubuf, unsigned int size,
		void **kpage, unsigned int *ksize, unsigned int *koffset) {

	if (name == sce_audio_map_main_name || name == sce_audio_map_bgm_name) {
		if (config.mode == MONAURAL_MODE_OFF) {
			return TAI_CONTINUE(int, hook_ref[0], name, 1, ubuf, size, kpage, ksize, koffset);
		}

		int ret = TAI_CONTINUE(int, hook_ref[0], name, 2, ubuf, size, kpage, ksize, koffset);

		if (ret >= 0) {
			// permission here is equal to the frame width
			if (permission == 4) {
				int half_stride = 32;
				int stride = half_stride * 2;
				void *base = *kpage + *koffset;
				for (void *pcm = base; pcm < base + size; pcm += stride) {
					__builtin_prefetch(pcm + stride);

					// load with deinterleave
					int16x8x2_t s1 = vld2q_s16(pcm);
					int16x8x2_t s2 = vld2q_s16(pcm + half_stride);

					// average two channels (rounding halving add)
					s1.val[0] = vrhaddq_s16(s1.val[0], s1.val[1]);
					s2.val[0] = vrhaddq_s16(s2.val[0], s2.val[1]);
					s1.val[1] = s1.val[0];
					s2.val[1] = s2.val[0];

					// store with interleave
					vst2q_s16(pcm, s1);
					vst2q_s16(pcm + half_stride, s2);
				}
			}
		}

		return ret;
	} else {
		return TAI_CONTINUE(int, hook_ref[0], name, permission, ubuf, size, kpage, ksize, koffset);
	}
}

static int sceAudioOutOpenPort_hook(int portType, int len, int freq, int param) {
	int port = TAI_CONTINUE(int, hook_ref[1], portType, len, freq, param);
	if (port >= 0) {
		sceAudioOutSetVolume(port,
			SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH,
			(int[]){SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB});
	}
	return port;
}

static int sceAudioOutSetVolume_hook(int port, int flag, int *vol) {
	int kvol[2];

	if (vol) {
		// already is kernel space memory
		if (ksceKernelMemcpyUserToKernel(kvol, (uintptr_t)vol, sizeof(kvol)) < 0) {
			kvol[0] = vol[0];
			kvol[1] = vol[1];
		}

		float bal_max = (float)MONAURAL_CHANNEL_BALANCE_MAX;
		if (flag & SCE_AUDIO_VOLUME_FLAG_L_CH) {
			kvol[0] = (int)((float)config.left_balance / bal_max * (float)kvol[0]);
		}
		if (flag & SCE_AUDIO_VOLUME_FLAG_R_CH) {
			kvol[1] = (int)((float)config.right_balance / bal_max * (float)kvol[1]);
		}

		vol = kvol;
	}

	return TAI_CONTINUE(int, hook_ref[2], port, flag, vol);
}

static void cleanup(void) {
	for (int i = 0; i < N_HOOK; i++) {
		if (hook_id[i] >= 0) { taiHookReleaseForKernel(hook_id[i], hook_ref[i]); }
	}

	for (int i = 0; i < N_INJECT; i++) {
		if (inject_id[i] >= 0) { taiInjectReleaseForKernel(inject_id[i]); }
	}
}

int _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize argc, const void *argv) { (void)argc; (void)argv;

	// find SceAudio module info
	tai_module_info_t minfo;
	minfo.size = sizeof(tai_module_info_t);
	GLZ(taiGetModuleInfoForKernel(KERNEL_PID, "SceAudio", &minfo));

	// get sceAudioOutSetVolume addr
	GLZ(module_get_export_func(KERNEL_PID, "SceAudio", 0x438BB957, 0x64167F11, (uintptr_t*)&sceAudioOutSetVolume));

	// get cstrings "SceAudioUserMapMain" and "SceAudioUserMapBgm"
	GLZ(module_get_offset(KERNEL_PID, minfo.modid, 0, 0x7F10, (uintptr_t*)&sce_audio_map_main_name));
	GLZ(module_get_offset(KERNEL_PID, minfo.modid, 0, 0x7F24, (uintptr_t*)&sce_audio_map_bgm_name));

	// read config
	if (read_config(&config) < 0) { reset_config(&config); }

	// do hooks
	hook_id[0] = taiHookFunctionImportForKernel(KERNEL_PID, hook_ref+0, "SceAudio", 0x6F25E18A, 0x278BC201, ksceKernelMapUserBlockDefaultType_hook);
	hook_id[1] = taiHookFunctionExportForKernel(KERNEL_PID, hook_ref+1, "SceAudio", 0x438BB957, 0x5BC341E4, sceAudioOutOpenPort_hook);
	hook_id[2] = taiHookFunctionExportForKernel(KERNEL_PID, hook_ref+2, "SceAudio", 0x438BB957, 0x64167F11, sceAudioOutSetVolume_hook);
	for (int i = 0; i < N_HOOK; i++) { GLZ(hook_id[i]); }

	// these injects will pass the frame width as the second argument to ksceKernelMapUserBlockDefaultType
	inject_id[0] = taiInjectDataForKernel(KERNEL_PID, minfo.modid, 0, 0x1852, "\x61\x46", 2); // mov r1, r12
	inject_id[1] = taiInjectDataForKernel(KERNEL_PID, minfo.modid, 0, 0x1B9C, "\x31\x00", 2); // mov r1, r6

	// this inject will replace ksceKernelMemcpyUserToKernel with memcpy in sceAudioOutSetVolume
	inject_id[2] = taiInjectDataForKernel(KERNEL_PID, minfo.modid, 0, 0x1D76, "\x03\xf0\x9c\xe9", 4); // blx #13116

	for (int i = 0; i < N_INJECT; i++) { GLZ(inject_id[i]); }

	return SCE_KERNEL_START_SUCCESS;

fail:
	cleanup();
	return SCE_KERNEL_START_FAILED;
}

int module_stop(SceSize argc, const void *argv) { (void)argc; (void)argv;
	cleanup();
	return SCE_KERNEL_STOP_SUCCESS;
}
