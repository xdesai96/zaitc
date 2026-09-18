#ifndef ZAITC_LAYOUTS
#define ZAITC_LAYOUTS

#include "config.h"
#include "mpd.h"
#include <math.h>
#include <notcurses/notcurses.h>
#include <stdint.h>

typedef struct Image Image;

typedef struct {
  struct ncplane *info_plane;
  struct ncvisual *cover;
  struct ncplane *cover_plane;
  struct ncplane *progbar_plane;
  struct ncprogbar *progbar;
} PlayerView;

typedef struct {
  Mpd *mpd;
  PlayerView view;
} PlayerContext;

void help_callback(struct nctab *t, struct ncplane *ncp, void *curry); // TODO: help_callback
void player_callback(struct nctab *t, struct ncplane *ncp, void *curry);
void cleanup_player(void *data);

#endif // ZAITC_LAYOUTS
