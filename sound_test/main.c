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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <psp2/audioout.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include "debug_screen/debugScreen.h"
#include "../monaural.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))

#define FREQ 48000
#define N_SAMPLE 55986
#define N_SAMPLE_ALIGNED ALIGN(N_SAMPLE, 64)
#define M_AUDIO_LEN (N_SAMPLE * 2)
#define S_AUDIO_LEN (N_SAMPLE_ALIGNED * 4)

#define UI_BUF_LEN 0x1000
#define X_MARGIN 20
#define Y_MARGIN 20
#define CLEAR_COLOUR 0xFF
#define BG_COLOUR 0xFFFFFF
#define FG_COLOUR 0x404040

typedef struct {
	int row;
	monaural_version_t version;
	monaural_config_t config;
} ui_state;

static void ui_init(void) {
	psvDebugScreenInit();
	PsvDebugScreenFont *font = psvDebugScreenGetFont();
	font = psvDebugScreenScaleFont2x(font);
	font = psvDebugScreenSetFont(font);
	psvDebugScreenSetBgColor(BG_COLOUR);
	psvDebugScreenSetFgColor(FG_COLOUR);
}

static void ui_reset(void) {
	psvDebugScreenBlank(0xFF);
	psvDebugScreenSetCoordsXY((int[]){X_MARGIN}, (int[]){Y_MARGIN});
}

static void ui_flush(void) {
	psvDebugScreenSwapFb();
}

__attribute__((__format__ (__printf__, 1, 2)))
static void ui_line(char *format, ...) {
	int y;
	psvDebugScreenGetCoordsXY((int[]){0}, &y);
	psvDebugScreenSetCoordsXY((int[]){X_MARGIN}, &y);

	char buf[UI_BUF_LEN];
	va_list opt;
	va_start(opt, format);
	vsnprintf(buf, sizeof(buf), format, opt);
	psvDebugScreenPuts(buf);
	va_end(opt);
}

static char ui_cursor(ui_state *state, int idx) {
	return state->row == idx ? '>' : ' ';
}

static void ui_render(ui_state *state) {
	ui_reset();

	monaural_version_t *ver = &state->version;
	ui_line("Monaural v%d.%d.%d by Asakura Reiko\n", ver->major, ver->mid, ver->minor);
	ui_line("\n");
	ui_line("UP/DOWN       Select item\n");
	ui_line("LEFT/RIGHT    Set option\n");
	ui_line("LT/RT         Channel balance +/-10\n");
	ui_line("CIRCLE        Play test sound\n");
	ui_line("CROSS         Exit application\n");
	ui_line("\n");

	if (state->config.mode == MONAURAL_MODE_MONO) {
		ui_line("%c Mono       [ON]    OFF\n", ui_cursor(state, 0));
	} else {
		ui_line("%c Mono        ON    [OFF]\n", ui_cursor(state, 0));
	}

	ui_line("%c L balance  %3d  /  %03d\n", ui_cursor(state, 1), state->config.left_balance, MONAURAL_CHANNEL_BALANCE_MAX);
	ui_line("%c R balance  %3d  /  %03d\n", ui_cursor(state, 2), state->config.right_balance, MONAURAL_CHANNEL_BALANCE_MAX);

	ui_flush();
}

static void play_test_sound(void *mem_base) {
	int port = sceAudioOutOpenPort(
		SCE_AUDIO_OUT_PORT_TYPE_MAIN,
		N_SAMPLE_ALIGNED,
		FREQ,
		SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
	if (port < 0) { goto done; }

	SceUID fd = sceIoOpen("app0:ps.pcm", SCE_O_RDONLY, 0);
	if (fd < 0) { goto release_port; }

	short *pcm = mem_base;
	if (sceIoRead(fd, pcm, M_AUDIO_LEN) == M_AUDIO_LEN) {
		int *pcm2 = mem_base + M_AUDIO_LEN;

		// left channel
		memset(pcm2, 0, S_AUDIO_LEN);
		for (int i = 0; i < N_SAMPLE; i++) {
			pcm2[i] = pcm[i];
		}
		sceAudioOutOutput(port, pcm2);
		sceAudioOutOutput(port, NULL);

		// right channel
		memset(pcm2, 0, S_AUDIO_LEN);
		for (int i = 0; i < N_SAMPLE; i++) {
			pcm2[i] = pcm[i];
			pcm2[i] <<= 16;
		}
		sceAudioOutOutput(port, pcm2);
		sceAudioOutOutput(port, NULL);

		// both channels
		memset(pcm2, 0, S_AUDIO_LEN);
		for (int i = 0; i < N_SAMPLE; i++) {
			pcm2[i] = pcm[i];
			pcm2[i] <<= 16;
			pcm2[i] += pcm[i];
		}
		sceAudioOutOutput(port, pcm2);
		sceAudioOutOutput(port, NULL);
	}
	sceIoClose(fd);

release_port:
	sceAudioOutReleasePort(port);
done:
	return;
}

static void adjust_balance(ui_state *state, int left, int right) {
	state->config.left_balance =
		MIN(MONAURAL_CHANNEL_BALANCE_MAX, MAX(0, state->config.left_balance + left));
	state->config.right_balance =
		MIN(MONAURAL_CHANNEL_BALANCE_MAX, MAX(0, state->config.right_balance + right));
	MonauralSetConfig(&state->config);
}

int main() {
	// allocate audio memory
	SceUID mem_id = sceKernelAllocMemBlock(
		"MonauralMemBlock",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
		ALIGN(M_AUDIO_LEN + S_AUDIO_LEN, 4096),
		NULL);
	if (mem_id < 0) { goto done; }
	void *mem_base;
	if (sceKernelGetMemBlockBase(mem_id, &mem_base) < 0) { goto free_mem; }

	// setup ui state
	ui_init();
	ui_state state;
	state.row = 0;
	MonauralGetVersion(&state.version);

	SceCtrlData last_ctrl;
	memset(&last_ctrl, 0, sizeof(last_ctrl));

	for (;;) {
		sceDisplayWaitVblankStartMulti(3);

		MonauralGetConfig(&state.config);

		SceCtrlData ctrl;
		if (sceCtrlReadBufferPositive(0, &ctrl, 1) <= 0) { continue; }
		int btns = ~last_ctrl.buttons & ctrl.buttons;

		if (btns & SCE_CTRL_UP) {
			state.row = (state.row - 1 + 3) % 3;

		} else if (btns & SCE_CTRL_DOWN) {
			state.row = (state.row + 1) % 3;

		} else if (btns & SCE_CTRL_LEFT) {
			if (state.row == 0) {
				state.config.mode = ~state.config.mode & 1;
				MonauralSetConfig(&state.config);
			} else if (state.row == 1) {
				adjust_balance(&state, -1, 0);
			} else if (state.row == 2) {
				adjust_balance(&state, 0, -1);
			}

		} else if (btns & SCE_CTRL_RIGHT) {
			if (state.row == 0) {
				state.config.mode = ~state.config.mode & 1;
				MonauralSetConfig(&state.config);
			} else if (state.row == 1) {
				adjust_balance(&state, 1, 0);
			} else if (state.row == 2) {
				adjust_balance(&state, 0, 1);
			}

		} else if (btns & SCE_CTRL_LTRIGGER) {
			if (state.row == 1) {
				adjust_balance(&state, -10, 0);
			} else if (state.row == 2) {
				adjust_balance(&state, 0, -10);
			}

		} else if (btns & SCE_CTRL_RTRIGGER) {
			if (state.row == 1) {
				adjust_balance(&state, 10, 0);
			} else if (state.row == 2) {
				adjust_balance(&state, 0, 10);
			}

		} else if (btns & SCE_CTRL_CIRCLE) {
			play_test_sound(mem_base);

		} else if (btns & SCE_CTRL_CROSS) {
			break;
		}
		last_ctrl = ctrl;

		ui_render(&state);
	}

free_mem:
	sceKernelFreeMemBlock(mem_id);
done:
	return sceKernelExitProcess(0);
}
