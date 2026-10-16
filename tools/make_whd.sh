#!/bin/bash
#
# Convert a Doom IWAD into the WHD blob the game embeds.
#
#   tools/make_whd.sh /path/to/doom1.wad
#
# Builds the whd_gen tool from vendor/rp2040-doom (once, into build-tools/)
# and writes data/doom1.whd. The data/ folder is git-ignored: never commit
# id Software's game data.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WAD="$1"

if [ -z "$WAD" ] || [ ! -f "$WAD" ]; then
    echo "Usage: $0 /path/to/doom1.wad" >&2
    exit 1
fi

TOOLS="$ROOT/build-tools"
if [ ! -x "$TOOLS/src/whd_gen/whd_gen" ]; then
    # whd_gen predates GCC 13, which no longer pulls in <cstdint> for it.
    cmake -S "$ROOT/vendor/rp2040-doom" -B "$TOOLS" -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-include cstdint -include climits"
    cmake --build "$TOOLS" --target whd_gen -j4
fi

mkdir -p "$ROOT/data"
"$TOOLS/src/whd_gen/whd_gen" "$WAD" "$ROOT/data/doom1.whd" -no-super-tiny
echo "Wrote $ROOT/data/doom1.whd"
