#include "player.h"
#include "image.h"
#include <notcurses/notcurses.h>

static struct ncvisual *create_visual(Image *img) {
  if (img == NULL || img->data == NULL)
    return NULL;
  return ncvisual_from_rgba(
    img->data,
    img->rows,
    img->cols * img->channels,
    img->cols
  );
}

static bool create_custom_plane(struct ncplane *parent, struct ncplane **ncp, int y, int x, int rows, int cols) {
  if (*ncp != NULL) {
    return true;
  }
  *ncp = ncplane_create(
    parent,
    &(struct ncplane_options){
      .y = y,
      .x = x,
      .rows = rows,
      .cols = cols,
    }
  );
  if (*ncp == NULL) {
    ncplane_printf_yx(parent, 1, 1, "Failed to create info plane");
    return false;
  }
  return true;
}

static void draw_cover(struct ncplane *ncp, struct ncvisual *ncv) {
  struct notcurses *nc = ncplane_notcurses(ncp);
  struct ncplane *blit = ncvisual_blit(nc, ncv, &(struct ncvisual_options){
    .n = ncp,
    .blitter = NCBLIT_8x1,
    .scaling = NCSCALE_SCALE,
  });
  if (blit == NULL)
    ncplane_printf(ncp, "Couldn't draw the image");
}

static void print_details(struct ncplane *ncp, Mpd *mpd, int begy, int x) {
  ncplane_printf_yx(ncp, begy+0, x, "%s - %s", mpd->song.artist, mpd->song.title);
  ncplane_printf_yx(ncp, begy+1, x, "Random: %s", mpd->status.random ? "on" : "off");
  ncplane_printf_yx(ncp, begy+2, x, "Repeat: %s", mpd->status.repeat ? "on" : "off");
  ncplane_printf_yx(ncp, begy+3, x, "Single: %s", mpd->status.single ? "on" : "off");
  const char *state;
  switch (mpd->status.state) {
    case MPD_STATE_PLAY:  state = "Playing"; break;
    case MPD_STATE_STOP:  state = "Stopped"; break;
    case MPD_STATE_PAUSE: state = "Paused";  break;
    default:              state = "Unkown";  break;
  }
  ncplane_printf_yx(ncp, begy+4, 1, "State: %s", state);
}

static bool init(PlayerContext *ctx, struct ncplane *ncp) {
  if (ctx->view.root.plane != NULL)
    return false;
  unsigned int max_y, max_x;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  if(!create_custom_plane(
    ncp,
    &ctx->view.root.plane,
    (max_y - ctx->view.root.height) / 2, (max_x - ctx->view.root.width) / 2,
    ctx->view.root.height, ctx->view.root.width
  ))
    return false;
  ncplane_perimeter_rounded(ctx->view.root.plane, 0, 0, 0);

  ctx->view.cover = create_visual(ctx->mpd->song.cover);
  if (ctx->view.cover == NULL)
    return false;

  if (!create_custom_plane(
    ctx->view.root.plane,
    &ctx->view.cover_plane,
    1, 2,
    (ctx->view.root.height - 4) / 2, ctx->view.root.height - 4
  ))
    return false;

  draw_cover(ctx->view.cover_plane, ctx->view.cover);
  ncplane_dim_yx(ctx->view.cover_plane, &max_y, &max_x);
  if(!create_custom_plane(
    ctx->view.root.plane,
    &ctx->view.details_plane,
    1, max_x + 4,
    ctx->view.root.height - 6, ctx->view.root.width - max_x - 6
  ))
    return false;
  return true;
}

static void player_update(PlayerContext *ctx) {
  if (ctx->mpd->status.state == MPD_STATE_UNKNOWN) {
    player_view_destroy(&ctx->view);
    return;
  }
  if (ctx->mpd->status.changed) {
    if (ctx->view.cover_plane != NULL && ctx->view.cover) {
      ncplane_erase(ctx->view.cover_plane);
      ncvisual_destroy(ctx->view.cover);
      ctx->view.cover = create_visual(ctx->mpd->song.cover);
      draw_cover(ctx->view.cover_plane, ctx->view.cover);
    }
  }
  if (ctx->view.details_plane != NULL) {
    ncplane_erase(ctx->view.details_plane);
    print_details(ctx->view.details_plane, ctx->mpd, 1, 1);
  }
  return;
}

void player_view_destroy(void *data) {
  PlayerView *view = data;
  ncplane_family_destroy(view->root.plane);
  view->root.plane = NULL;
  view->details_plane = NULL;
  view->cover_plane = NULL;
  view->progbar_plane = NULL;
  view->progbar = NULL;
  view->cover = NULL;
}

void player_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t;
  PlayerContext *ctx = curry;
  if (!init(ctx, ncp))
    player_update(ctx);
}
