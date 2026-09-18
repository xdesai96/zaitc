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
    ncplane_printf_yx(parent, 1, 1, "Failed to create plane");
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
  unsigned int max_x = ncplane_dim_x(ncp);
  size_t available = max_x - x - strlen(mpd->song.artist) - 3;
  bool truncated = strlen(mpd->song.title) > available;
  ncplane_printf_yx(
    ncp,
    begy+0, x,
    "%s - %.*s%s",
    mpd->song.artist,
    (int)(truncated ? available - 3 : available), mpd->song.title, truncated ? "..." : ""
  );
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

static void create_progbar(struct ncplane *ncp, struct ncprogbar **progbar) {
  ncprogbar_options opts = {
    .ulchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .urchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .blchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .brchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
  };
  *progbar = ncprogbar_create(ncp, &opts);
}

static void update_progbar(struct ncprogbar *progbar, Mpd *mpd) {
  if (mpd->status.duration > 0) {
    double perc = fmin((double) mpd->status.elapsed / mpd->status.duration, 1.0);
    ncprogbar_set_progress(progbar, perc);
  }
  struct ncplane *bar = ncprogbar_plane(progbar);
  ncplane_perimeter_rounded(bar, 0, 0, 0);
  ncplane_printf_aligned(bar, 0, NCALIGN_LEFT, "%02d:%02d", mpd->status.elapsed / 60, mpd->status.elapsed % 60);
  ncplane_printf_aligned(bar, 0, NCALIGN_RIGHT, "%02d:%02d", mpd->status.duration / 60, mpd->status.duration % 60);
}

static bool init(PlayerContext *ctx, struct ncplane *ncp) {
  if (ctx->view.root.plane != NULL)
    return false;
  unsigned int max_y, max_x;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  // Root
  if(!create_custom_plane(
    ncp,
    &ctx->view.root.plane,
    (max_y - ctx->view.root.height) / 2, (max_x - ctx->view.root.width) / 2,
    ctx->view.root.height, ctx->view.root.width
  ))
    return false;
  ncplane_perimeter_rounded(ctx->view.root.plane, 0, 0, 0);
  // Visual
  ctx->view.cover = create_visual(ctx->mpd->song.cover);
  if (ctx->view.cover == NULL)
    return false;
  // Cover
  if (!create_custom_plane(
    ctx->view.root.plane,
    &ctx->view.cover_plane,
    1, 2,
    (ctx->view.root.height - 4) / 2, ctx->view.root.height - 4
  ))
    return false;
  draw_cover(ctx->view.cover_plane, ctx->view.cover);
  // Details
  ncplane_dim_yx(ctx->view.cover_plane, &max_y, &max_x);
  if(!create_custom_plane(
    ctx->view.root.plane,
    &ctx->view.details_plane,
    1, max_x + 4,
    ctx->view.root.height - 6, ctx->view.root.width - max_x - 6
  ))
    return false;
  // Progbar
  if (!create_custom_plane(
    ctx->view.root.plane,
    &ctx->view.progbar_plane,
    ctx->view.root.height - 4, 2,
    3, ctx->view.root.width - 4
  ))
    return false;
  create_progbar(ctx->view.progbar_plane, &ctx->view.progbar);
  update_progbar(ctx->view.progbar, ctx->mpd);
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
  if (ctx->view.progbar_plane != NULL && ctx->view.progbar != NULL)
    update_progbar(ctx->view.progbar, ctx->mpd);
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
