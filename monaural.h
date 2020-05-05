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
