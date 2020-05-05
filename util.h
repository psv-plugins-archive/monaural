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

#ifndef UTIL_H
#define UTIL_H

#define CONFIG_BASE_DIR "ur0:data/"
#define CONFIG_MONAURAL_DIR CONFIG_BASE_DIR "monaural/"
#define CONFIG_PATH CONFIG_MONAURAL_DIR "config.bin"

#define GLZ(x) do {\
	if ((x) < 0) { goto fail; }\
} while (0)

#define GNE(x, k) do {\
	if ((x) != (k)) { goto fail; }\
} while(0)

#endif
