#include "layouts.h"

enum {
  MAX_TEXT_WIDTH = 30,
  IMAGE_PADDING_Y = 2,
  IMAGE_PADDING_X = 3,
  TEXT_PADDING_X = 5,
  PROGBAR_HEIGHT = 3,
  PROGBAR_MARGIN_Y = 2,
};

static double scale(PlayerContext *ctx) {
  if (ctx->mpd->song.cover == NULL)
    return 0.0;
  double scale = fmin(
    25.0 / ctx->mpd->song.cover->cols,
    25.0 / ctx->mpd->song.cover->rows
  );
  scale = fmin(scale, 1.0);
  return scale;
}

static uint32_t cover_width(PlayerContext *ctx) {
  return ctx->mpd->song.cover->cols * scale(ctx);
}

static uint32_t cover_height(PlayerContext *ctx) {
  return ctx->mpd->song.cover->rows * scale(ctx);
}

static uint32_t info_height(PlayerContext *ctx) {
  return cover_height(ctx) + IMAGE_PADDING_Y;
}

static uint32_t info_width(PlayerContext *ctx) {
  return cover_width(ctx) + TEXT_PADDING_X + MAX_TEXT_WIDTH + IMAGE_PADDING_X;
}

void cleanup_player(void *data) {
  PlayerContext *ctx = data;
  ncplane_family_destroy(ctx->view.info_plane);
  ctx->view.info_plane = NULL;
  ctx->view.cover_plane = NULL;
  ctx->view.progbar_plane = NULL;
  ctx->view.progbar = NULL;
}

static struct ncvisual *create_cover_visual(Image *img) {
  if (img == NULL || img->data == NULL)
    return NULL;
  return ncvisual_from_rgba(
    img->data,
    img->rows,
    img->cols * img->channels,
    img->cols
  );
}

static void update_progbar(PlayerContext *ctx) {
  if (ctx->mpd->status.duration > 0) {
    double perc = fmin((double) ctx->mpd->status.elapsed / ctx->mpd->status.duration, 1.0);
    ncprogbar_set_progress(ctx->view.progbar, perc);
  }
  struct ncplane *bar = ncprogbar_plane(ctx->view.progbar);
  ncplane_perimeter_rounded(bar, 0, 0, 0);
  ncplane_printf_aligned(
    bar, 0, NCALIGN_LEFT,
    "%02d:%02d", ctx->mpd->status.elapsed / 60, ctx->mpd->status.elapsed % 60
  );
  ncplane_printf_aligned(
    bar, 0, NCALIGN_RIGHT,
    "%02d:%02d", ctx->mpd->status.duration / 60, ctx->mpd->status.duration % 60
  );
}

static void create_progbar(PlayerContext *ctx) {
  if (ctx->view.progbar_plane != NULL) {
    ncplane_destroy(ctx->view.progbar_plane);
    ctx->view.progbar_plane = NULL;
  }
  ctx->view.progbar_plane = ncplane_create(
    ctx->view.info_plane,
    &(struct ncplane_options){
      .y    = (info_height(ctx) - 4),
      .x    = 2,
      .rows = 3,
      .cols = info_width(ctx) - 4
    }
  );
  if (ctx->view.progbar_plane == NULL)
    return;
  ncprogbar_options opts = {
    .ulchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .urchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .blchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .brchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
  };
  ctx->view.progbar = ncprogbar_create(ctx->view.progbar_plane, &opts);
  return;
}

static void print_info(PlayerContext *ctx) {
  ncplane_printf_yx(
    ctx->view.info_plane,
    IMAGE_PADDING_Y + 0, cover_width(ctx) + TEXT_PADDING_X,
    "Title: %s", ctx->mpd->song.title ? ctx->mpd->song.title : "Unknown"
  );
  ncplane_printf_yx(
    ctx->view.info_plane,
    IMAGE_PADDING_Y + 1, cover_width(ctx) + TEXT_PADDING_X,
    "Artist: %s", ctx->mpd->song.artist ? ctx->mpd->song.artist : "Unknown"
  );
  ncplane_printf_yx(
    ctx->view.info_plane,
    IMAGE_PADDING_Y + 2, cover_width(ctx) + TEXT_PADDING_X,
    "Volume: %d%%", ctx->mpd->status.volume
  );
  // TODO: Show states for "repeat" "single" and so on
}

static void draw_cover(PlayerContext *ctx) {
  if (ctx->view.cover) {
    struct notcurses *nc = ncplane_notcurses(ctx->view.info_plane);
    struct ncplane *blit = ncvisual_blit(nc, ctx->view.cover, &(struct ncvisual_options){
      .n = ctx->view.cover_plane,
      .blitter = NCBLIT_8x1,
      .scaling = NCSCALE_SCALE
    });
    if (blit == NULL) {
      ncplane_printf(ctx->view.cover_plane, "ncvisual_blit failed");
      return;
    }
  }
}

static void player_init(PlayerContext *ctx, struct ncplane *ncp) {
  if (ctx->view.info_plane != NULL)
    return;
  if (ctx->view.cover == NULL)
    ctx->view.cover = create_cover_visual(ctx->mpd->song.cover);
  unsigned int max_x, max_y;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  ctx->view.info_plane = ncplane_create(
    ncp,
    &(struct ncplane_options){
      .y = (max_y - info_height(ctx)) / 2,
      .x = (max_x - info_width(ctx)) / 2,
      .rows = info_height(ctx),
      .cols = info_width(ctx),
    }
  );
  if (ctx->view.info_plane == NULL) {
    ncplane_printf(ncp, "Failed to create info plane");
    return;
  }
  print_info(ctx);
  ncplane_perimeter_rounded(ctx->view.info_plane, 0, 0, 0);
  ctx->view.cover_plane = ncplane_create(ctx->view.info_plane, &(struct ncplane_options){
    .y = IMAGE_PADDING_Y,
    .x = IMAGE_PADDING_X,
    .rows = cover_height(ctx),
    .cols = cover_width(ctx),
  });
  if (ctx->view.cover_plane == NULL) {
    ncplane_printf(ctx->view.info_plane, "Failed to create cover plane");
    return;
  }
  if (ctx->view.cover == NULL) {
    ncplane_printf_yx(ctx->view.info_plane, 0, 0, "No cover");
    return;
  }
  draw_cover(ctx);
  create_progbar(ctx);
  update_progbar(ctx);
}

static void player_update(PlayerContext *ctx, struct ncplane *ncp) {
  if (ctx->view.info_plane == NULL)
    return;
  if (ctx->mpd->status.changed) {
    unsigned int max_x, max_y;
    ncplane_resize_simple(ctx->view.info_plane, info_height(ctx), info_width(ctx));
    ncplane_dim_yx(ncp, &max_y, &max_x);
    ncplane_move_yx(ctx->view.info_plane, (max_y - info_height(ctx)) / 2, (max_x - info_width(ctx)) / 2);
    ncplane_resize_simple(ctx->view.cover_plane, cover_height(ctx), cover_width(ctx));
    ncplane_dim_yx(ctx->view.info_plane, &max_y, &max_x);
    ncplane_resize_simple(ctx->view.progbar_plane, 3, max_x - 4);
    ncplane_move_yx(ctx->view.progbar_plane, max_y - 4, 2);
    ncplane_erase(ctx->view.info_plane);
    ncplane_erase(ctx->view.cover_plane);
    ncplane_perimeter_rounded(ctx->view.info_plane, 0, 0, 0);
    if (ctx->view.cover != NULL) {
      ncvisual_destroy(ctx->view.cover);
      ctx->view.cover = create_cover_visual(ctx->mpd->song.cover);
      draw_cover(ctx);
    }
  }
  // TODO: Fix bug with "re-printing" details
  // Now it does not clear the screen before re-printing the info.
  // E.g if you got 100% volume and then decrease it it shows the "%" char twice.
  print_info(ctx);
  update_progbar(ctx);
}

void player_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: unused
  PlayerContext *ctx = curry;
  if (ctx->view.info_plane == NULL)
    player_init(ctx, ncp);
  else
    player_update(ctx, ncp);
}

void help_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: unused
  (void)curry; // WARNING: unused
  (void)ncp; // WARNING: unused
}
