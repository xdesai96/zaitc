#include "config.h"

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

static int config_default(Config *config) {
  config->mpd.host = strdup("localhost");
  if (config->mpd.host == NULL) {
    return 0;
  }
  config->mpd.port = 6600;
  config->mpd.timeout = 10000;
  return 1;
}

static int load_mpd_config(Config *config) {
  char *path = config_path();
  if (!path) {
    return -1;
  }
  toml_result_t result = toml_parse_file_ex(path);
  if (!result.ok) {
    fprintf(stderr, "TOML error: %s\n", result.errmsg);
    free(path);
    return 0;
  }
  free(path);

  const char *host;
  toml_datum_t host_parsed = toml_seek(result.toptab, "mpd.host");
  switch (host_parsed.type) {
    case TOML_UNKNOWN: host = "localhost";    break;
    case TOML_STRING: host = host_parsed.u.s; break;
    default:
      fprintf(stderr, "TOML error: invalid 'host' type in config.\n");
      toml_free(result);
      return 0;
  }
  int port, timeout;
  toml_datum_t port_parsed = toml_seek(result.toptab, "mpd.port");
  switch (port_parsed.type) {
    case TOML_UNKNOWN: port = 6600;              break;
    case TOML_INT64: port = port_parsed.u.int64; break;
    default:
      fprintf(stderr, "TOML error: invalid 'port' type in config.\n");
      toml_free(result);
      return 0;
  }
  toml_datum_t timeout_parsed = toml_seek(result.toptab, "mpd.timeout");
  switch (timeout_parsed.type) {
    case TOML_UNKNOWN: timeout = 10000;             break;
    case TOML_INT64: timeout = timeout_parsed.u.int64; break;
    default:
      fprintf(stderr, "TOML error: invalid 'timeout' type in config.\n");
      toml_free(result);
      return 0;
  }
  config->mpd.host = strdup(host);
  if (config->mpd.host == NULL) {
    toml_free(result);
    return 0;
  }
  config->mpd.port = port;
  config->mpd.timeout = timeout;
  toml_free(result);
  return 1;
}

int config_load(Config *config) {
  int loaded = load_mpd_config(config);
  if (loaded < 0) // Couldn't read the config file
    return config_default(config);
  if (!loaded) // Read the file but couldn't properly load
    return 0;
  return 1; // Well done
}

void config_free(Config *config) {
  free(config->mpd.host);
}
