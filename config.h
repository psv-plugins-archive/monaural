#ifndef CONFIG_H
#define CONFIG_H

#include "monaural.h"

void reset_config(monaural_config_t *config);

int read_config(monaural_config_t *config);
int write_config(monaural_config_t *config);

#endif
