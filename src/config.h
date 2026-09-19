#ifndef ZAITC_CONFIG
#define ZAITC_CONFIG

#include "../thirdparty/tomlc17.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *host;
  unsigned int port;
  unsigned int timeout;
} MpdConfig;

typedef struct {
  MpdConfig mpd;
} Config;

int config_load(Config *config);
void config_free(Config *config);

#endif // ZAITC_CONFIG
