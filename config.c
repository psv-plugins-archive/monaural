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

#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include "config.h"
#include "monaural.h"
#include "util.h"

void reset_config(monaural_config_t *config) {
	config->mode = MONAURAL_MODE_MONO;
	config->left_balance = MONAURAL_CHANNEL_BALANCE_MAX;
	config->right_balance = MONAURAL_CHANNEL_BALANCE_MAX;
}

int read_config(monaural_config_t *config) {
	SceUID fd = ksceIoOpen(CONFIG_PATH, SCE_O_RDONLY, 0);
	GLZ(fd);

	int ret = ksceIoRead(fd, config, sizeof(*config));
	ksceIoClose(fd);
	GNE(ret, sizeof(*config));

	if (config->mode != MONAURAL_MODE_OFF && config->mode != MONAURAL_MODE_MONO) { goto fail; }
	if (config->left_balance < 0 || MONAURAL_CHANNEL_BALANCE_MAX < config->left_balance) { goto fail; }
	if (config->right_balance < 0 || MONAURAL_CHANNEL_BALANCE_MAX < config->right_balance) { goto fail; }

	return 0;

fail:
	return -1;
}

int write_config(monaural_config_t *config) {
	ksceIoMkdir(CONFIG_BASE_DIR, 0777);
	ksceIoMkdir(CONFIG_MONAURAL_DIR, 0777);
	SceUID fd = ksceIoOpen(CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	GLZ(fd);

	int ret = ksceIoWrite(fd, config, sizeof(*config));
	ksceIoClose(fd);
	GNE(ret, sizeof(*config));

	return 0;

fail:
	return -1;
}
