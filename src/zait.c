#include "config.h"
#include "layouts.h"
#include "mpd.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#include <mpd/client.h>
#include <notcurses/notcurses.h>
#include <unistd.h>

int main(void) {
  // INIT CONFIG
  // TODO: Config
  Config cfg = {
    .mpd = { .host = "localhost", .port = 6600, .timeout = 10000 },
    .target_fps = 24, // To save your CPU
  };
  // INIT MPD
  Image cover = {0};
  Mpd mpd = {
    .conn   = NULL,
    .config = cfg.mpd,
    .song   = {
      .cover = &cover,
    },
  };
  int connected = mpd_connect(&mpd);
  if (!connected) {
    fprintf(stderr, "MPD: Couldn't connect\n");
    return 1;
  }
  // INIT NOTCURSES
  struct notcurses_options ncopts = {
    .flags    = NCOPTION_SUPPRESS_BANNERS,
    .margin_t = 1,
    .margin_r = 1,
    .margin_b = 1,
    .margin_l = 1,
  };
  struct notcurses *nc = notcurses_init(&ncopts, NULL);
  if (nc == NULL) {
    mpd_disconnect(&mpd);
    return EXIT_FAILURE;
  }
  struct ncplane *stdplane = notcurses_stdplane(nc);
  struct ncinput ni;
  // CREATE TABS
  struct nctabbed_options tbopts = {
    .selchan   = NCCHANNELS_INITIALIZER(0, 0, 0, 255, 255, 255),
    .separator = " ",
  };
  Context ctx = {
    .mpd = &mpd,
    .tab = {
      .tb = nctabbed_create(stdplane, &tbopts),
    },
  };
  ctx.tab.all[PLAYER]  = nctabbed_add(ctx.tab.tb, NULL, NULL, player_callback, " [1] PLAYER ", &ctx);
  ctx.tab.all[HELP]    = nctabbed_add(ctx.tab.tb, ctx.tab.all[PLAYER], NULL, help_callback, " [2] HELP ", &ctx);
  ctx.tab.current = PLAYER;
  // START LOOP
  int quit = 0;
  unsigned int key_id;
  while (!quit) {
    // LISTEN KEYS
    key_id = notcurses_get_nblock(nc, &ni);
    if (key_id != 0 &&
      (ni.evtype == NCTYPE_RELEASE ||
      ni.evtype == NCTYPE_REPEAT ||
      ni.evtype == NCTYPE_UNKNOWN)) { // NOTE: for st does not work without NCTYPE_UNKNOWN
      if (key_id == 'q')
        quit = 1;
      mpd_handle_keys(key_id, &mpd);
      tab_selector(&ctx, key_id);
    }

    mpd_get_current_song(&mpd);

    // RENDER CONTENT
    nctabbed_redraw(ctx.tab.tb);
    notcurses_render(nc);
    // FPS
    usleep(1000000 / cfg.target_fps);
  }
  mpd_disconnect(&mpd);
  ncvisual_destroy(ctx.player.cover);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
