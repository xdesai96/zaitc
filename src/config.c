#include "config.h"

static void error_loading(toml_result_t *result, const char *mes) {
  fprintf(stderr, "TOML error: %s\n", mes);
  if (result != NULL)
    toml_free(*result);
  exit(EXIT_FAILURE);
}

static char *config_path(void) {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");
  const char *suffix;
  if (xdg && xdg[0] != '\0') {
    suffix = "/zaitc/config.toml";
    size_t len = strlen(xdg) + strlen(suffix) + 1;
    char *path = malloc(len);
    if (!path)
      return NULL;
    snprintf(path, len, "%s%s", xdg, suffix);
    return path;
  }
  if (home && home[0] != '\0') {
    suffix = "/.config/zaitc/config.toml";
    size_t len = strlen(home) + strlen(suffix) + 1;
    char *path = malloc(len);
    if (!path)
      return NULL;
    snprintf(path, len, "%s%s", home, suffix);
    return path;
  }
  return NULL;
}

static int load_int(toml_result_t result, const char *key, int def, int *out) {
  toml_datum_t value = toml_seek(result.toptab, key);
  switch (value.type) {
    case TOML_UNKNOWN:
      *out = def;
      return 1;
    case TOML_INT64:
      *out = value.u.int64;
      return 1;
    default:
      return 0;
  }
}

static const char *load_str(toml_result_t result, const char *key, const char *def) {
  toml_datum_t value = toml_seek(result.toptab, key);
  switch (value.type) {
    case TOML_UNKNOWN:
      return def;
    case TOML_STRING:
      return value.u.s;
    default:
      return NULL;
  }
}

static void load_mpd_config(toml_result_t *result, Config *config) {
  const char *host = load_str(*result, "mpd.host", "localhost");
  if (host == NULL)
    error_loading(result, "`mpd.host` is not valid in config.");
  int port, timeout;
  if (!load_int(*result, "mpd.port", 6600, &port))
    error_loading(result, "`mpd.port` is not valid in config.");
  if (!load_int(*result, "mpd.timeout", 10000, &timeout))
    error_loading(result, "`mpd.timeout` is not valid in config.");
  config->mpd.host = strdup(host);
  if (config->mpd.host == NULL)
    error_loading(result, "Couldn't allocate memory for `mpd.host`.");
  config->mpd.port = port;
  config->mpd.timeout = timeout;
}

static void load_tabs_config(toml_result_t *result, Config *config) {
  int width, height;
  if (!load_int(*result, "tabs.player.plane.width", 80, &width))
    error_loading(result, "`tabs.player.plane.width` is not valid in config.");
  if (!load_int(*result, "tabs.player.plane.height", 15, &height)) {
    error_loading(result, "`tabs.player.plane.height` is not valid in config.");
  }
  config->tabs.player.plane.height= height;
  config->tabs.player.plane.width = width;
}

int config_load(Config *config) {
  char *path = config_path();
  if (!path) return -1;
  toml_result_t result = toml_parse_file_ex(path);
  free(path);
  if (!result.ok) error_loading(&result, result.errmsg);
  load_mpd_config(&result, config);
  load_tabs_config(&result, config);
  toml_free(result);
  return 1;
}

void config_free(Config *config) {
  if (config->mpd.host != NULL)
    free(config->mpd.host);
}
