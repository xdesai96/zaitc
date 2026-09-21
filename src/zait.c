#include "config.h"
#include "tabs.h"
#include "player.h"
#include "mpd.h"
#include <assert.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"
#include "../thirdparty/tomlc17.h"

#include <mpd/client.h>
#include <notcurses/notcurses.h>
#include <unistd.h>

void handle_resize(TabList tabs, PlayerContext *playerCtx, struct notcurses *nc, struct ncplane *stdplane, unsigned int *std_y, unsigned int *std_x) {
  unsigned int nstd_y, nstd_x;
  notcurses_term_dim_yx(nc, &nstd_y, &nstd_x);
  if (*std_y != nstd_y || *std_x != nstd_x) {
    *std_y = nstd_y;
    *std_x = nstd_x;
    notcurses_refresh(nc, NULL, NULL);
    switch (tabs.current->key_id) {
      case '1':
        player_handle_resize(&playerCtx->view, stdplane);
        break;
      default: break;
    }
  }
}

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
    return EXIT_FAILURE;
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
        .height = cfg.tabs.player.plane.height,
        .width = cfg.tabs.player.plane.width,
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
  unsigned int std_y, std_x;
  notcurses_term_dim_yx(nc, &std_y, &std_x);
  while (!quit) {
    // RESIZE HANDLER
    handle_resize(tabs, &playerCtx, nc, stdplane, &std_y, &std_x);

    key_id = notcurses_get_nblock(nc, &ni);
    // LISTEN KEYS
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
