#include "config.h"
#include "tabs.h"
#include "player.h"
#include "mpd.h"
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"
#include "../thirdparty/tomlc17.h"

#include <mpd/client.h>
#include <notcurses/notcurses.h>
#include <unistd.h>

int main(void) {
  // LOAD CONFIG
  Config cfg = {0};
  if (!config_load(&cfg)) {
    return EXIT_FAILURE;
  }
  // INIT MPD
  Image cover = {0};
  Mpd mpd = {
    .conn   = NULL,
    .config = cfg.mpd,
    .song = {
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
    config_free(&cfg);
    mpd_disconnect(&mpd);
    fprintf(stderr, "NOTCURSES: Couldn't initialize");
    return EXIT_FAILURE;
  }
  struct ncplane *stdplane = notcurses_stdplane(nc);
  struct ncinput ni;
  // CREATE TABS
  struct nctabbed_options tbopts = {
    .selchan   = NCCHANNELS_INITIALIZER(255, 255, 255, NCALPHA_TRANSPARENT, NCALPHA_TRANSPARENT, NCALPHA_TRANSPARENT),
    .hdrchan   = NCCHANNELS_INITIALIZER(155, 155, 155, NCALPHA_TRANSPARENT, NCALPHA_TRANSPARENT, NCALPHA_TRANSPARENT),
    .separator = " ",
  };
  PlayerContext playerCtx = {
    .mpd = &mpd,
    .view = {
      .root = {
        .height = 15,
        .width = 80,
      },
    },
  };
  TabList tabs = { .tb = nctabbed_create(stdplane, &tbopts) };
  TabView player = {
    .key_id = '1',
    .next = NULL,
    .tab = nctabbed_add(tabs.tb, NULL, NULL, player_callback, " [1] PLAYER ", &playerCtx),
    .destroyFn = player_view_destroy,
    .data = &playerCtx.view,
  };
  tabs.head = &player;
  tabs.current = tabs.head;
  player.next = &(TabView){
    .key_id = '2',
    .next = NULL,
    .tab = nctabbed_add(tabs.tb, player.tab, NULL, NULL, " [2] HELP ", NULL),
    .data = NULL,
    .destroyFn = NULL,
  };
  // Can we load images?
  mpd.song.cover->can_load = notcurses_canopen_images(nc);
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
      tab_bindings(&tabs, key_id);
    }
    mpd_get_current_song(&mpd);
    // RENDER CONTENT
    nctabbed_redraw(tabs.tb);
    notcurses_render(nc);
    // FPS
    usleep(1000000 / 24); // To save your cpu
  }
  mpd_disconnect(&mpd);
  config_free(&cfg);
  ncvisual_destroy(playerCtx.view.cover);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
