#include "mpd.h"
#include <mpd/player.h>
#include <mpd/status.h>

static void update_status(Mpd *ctx) {
  struct mpd_status *status = mpd_run_status(ctx->conn);
  if (!status)
    return;
  struct mpd_song *current_song = mpd_run_current_song(ctx->conn);
  if (current_song == NULL) {
    mpd_status_free(status);
    return;
  }
  const char *title    = mpd_song_get_tag(current_song, MPD_TAG_TITLE, 0);
  const char *artist   = mpd_song_get_tag(current_song, MPD_TAG_ARTIST, 0);
  ctx->status.state    = mpd_status_get_state(status);
  ctx->status.repeat   = mpd_status_get_repeat(status);
  ctx->status.volume   = fmax(mpd_status_get_volume(status), 0);
  ctx->status.single   = mpd_status_get_single_state(status);
  ctx->status.consume  = mpd_status_get_consume_state(status);
  ctx->status.random   = mpd_status_get_random(status);
  ctx->status.duration = mpd_status_get_total_time(status);
  ctx->status.elapsed  = mpd_status_get_elapsed_ms(status) / 1000;
  const char *new_uri  = mpd_song_get_uri(current_song);
  bool changed = ctx->song.uri == NULL || new_uri == NULL || strcmp(ctx->song.uri, new_uri) != 0;
  free(ctx->song.uri);
  free(ctx->song.title);
  free(ctx->song.artist);
  ctx->song.uri        = new_uri ? strdup(new_uri) : NULL;
  ctx->song.title      = strdup(title ? title : "Unknown");
  ctx->song.artist     = strdup(artist ? artist : "Unkown");
  ctx->status.changed  = changed;
  mpd_status_free(status);
  mpd_song_free(current_song);
}

int mpd_connect(Mpd *ctx) {
  ctx->conn = mpd_connection_new(
    ctx->config.host,
    ctx->config.port,
    ctx->config.timeout
  );
  if (ctx->conn == NULL) {
    return 0;
  }
  if (mpd_connection_get_error(ctx->conn) != MPD_ERROR_SUCCESS) {
    mpd_disconnect(ctx);
    return 0;
  }
  return 1;
}

void mpd_disconnect(Mpd *ctx) {
  if (ctx->conn != NULL) {
    mpd_connection_free(ctx->conn);
    ctx->conn = NULL;
    free(ctx->song.cover->data);
    free(ctx->song.uri);
    free(ctx->song.title);
    free(ctx->song.artist);
  }
}

static void mpd_toggle_playback(Mpd *ctx) {
  switch (ctx->status.state) {
    case MPD_STATE_UNKNOWN:
    case MPD_STATE_STOP: mpd_run_play(ctx->conn); break;
    case MPD_STATE_PAUSE:
    case MPD_STATE_PLAY: mpd_run_toggle_pause(ctx->conn); break;
  }
}

static void mpd_safe_next(Mpd *ctx) {
  if (ctx->status.state == MPD_STATE_STOP)
    mpd_toggle_playback(ctx);
  mpd_run_next(ctx->conn);
}

static void mpd_safe_prev(Mpd *ctx) {
  if (ctx->status.state == MPD_STATE_STOP)
    mpd_toggle_playback(ctx);
  mpd_run_previous(ctx->conn);
}

void mpd_handle_keys(uint32_t key_id, Mpd *ctx) {
  switch (key_id) {
    case 'p': mpd_toggle_playback(ctx);                                       break;
    case 'n': mpd_safe_next(ctx);                                             break;
    case 'N': mpd_safe_prev(ctx);                                             break;
    case 's': mpd_run_stop(ctx->conn);                                        break;
    case 'r': mpd_run_repeat(ctx->conn, !ctx->status.repeat);                 break;
    case 'R': mpd_run_random(ctx->conn, !ctx->status.random);                 break;
    case 't': mpd_run_single(ctx->conn, !ctx->status.single);                 break;
    case 'f': mpd_run_seek_current(ctx->conn, 2, true);                       break;
    case 'b': mpd_run_seek_current(ctx->conn, -2, true);                      break;
    case 'j': mpd_run_set_volume(ctx->conn, fmax(ctx->status.volume-5, 0));   break;
    case 'k': mpd_run_set_volume(ctx->conn, fmin(ctx->status.volume+5, 100)); break;
  }
  return;
}

static int mpd_load_cover(Mpd *ctx) {
  const size_t capacity = 1024*1024;
  unsigned char *read = malloc(capacity);
  if (read == NULL)
    return 0;
  size_t size = 0;
  // read the image
  int chunk;
  while (size < capacity) {
    chunk = mpd_run_readpicture(ctx->conn, ctx->song.uri, size, read+size, capacity-size);
    if (chunk <= 0)
      break;
    size += (size_t)chunk;
  }
  if (size == 0) {
    free(read);
    return 0;
  }
  // load the image
  int result = image_load(read, size, ctx->song.cover);
  free(read);
  return result;
}

void mpd_get_current_song(Mpd *ctx) {
  update_status(ctx);
  if (ctx->status.changed) {
    free(ctx->song.cover->data);
    if (ctx->song.cover->can_load)
      if (!mpd_load_cover(ctx))
        generate_default_cover(ctx->song.cover);
  }
}
