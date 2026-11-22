#!/bin/bash
# Build gpuprobe. Needs glslangValidator, gcc, libvulkan, and a directory with vulkan/vulkan.h
# (default: the headers vendored in a gpubench checkout).
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
INC=${VULKAN_INCLUDE:-$HOME/dev/gpubench/main/rt/third_party}
OUT=${1:-$HOME/.cache/gpuparity/gpuprobe}
mkdir -p "$OUT"
glslangValidator -V "$HERE/probe.comp" --vn probe_spv -o "$OUT/probe_spv.h" >/dev/null
gcc -O2 -Wall -Wextra -std=gnu11 -I"$INC" -I"$OUT" -o "$OUT/gpuprobe" "$HERE/gpuprobe.c" -lvulkan -lm
echo "$OUT/gpuprobe"
