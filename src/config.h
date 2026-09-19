#ifndef ZAITC_CONFIG
#define ZAITC_CONFIG

#include "../thirdparty/tomlc17.h"
#include <stdlib.h>
#include <string.h>

// MPD
typedef struct {
  char *host;
  unsigned int port;
  unsigned int timeout;
} MpdConfig;

// PLAYER TAB
typedef struct {
  size_t plane_width;
  size_t plane_height;
} PlayerRootPlane;

typedef struct {
  PlayerRootPlane plane;
} PlayerConfig;

// TABS
typedef struct {
  PlayerConfig player;
} TabsConfig;

// CONFIG
typedef struct {
  MpdConfig mpd;
  TabsConfig tabs;
} Config;

int config_load(Config *config);
void config_free(Config *config);

#endif // ZAITC_CONFIG
