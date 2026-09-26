#!/bin/bash
# usage: web/deploy_web.sh <build dir with index.html/townforge.*> <site dir>
set -e
src=$1; dst=$2; here=$(cd "$(dirname "$0")" && pwd)
cp "$src"/index.html "$src"/townforge.js "$src"/townforge.wasm "$src"/townforge.data "$dst"/
cp "$here"/manifest.webmanifest "$here"/cloud.js "$dst"/
# Cloud saves: configured from the environment (never committed). The anon key is
# Supabase's public browser key - row-level security is what protects the saves.
if [ -n "$SUPABASE_URL" ] && [ -n "$SUPABASE_ANON_KEY" ]; then
  printf 'window.TF_CLOUD = { url: "%s", key: "%s" };\n' "$SUPABASE_URL" "$SUPABASE_ANON_KEY" > "$dst"/cloud-config.js
  echo "cloud saves: ON"
else
  echo 'window.TF_CLOUD = null; // cloud saves not configured (set SUPABASE_URL + SUPABASE_ANON_KEY)' > "$dst"/cloud-config.js
  echo "cloud saves: off (SUPABASE_URL / SUPABASE_ANON_KEY not set)"
fi
mkdir -p "$dst"/icons && cp "$here"/icons/*.png "$dst"/icons/
stamp=$(date -u +%Y%m%d%H%M%S)-$(git -C "$here" rev-parse --short HEAD)
sed "s/__VERSION__/$stamp/" "$here"/sw.js > "$dst"/sw.js
echo "deployed web app version $stamp"
