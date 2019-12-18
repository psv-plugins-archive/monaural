#ifndef MONAURAL_H
#define MONAURAL_H

#define MONAURAL_MODE_OFF  0
#define MONAURAL_MODE_MONO 1

#define MONAURAL_CHANNEL_BALANCE_MAX 100

typedef struct {
	int major;
	int mid;
	int minor;
} monaural_version_t;

typedef struct {
	int mode;
	int left_balance;
	int right_balance;
} monaural_config_t;

void MonauralGetVersion(monaural_version_t *version);

void MonauralGetConfig(monaural_config_t *uconfig);
void MonauralSetConfig(monaural_config_t *uconfig);

#endif
