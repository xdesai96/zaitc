#ifndef ZAITC_MPD
#define ZAITC_MPD

#include "image.h"
#include <math.h>
#include <mpd/client.h>
#include <string.h>

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
} Mpd;

int mpd_connect(Mpd *ctx);
void mpd_disconnect(Mpd *ctx);

void mpd_get_current_song(Mpd *ctx);
void mpd_handle_keys(uint32_t key_id, Mpd *ctx);

#endif // ZAITC_MPD
