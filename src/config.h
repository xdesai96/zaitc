#ifndef ZAITC_CONFIG
#define ZAITC_CONFIG

#include <stdlib.h>

typedef struct {
  char *host;
  unsigned int port;
  unsigned int timeout;
} MpdConfig;

typedef struct {
  MpdConfig mpd;
  unsigned int target_fps;
} Config;

#endif // ZAITC_CONFIG
