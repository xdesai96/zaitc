# ZaitC

ZaitC is a minimal TUI music player for MPD, written in C and built with notcurses.

## Preview

![ZaitC screenshot](assets/screenshot.png)

## Installation

### Requirements

* `GCC` or another compatible C compiler
* `pkg-config`
* [notcurses](https://notcurses.com)
* [libmpdclient](https://musicpd.org/libs/libmpdclient)
* [MPD](https://www.musicpd.org/)

### Build from source

```bash
git clone https://codeberg.org/xdesai/zaitc.git
cd zaitc
sudo make install
```

### Run with nix

```bash
nix run git+https://codeberg.org/xdesai/zaitc.git
```

## Usage

### Keybindings

<details><summary>The table of bindings</summary>

| Key   | Action        |
| ----- | ------------- |
| `p`   | Play / pause  |
| `s`   | Stop          |
| `n`   | Next song     |
| `N`   | Previous song |
| `f`   | Seek forward  |
| `b`   | Seek backward |
| `k`   | Volume up     |
| `j`   | Volume down   |
| `r`   | Toggle repeat |
| `R`   | Toggle random |
| `t`   | Toggle single |
| `Tab` | Next Tab      |

</details>

### Configuration

The configuration file is loaded from:

- `$XDG_CONFIG_HOME/zaitc/config.toml`
- `$HOME/.config/zaitc/config.toml` if `XDG_CONFIG_HOME` is not set

Default config:

```toml
[mpd]
host = "localhost"
port = 6600
timeout = 10000

[tabs.player]
plane = { width = 80, height = 15 }
```

## TODO

- [ ] Add command-line options (e.g. `--host` `--port`)
- [ ] Add more configuration options
- [ ] Add `library` tab
- [ ] Add `queue` tab
- [ ] Finish `help` tab
