#include "layouts.h"
#include <math.h>

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

struct ncprogbar *create_progbar(struct ncplane *music_info, int max_y, int max_x, Context *context) {
  struct ncplane *progbar_plane = ncplane_create(
    music_info,
    &(struct ncplane_options){
      .y    = (max_y - 4),
      .x    = 2,
      .rows = 3,
      .cols = max_x - 4
    }
  );
  if (progbar_plane == NULL)
    return NULL;
  ncprogbar_options opts = {
    .ulchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .urchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .blchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
    .brchannel = NCCHANNEL_INITIALIZER(255, 255, 255),
  };
  struct ncprogbar *progress = ncprogbar_create(progbar_plane, &opts);
  if (progress == NULL)
    return NULL;
  double perc = 0;
  if (context->mpd->status.duration > 0) {
    perc = (double)context->mpd->status.elapsed / context->mpd->status.duration;
    if (perc > 1.0)
      perc = 1.0;
  }
  ncprogbar_set_progress(progress, perc);
  struct ncplane *bar = ncprogbar_plane(progress);
  ncplane_perimeter_rounded(bar, 0, 0, 0);
  ncplane_printf_aligned(
    bar, 0, NCALIGN_LEFT, 
    "%02d:%02d", context->mpd->status.elapsed / 60, context->mpd->status.elapsed % 60
  );
  ncplane_printf_aligned(
    bar, 0, NCALIGN_RIGHT, 
    "%02d:%02d", context->mpd->status.duration / 60, context->mpd->status.duration % 60
  );
  return progress;
}

static void tab_root_destroy(Context *ctx) {
  if (ctx->player.progbar != NULL) {
    ncprogbar_destroy(ctx->player.progbar);
    ctx->player.progbar = NULL;
  }
  if (ctx->tabs.tab_root != NULL) {
    ncplane_family_destroy(ctx->tabs.tab_root);
    ctx->tabs.tab_root = NULL;
  }
}

static struct ncplane *tab_root_create(Context *ctx, struct ncplane *parent, const struct ncplane_options *opts) {
  if (ctx->tabs.tab_root != NULL)
    tab_root_destroy(ctx);
  ctx->tabs.tab_root = ncplane_create(parent, opts);
  return ctx->tabs.tab_root;
}

void tab_root_clear(Context *ctx) {
  if (ctx->tabs.tab_root != NULL)
    ncplane_erase(ctx->tabs.tab_root);
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
  if (next == ctx->tabs.current)
    return;
  tab_root_destroy(ctx);
  ctx->tabs.current = next;
  nctabbed_select(ctx->tabs.tb, ctx->tabs.all[next]);
}

void player_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: tab is unused
  Context *context = curry;
  PlayerLayout *player = &context->player;
  unsigned int max_x, max_y;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  struct ncplane *root = tab_root_create(
    context,
    ncp,
    &(struct ncplane_options){
      .y = 0, .x = 0,
      .rows = max_y, .cols = max_x
    }
  );
  ncplane_dim_yx(root, &max_y, &max_x);
  size_t img_w = context->mpd->song.cover->cols;
  size_t img_h = context->mpd->song.cover->rows;
  double scale = fmin(
    25.0 / img_w,
    25.0 / img_h
  );
  if (scale > 1.0)
    scale = 1.0;
  unsigned int cover_width = img_w * scale;
  unsigned int cover_height = img_h * scale;
  unsigned int text_width = 30;
  unsigned int margin_y = 2;
  unsigned int margin_x = 3;
  unsigned int gap = margin_x + 2;
  unsigned int info_width = cover_width + gap + text_width + margin_x;
  unsigned int info_height = cover_height + margin_x;
  if (max_x < info_width || max_y < info_height) {
    ncplane_printf_yx(root, 0, 0, "Terminal too small");
    return;
  }
  struct ncplane *music_info = ncplane_create(
    root,
    &(struct ncplane_options){
      .y = (max_y - info_height) / 2,
      .x = (max_x - info_width) / 2,
      .rows = info_height,
      .cols = info_width,
    }
  );
  if (music_info == NULL) {
    ncplane_printf_yx(root, 0, 0, "Failed to create music_info");
    return;
  }
  ncplane_perimeter_rounded(music_info, 0, 0, 0);
  struct ncplane_options coveropts = {
    .y = margin_y,
    .x = margin_x,
    .rows = cover_height,
    .cols = cover_width,
  };
  struct ncplane *cover_plane = ncplane_create(music_info, &coveropts);
  if (cover_plane == NULL) {
    ncplane_printf(music_info, "Failed to create cover plane");
    return;
  }
  struct notcurses *nc = ncplane_notcurses(music_info);
  ncplane_dim_yx(music_info, &max_y, &max_x);
  struct ncvisual_options ncvopts = {
    .n = cover_plane,
    .blitter = NCBLIT_8x1,
    .scaling = NCSCALE_SCALE
  };
  if (player->ncv == NULL) {
    ncplane_printf_yx(root, 0, 0, "No cover");
    return;
  }
  struct ncplane*rc = ncvisual_blit(nc, player->ncv, &ncvopts);
  if (rc == NULL) {
    ncplane_printf(music_info, "ncvisual_blit failed: %p\n", rc);
    ncplane_printf(root, "player->ncv: %p\n", player->ncv);
    return;
  }
  ncplane_printf_yx(
    music_info,
    coveropts.y + 0, cover_width + gap,
    "Title: %s", context->mpd->song.title ? context->mpd->song.title : "Unknown"
  );
  ncplane_printf_yx(
    music_info, coveropts.y + 1, cover_width + gap, 
    "Artist: %s", context->mpd->song.artist ? context->mpd->song.artist : "Unknown"
  );
  ncplane_printf_yx(
    music_info, coveropts.y + 2, cover_width + gap, 
    "Volume: %d%%", context->mpd->status.volume ? context->mpd->status.volume : 999
  );
  if (player->progbar != NULL) {
    ncprogbar_destroy(player->progbar);
    player->progbar = NULL;
  }
  player->progbar = create_progbar(music_info, max_y, max_x, context);
}

void help_callback(struct nctab *t, struct ncplane *ncp, void *curry) {
  (void)t; // WARNING: tab is unused
  Context *context = curry;
  unsigned int max_x, max_y;
  ncplane_dim_yx(ncp, &max_y, &max_x);
  struct ncplane_options rootopts = {.y = 0, .x = 0, .rows = max_y, .cols = max_x};
  struct ncplane *root = tab_root_create(context, ncp, &rootopts);
  ncplane_printf_yx(root, 1, 1, "HELP");
}
