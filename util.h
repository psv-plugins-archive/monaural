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
