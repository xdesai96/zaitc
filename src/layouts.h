#ifndef ZAITC_LAYOUTS
#define ZAITC_LAYOUTS

#include "config.h"
#include "mpd.h"
#include <notcurses/notcurses.h>
#include <stdint.h>

typedef struct Image Image;

typedef struct {
  struct ncvisual *ncv;
  struct ncplane *info_plane;
  struct ncplane *cover_plane;
  struct ncplane *progbar_plane;
  struct ncprogbar *progbar;
} Player;

typedef enum {
  HELP,
  PLAYER,
  COUNT,
} TabId;

typedef struct {
  TabId current;
  struct nctab *all[COUNT];
  struct nctabbed *tb;
} Tab;

typedef struct {
  MpdContext *mpd;
  Player player;
  Tab tab;
} Context;

struct ncvisual *create_cover_visual(Image *img);
void help_callback(struct nctab *t, struct ncplane *ncp, void *curry); // TODO: help_callback
void player_callback(struct nctab *t, struct ncplane *ncp, void *curry);
void tab_selector(Context *ctx, uint32_t key_id);

#endif // ZAITC_LAYOUTS
