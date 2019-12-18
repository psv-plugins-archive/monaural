#ifndef SCE_AUDIO_H
#define SCE_AUDIO_H

#define SCE_AUDIO_VOLUME_SHIFT 15
#define SCE_AUDIO_VOLUME_0DB   (1 << SCE_AUDIO_VOLUME_SHIFT)

#define SCE_AUDIO_VOLUME_FLAG_L_CH 0x0001
#define SCE_AUDIO_VOLUME_FLAG_R_CH 0x0002

typedef struct {
// 0x0
	int unk_0; // initial value 0
	int unk_4; // initial value 0, 0 when port is not opened
	int unk_8;
	int mapped_mem_id;
// 0x10
	void *mapped_mem;
	int unk_14;
// 0x18
	unsigned short sample_len;
	char frame_width;
	char unk_1B;
// 0x1C
	unsigned short lvol; // initial value SCE_AUDIO_VOLUME_0DB
	unsigned short rvol; // initial value SCE_AUDIO_VOLUME_0DB
// 0x20
} main_port_t;

typedef struct {
// 0x0
	int unk_0;
	int unk_4;
	int unk_8;
	int unk_C;
// 0x10
	char unk_10;
	char frame_width;
	char alc_mode; // only for bgm port
	char evf;
// 0x14
	char unk_14;
	char is_non_game;
	unsigned short sample_len;
// 0x18
	unsigned short sample_remain;
	unsigned short lvol; // initial value SCE_AUDIO_VOLUME_0DB
	unsigned short rvol; // initial value SCE_AUDIO_VOLUME_0DB
	unsigned short freq;
// 0x20
	short unk_20; // initial value 0x3333333
	short unk_22;
	unsigned int creation_time; // from ksceKernelGetProcessTimeLowCore
// 0x28
} hi_port_t;

typedef struct {
// 0x0
	char unk_0[0x1C];
	main_port_t main_port[9];
	int pad_13C;
// 0x140
	hi_port_t hi_port[2];
// 0x190
} proc_local_stor_t;

#endif
