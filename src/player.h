#ifndef ZAITC_PLAYER
#define ZAITC_PLAYER

#include "config.h"
#include "mpd.h"
#include "image.h"
#include <math.h>
#include <notcurses/notcurses.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Image Image;

typedef struct {
  struct ncplane *plane;
  int width;
  int height;
} PlayerRoot;

typedef struct {
  PlayerRoot root;
  struct ncplane *details_plane;
  struct ncvisual *cover;
  struct ncplane *cover_plane;
  struct ncplane *progbar_plane;
  struct ncprogbar *progbar;
} PlayerView;

typedef struct {
  Mpd *mpd;
  PlayerView view;
} PlayerContext;

void player_callback(struct nctab *t, struct ncplane *ncp, void *curry);
void player_view_destroy(void *data);
void player_handle_resize(PlayerView *view, struct ncplane *ncp);

#endif // ZAITC_PLAYER

