#include "layouts.h"
#include <math.h>

enum {
  MAX_TEXT_WIDTH = 30,
  IMAGE_PADDING_Y = 2,
  IMAGE_PADDING_X = 3,
  TEXT_PADDING_X = 5,
  PROGBAR_HEIGHT = 3,
  PROGBAR_MARGIN_Y = 2,
};

static double scale(Context *ctx) {
  if (ctx->mpd->song.cover == NULL)
    return 0.0;
  double scale = fmin(
    25.0 / ctx->mpd->song.cover->cols,
    25.0 / ctx->mpd->song.cover->rows
  );
  scale = fmin(scale, 1.0);
  return scale;
}

static uint32_t cover_width(Context *ctx) {
  return ctx->mpd->song.cover->cols * scale(ctx);
}

static uint32_t cover_height(Context *ctx) {
  return ctx->mpd->song.cover->rows * scale(ctx);
}

static uint32_t info_height(Context *ctx) {
  return cover_height(ctx) + IMAGE_PADDING_Y;
}

static uint32_t info_width(Context *ctx) {
  return cover_width(ctx) + TEXT_PADDING_X + MAX_TEXT_WIDTH + IMAGE_PADDING_X;
}

static void close_player_tab(Context *ctx) {
  ncplane_family_destroy(ctx->player.info_plane);
  ctx->player.info_plane = NULL;
  ctx->player.cover_plane = NULL;
  ctx->player.progbar_plane = NULL;
  ctx->player.progbar = NULL;
}

void tab_selector(Context *ctx, uint32_t key_id) {
  TabId next;
  switch (key_id) {
  case '1':
    next = PLAYER;
    break;
  case '2':
    next = HELP;
    break;
  default:
    return;
  }
  if (next == ctx->tab.current)
    return;
  ncplane_erase(nctabbed_content_plane(ctx->tab.tb));
  close_player_tab(ctx);
  ctx->tab.current = next;
  nctabbed_select(ctx->tab.tb, ctx->tab.all[next]);
}

struct ncvisual *create_cover_visual(Image *img) {
  if (img == NULL || img->data == NULL)
    return NULL;
  return ncvisual_from_rgba(
    img->data,
    img->rows,
    img->cols * img->channels,
    img->cols
  );
}

static void update_progbar(Context *ctx) {
  double perc = 0;
  if (ctx->mpd->status.duration > 0) {
    perc = (double) ctx->mpd->status.elapsed / ctx->mpd->status.duration;
    perc = fmin(perc, 1.0);
  }
  ncprogbar_set_progress(ctx->player.progbar, perc);
  struct ncplane *bar = ncprogbar_plane(ctx->player.progbar);
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


static void create_progbar(Context *ctx, int max_y, int max_x) {
  if (ctx->player.progbar_plane != NULL) {
    ncplane_destroy(ctx->player.progbar_plane);
    ctx->player.progbar_plane = NULL;
  }
  ctx->player.progbar_plane = ncplane_create(
    ctx->player.info_plane,
    &(struct ncplane_options){
      .y    = (max_y - 4),
      .x    = 2,
      .rows = 3,
      .cols = max_x - 4
    }
  );
  if (ctx->player.progbar_plane == NULL)
    return;
  ncprogbar_options opts = {
    .ulchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .urchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .blchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .brchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
  };
  ctx->player.progbar = ncprogbar_create(ctx->player.progbar_plane, &opts);
  return;
}

static void print_info(Context *ctx) {
  ncplane_printf_yx(
    ctx->player.info_plane,
    IMAGE_PADDING_Y + 0, cover_width(ctx) + TEXT_PADDING_X,
    "Title: %s", ctx->mpd->song.title ? ctx->mpd->song.title : "Unknown"
  );
  ncplane_printf_yx(
    ctx->player.info_plane,
    IMAGE_PADDING_Y + 1, cover_width(ctx) + TEXT_PADDING_X,
    "Artist: %s", ctx->mpd->song.artist ? ctx->mpd->song.artist : "Unknown"
  );
  ncplane_printf_yx(
    ctx->player.info_plane,
    IMAGE_PADDING_Y + 2, cover_width(ctx) + TEXT_PADDING_X,
    "Volume: %d%%", ctx->mpd->status.volume
  );
}

static void draw_cover(Context *ctx) {
  struct notcurses *nc = ncplane_notcurses(ctx->player.info_plane);
  struct ncplane *blit = ncvisual_blit(nc, ctx->player.ncv, &(struct ncvisual_options){
    .n = ctx->player.cover_plane,
    .blitter = NCBLIT_8x1,
    .scaling = NCSCALE_SCALE
  });
  if (blit == NULL) {
    ncplane_printf(ctx->player.cover_plane, "ncvisual_blit failed");
    return;
  }
}

static void player_init(Context *ctx, struct ncplane *ncp) {
  if (ctx->player.info_plane != NULL)
    return;
  unsigned int max_x, max_y;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  ctx->player.info_plane = ncplane_create(
    ncp,
    &(struct ncplane_options){
      .y = (max_y - info_height(ctx)) / 2,
      .x = (max_x - info_width(ctx)) / 2,
      .rows = info_height(ctx),
      .cols = info_width(ctx),
    }
  );
  ncplane_perimeter_rounded(ctx->player.info_plane, 0, 0, 0);
  if (ctx->player.info_plane == NULL) {
    ncplane_printf(ncp, "Failed to create info plane");
    return;
  }
  ctx->player.cover_plane = ncplane_create(ctx->player.info_plane, &(struct ncplane_options){
    .y = IMAGE_PADDING_Y,
    .x = IMAGE_PADDING_X,
    .rows = cover_height(ctx),
    .cols = cover_width(ctx),
  });
  if (ctx->player.cover_plane == NULL) {
    ncplane_printf(ctx->player.info_plane, "Failed to create cover plane");
    return;
  }
  if (ctx->player.ncv == NULL) {
    ncplane_printf_yx(ncp, 0, 0, "No cover");
    return;
  }
  draw_cover(ctx);
  print_info(ctx);
  ncplane_dim_yx(ctx->player.info_plane, &max_y, &max_x);
  create_progbar(ctx, max_y, max_x);
  update_progbar(ctx);
}

static void player_update(Context *ctx, struct ncplane *ncp) {
  (void)ncp; // WARNING: unused
  if (ctx->player.info_plane == NULL)
    return;
  if (ctx->mpd->status.changed) {
    unsigned int max_x, max_y;
    ncplane_resize_simple(ctx->player.info_plane, info_height(ctx), info_width(ctx));
    ncplane_dim_yx(ncp, &max_y, &max_x);
    ncplane_move_yx(ctx->player.info_plane, (max_y - info_height(ctx)) / 2, (max_x - info_width(ctx)) / 2);
    ncplane_resize_simple(ctx->player.cover_plane, cover_height(ctx), cover_width(ctx));
    ncplane_dim_yx(ctx->player.info_plane, &max_y, &max_x);
    ncplane_resize_simple(ctx->player.progbar_plane, 3, max_x - 4);
    ncplane_move_yx(ctx->player.progbar_plane, max_y - 4, 2);
    ncplane_erase(ctx->player.info_plane);
    ncplane_erase(ctx->player.cover_plane);
    ncplane_perimeter_rounded(ctx->player.info_plane, 0, 0, 0);
    draw_cover(ctx);
  }
  print_info(ctx);
  update_progbar(ctx);
}

void player_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: unused
  Context *ctx = curry;
  if (ctx->player.info_plane == NULL)
    player_init(ctx, ncp);
  else
    player_update(ctx, ncp);
}

void help_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: unused
  (void)curry; // WARNING: unused
  (void)ncp; // WARNING: unused
}
