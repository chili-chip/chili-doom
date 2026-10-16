# chili-doom

Doom for the ChiliChip VGC Zero (RP2350, 128x128 OLED), built on the
[32blit SDK](https://github.com/chili-chip/32blit-sdk) like every other ChiliChip game.

Status: the desktop build plays the shareware demo loop at the console's native 128x128.
The console (`vgc-blit`) build does not compile yet; that is the next milestone.

## Bring your own WAD

This repo contains no id Software game data, and builds published from it never will.
To play, convert your own copy of the shareware `DOOM1.WAD` (version 1.9) before building:

```
tools/make_whd.sh /path/to/doom1.wad    # writes data/doom1.whd (git-ignored)
```

On Ubuntu or Debian the shareware WAD comes in the `doom-wad-shareware` package
(`/usr/share/games/doom/doom1.wad`). Without `data/doom1.whd` the game still builds and
shows "No WAD built in".

## Building (desktop)

Check out [32blit-sdk](https://github.com/chili-chip/32blit-sdk) next to this repo, then:

```
cmake --preset linux
cmake --build out/build/linux
./out/build/linux/chili-doom.elf
```

Controls: arrows move and turn, A fires, B uses, Y and X strafe, MENU opens the Doom menu.
On a keyboard: Z = A, X = B, C = X, V = Y, Esc = MENU (the 32blit SDL defaults).

## How it fits together

| Path | What it is |
| --- | --- |
| `vendor/rp2040-doom/` | kilograham's [rp2040-doom](https://github.com/kilograham/rp2040-doom) (Chocolate Doom for RP2040), as modified by [ThumbyDOOM](https://github.com/austinio7116/ThumbyDOOM) for a native 128x128 renderer. `doom1.whx` removed. |
| `port/` | ThumbyDOOM's platform layer (video, input, sound, system), lightly adapted. |
| `shim/` | Stand-ins for the pico SDK headers the engine includes, so it builds on every 32blit target. |
| `src/doom_blit.cpp` | The 32blit game: `init` starts Doom, `update` runs one frame per 35 Hz tic, `render` copies the 128x128 frame to the screen. |
| `tools/make_whd.sh` | Builds `whd_gen` and converts a WAD into the compressed WHD blob that gets linked into the game. |

Changes to vendored files are marked `CHILI_BLIT` (in `src/doom/d_main.c` and `src/i_swap.h`).

Upstream snapshots: rp2040-doom `29a453c` via ThumbyDOOM `22070a1`.

## Licence

GPL v2, inherited from Chocolate Doom and rp2040-doom; see `LICENSE`. The 32blit SDK is MIT.
DOOM is a trademark of id Software; this project is not affiliated with id Software.
