#include "config.h"

static char *config_path(void) {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");
  if (xdg && xdg[0] != '\0') {
    char *path = malloc(strlen(xdg) + strlen("/zaitc/config.toml") + 1);
    if (!path)
      return NULL;
    sprintf(path, "%s/zaitc/config.toml", xdg);
    return path;
  }
  if (home && home[0] != '\0') {
    char *path = malloc(strlen(home) + strlen("/.config/zaitc/config.toml") + 1);
    if (!path)
      return NULL;
    sprintf(path, "%s/.config/zaitc/config.toml", home);
    return path;
  }
  return NULL;
}

static int load_mpd_config(Config *config) {
  char *path = config_path();
  if (!path) {
    fprintf(stderr, "TOML error: Couldn't determine config path\n");
    return 0;
  }
  toml_result_t result = toml_parse_file_ex(path);
  if (!result.ok) {
    fprintf(stderr, "TOML error: %s\n", result.errmsg);
    return 0;
  }
  const char *host;
  toml_datum_t host_parsed = toml_seek(result.toptab, "mpd.host");
  switch (host_parsed.type) {
    case TOML_UNKNOWN: host = "localhost";    break;
    case TOML_STRING: host = host_parsed.u.s; break;
    default:
      fprintf(stderr, "TOML error: invalid 'host' type in config.\n");
      toml_free(result);
      return 0;
      break;
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
      break;
  }
  toml_datum_t timeout_parsed = toml_seek(result.toptab, "mpd.timeout");
  switch (timeout_parsed.type) {
    case TOML_UNKNOWN: timeout = 10000;             break;
    case TOML_INT64: timeout = timeout_parsed.u.int64; break;
    default:
      fprintf(stderr, "TOML error: invalid 'timeout' type in config.\n");
      toml_free(result);
      return 0;
      break;
  }
  config->mpd.host = strdup(host);
  config->mpd.port = port;
  config->mpd.timeout = timeout;
  toml_free(result);
  return 1;
}

int config_load(Config *config) {
  if (!load_mpd_config(config))
    return 0;
  return 1;
}

void config_free(Config *config) {
  free(config->mpd.host);
}
