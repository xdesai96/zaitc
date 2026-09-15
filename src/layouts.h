#ifndef ZAITC_LAYOUTS
#define ZAITC_LAYOUTS

#include "config.h"
#include "mpd.h"
#include <notcurses/notcurses.h>
#include <stdint.h>

typedef struct Image Image;

typedef struct {
  struct ncprogbar *progbar;
  struct ncvisual *ncv;
} PlayerLayout;

typedef enum {
  HELP,
  PLAYER,
  COUNT,
} TabId;

typedef struct {
  TabId current;
  struct ncplane *tab_root;
  struct nctab *all[COUNT];
  struct nctabbed *tb;
} Tabs;

typedef struct {
  MpdContext *mpd;
  PlayerLayout player;

  Tabs tabs;
} Context;

struct ncvisual *create_cover_visual(Image *img);
void handle_resize(Context *ctx, struct ncplane *content, struct ncplane *stdplane, unsigned int *curr_y, unsigned int *curr_x);
void help_callback(struct nctab *t, struct ncplane *ncp, void *curry); // TODO: help_callback
void player_callback(struct nctab *t, struct ncplane *ncp, void *curry);
void tab_root_clear(Context *ctx);
void tab_selector(Context *ctx, uint32_t key_id);

#endif // ZAITC_LAYOUTS
