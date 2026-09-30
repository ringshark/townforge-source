#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
raylib=${RAYLIB_DIR:?Set RAYLIB_DIR to raylib 6.0}
mkdir -p build-web
em++ -O1 -std=c++17 -DPLATFORM_WEB main.cpp \
  -I "$raylib/src" "$raylib/src/libraylib.web.a" \
  -sUSE_GLFW=3 -sINITIAL_MEMORY=536870912 -sSTACK_SIZE=1048576 -lidbfs.js \
  --preload-file assets --shell-file shell_3d.html -o build-web/townforge.html
mv build-web/townforge.html build-web/index.html
