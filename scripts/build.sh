#!/usr/bin/env bash
# Build the XIAO MG24 RGB lamp firmware from the command line.
#
#   scripts/build.sh            generate the slc project into build/gen and build it
#
# Pinned tools (override with environment variables):
#   SLC        slc-cli 6.0.23              (default ~/.local/opt/slc_cli/slc)
#   GCC_BIN    Arm GNU Toolchain 14.2.Rel1 (bin directory)
# The Simplicity SDK 2026.6.1 must be registered with slc
# (`slc configuration --sdk <path>` and `slc signature trust --sdk <path>`).
#
# Output: build/gen/build/debug/xiao_mg24_rgb_lamp.{s37,hex,bin,out}
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SLC="${SLC:-$HOME/.local/opt/slc_cli/slc}"
GCC_BIN="${GCC_BIN:-/opt/silabs/installs/conan/p/gcc-a469753773d4f2/p/bin}"
PART="EFR32MG24B220F1536IM48"
OUT="$ROOT/build/gen"

export PATH="$GCC_BIN:$PATH"
arm-none-eabi-gcc --version | head -1

rm -rf "$OUT"
"$SLC" generate -p "$ROOT/firmware/xiao_mg24_rgb_lamp.slcp" -d "$OUT" \
  --with "$PART" -o makefile
make -C "$OUT" -j"$(nproc 2>/dev/null || echo 2)" -f xiao_mg24_rgb_lamp.Makefile

arm-none-eabi-size "$OUT/build/debug/xiao_mg24_rgb_lamp.out"
ls -l "$OUT"/build/debug/xiao_mg24_rgb_lamp.{s37,hex}
