#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
raylib=${RAYLIB_DIR:?Set RAYLIB_DIR to raylib 6.0}
# TF_VARIANT (2026-10-19): 2d = the classic top-down game only, 3d = the 3D game only, empty = the
# old combined build with the view switch. A variant builds into build-web-<variant>.
variant=${TF_VARIANT:-}
case "$variant" in
  2d) define=-DTF_VARIANT_2D ;;
  3d) define=-DTF_VARIANT_3D ;;
  "") define= ;;
  *) echo "TF_VARIANT must be 2d, 3d or empty (got '$variant')" >&2; exit 1 ;;
esac
# Let 2D grow its heap as needed instead of allocating 512 MB at startup.
# The maximum retains headroom for the existing wilderness and gear screens.
memory=(-sINITIAL_MEMORY=536870912)
if [ "$variant" = "2d" ]; then
  memory=(-sINITIAL_MEMORY=134217728 -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=536870912)
fi
out=build-web${variant:+-$variant}
mkdir -p "$out"
em++ -O1 -std=c++17 -DPLATFORM_WEB $define main.cpp \
  -I "$raylib/src" "$raylib/src/libraylib.web.a" \
  -sUSE_GLFW=3 "${memory[@]}" -sSTACK_SIZE=1048576 -lidbfs.js \
  --preload-file assets --shell-file shell_3d.html -o "$out/townforge.html"
mv "$out/townforge.html" "$out/index.html"
