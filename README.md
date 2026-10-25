# chili-doom

Doom for the ChiliChip VGC Zero (RP2350, 128x128 OLED), built on the
[32blit SDK](https://github.com/chili-chip/32blit-sdk) like every other ChiliChip game.

Status: shareware Doom plays at the console's native 128x128 on desktop and on the VGC Zero
(`vgc-blit`), with sound effects and OPL music. Setting music volume to 0 in the in-game menu
turns the music synth off to save CPU.

## Bring your own WAD

This repo contains no id Software game data, and builds published from it never will.
(The ThumbyDOOM submodule carries upstream's `doom1.whx`; chili-doom never builds it in.)
To play, convert your own copy of the shareware `DOOM1.WAD` (version 1.9) before building:

```
tools/make_whd.sh /path/to/doom1.wad    # writes data/doom1.whd (git-ignored)
```

On Ubuntu or Debian the shareware WAD comes in the `doom-wad-shareware` package
(`/usr/share/games/doom/doom1.wad`). Without `data/doom1.whd` the game still builds and
shows "No WAD built in".

## Building (desktop)

The engine comes in as a git submodule, so clone with `--recursive`
(or run `git submodule update --init` in an existing clone).
Check out [32blit-sdk](https://github.com/chili-chip/32blit-sdk) next to this repo, then:

```
cmake --preset linux
cmake --build out/build/linux
./out/build/linux/chili-doom.elf
```

## Building for the VGC Zero

Check out [32blit-sdk](https://github.com/chili-chip/32blit-sdk) and
[pico-sdk](https://github.com/chili-chip/pico-sdk) next to this repo, make `data/doom1.whd` as above, then:

```
cmake --preset vgc-blit
cmake --build out/build/vgc-blit
```

Copy `out/build/vgc-blit/chili-doom.blit` onto the console's **CHILICHIP** drive to install it.
A `.blit` built with your WAD contains id Software's game data: keep it to yourself.

RAM is the tight part: games get 448 KB and Doom uses almost all of it. To make room, the
screen shows Doom's 128x128 frame directly instead of a copy (see `init()` in
`src/doom_blit.cpp`), and the screen-melt snapshot doubles as the menu backdrop.

Controls: arrows move and turn, A fires, B uses, Y and X strafe, MENU opens the Doom menu.
On a keyboard: Z = A, X = B, C = X, V = Y, Esc = MENU (the 32blit SDL defaults).

## How it fits together

| Path | What it is |
| --- | --- |
| `vendor/ThumbyDOOM/` | Submodule: our fork [chili-chip/ThumbyDOOM](https://github.com/chili-chip/ThumbyDOOM) of [ThumbyDOOM](https://github.com/austinio7116/ThumbyDOOM). We build its `vendor/rp2040-doom/` (kilograham's [rp2040-doom](https://github.com/kilograham/rp2040-doom), Chocolate Doom for RP2040, with a native 128x128 renderer), its `port/` platform layer and its `host/` stand-ins for the pico SDK headers. |
| `src/doom_blit.cpp` | The 32blit game: `init` starts Doom, `update` runs one frame per 35 Hz tic, `render` copies the 128x128 frame to the screen. |
| `tools/make_whd.sh` | Builds `whd_gen` and converts a WAD into the compressed WHD blob that gets linked into the game. |

Changes for chili-doom live on the fork and are marked `CHILI_BLIT` or `chili-doom`. The fork is based on ThumbyDOOM `22070a1`
(rp2040-doom `29a453c`).

## Credits

chili-doom only adds the 32blit layer. The game and the port are other people's work. Thanks to:

| Who | What |
| --- | --- |
| [id Software](https://github.com/id-Software/DOOM) | DOOM, and its source code released under the GPL |
| Simon Howard and the [Chocolate Doom](https://www.chocolate-doom.org/) contributors | The source port rp2040-doom is built on |
| Graham Sanderson ([kilograham/rp2040-doom](https://github.com/kilograham/rp2040-doom)) | Doom for the RP2040, with the compressed WHD format and the renderer |
| austinio7116 ([ThumbyDOOM](https://github.com/austinio7116/ThumbyDOOM)) | The 128x128 RP2350 port for the Thumby Color that we build on |
| Pimoroni and the [32blit](https://github.com/32blit/32blit-sdk) contributors | The 32blit SDK (MIT) |

## Licence

GPL v2, inherited from Chocolate Doom and rp2040-doom; see `LICENSE`. The 32blit SDK is MIT.
DOOM is a trademark of id Software; this project is not affiliated with id Software.
