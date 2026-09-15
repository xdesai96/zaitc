#ifndef ZAITC_MPD
#define ZAITC_MPD

#include "image.h"
#include <string.h>
#include <mpd/client.h>

typedef struct {
  bool changed;
  bool random;
  bool repeat;
  enum mpd_consume_state consume;
  enum mpd_single_state single;
  enum mpd_state state;
  int volume;
  unsigned duration;
  unsigned elapsed;
} MpdStatus;

typedef struct {
  Image *cover;
  char *artist;
  char *title;
  char *uri;
  struct mpd_song *current;
} MpdSong;

typedef struct {
  MpdConfig config;
  MpdSong song;
  MpdStatus status;
  struct mpd_connection *conn;
} MpdContext;

int mpd_connect(MpdContext *ctx);
void mpd_disconnect(MpdContext *ctx);

int mpd_get_current_song(MpdContext *ctx);
void mpd_handle_keys(uint32_t key_id, MpdContext *ctx);

int mpd_load_cover(MpdContext *ctx);

#endif // ZAITC_MPD
