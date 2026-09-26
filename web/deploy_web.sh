#!/bin/bash
# usage: web/deploy_web.sh <build dir with index.html/townforge.*> <site dir>
set -e
src=$1; dst=$2; here=$(cd "$(dirname "$0")" && pwd)
cp "$src"/index.html "$src"/townforge.js "$src"/townforge.wasm "$src"/townforge.data "$dst"/
cp "$here"/manifest.webmanifest "$dst"/
mkdir -p "$dst"/icons && cp "$here"/icons/*.png "$dst"/icons/
stamp=$(date -u +%Y%m%d%H%M%S)-$(git -C "$here" rev-parse --short HEAD)
sed "s/__VERSION__/$stamp/" "$here"/sw.js > "$dst"/sw.js
echo "deployed web app version $stamp"
