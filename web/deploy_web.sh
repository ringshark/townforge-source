#!/bin/bash
# usage: web/deploy_web.sh <build dir with index.html/townforge.*> <site dir>
set -euo pipefail
src=${1:?build directory required}; dst=${2:?site directory required}; here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$dst"
cp "$src"/index.html "$src"/townforge.js "$src"/townforge.wasm "$src"/townforge.data "$dst"/
cp "$here"/manifest.webmanifest "$here"/cloud.js "$here"/guildnet.js "$here"/mpnet.js "$here"/landscape.js "$dst"/
# Multiplayer (2026-09-29): the zone server's address from TF_MP_URL (wss://...workers.dev).
if [ -n "${TF_MP_URL:-}" ]; then
  printf "window.TF_MP_URL = '%s';\n" "${TF_MP_URL:-}" > "$dst"/mp-config.js
  echo "multiplayer: ON ($TF_MP_URL)"
elif [ -f "$dst"/mp-config.js ] && grep -q "wss://" "$dst"/mp-config.js; then
  echo "multiplayer: ON (kept the site's existing mp-config.js)"
else
  cp "$here"/mp-config.js "$dst"/mp-config.js
  echo "multiplayer: off (set TF_MP_URL after deploying server/mp)"
fi
# Cloud saves: configured from the environment (never committed). The anon key is
# Supabase's public browser key - row-level security is what protects the saves.
if [ -n "${SUPABASE_URL:-}" ] && [ -n "${SUPABASE_ANON_KEY:-}" ]; then
  printf 'window.TF_CLOUD = { url: "%s", key: "%s" };\n' "${SUPABASE_URL:-}" "${SUPABASE_ANON_KEY:-}" > "$dst"/cloud-config.js
  echo "cloud saves: ON"
elif [ -f "$dst"/cloud-config.js ] && grep -q 'url:' "$dst"/cloud-config.js; then
  echo "cloud saves: ON (kept the site's existing cloud-config.js)"
else
  echo 'window.TF_CLOUD = null; // cloud saves not configured (set SUPABASE_URL + SUPABASE_ANON_KEY)' > "$dst"/cloud-config.js
  echo "cloud saves: off (SUPABASE_URL / SUPABASE_ANON_KEY not set)"
fi
mkdir -p "$dst"/icons && cp "$here"/icons/*.png "$dst"/icons/
mkdir -p "$dst"/wiki && cp "$here"/wiki/index.html "$dst"/wiki/ # the player wiki, public at <site>/wiki/
stamp=$(date -u +%Y%m%d%H%M%S)-$(git -C "$here" rev-parse --short HEAD)
# TF_VARIANT (2026-10-19): 2d / 3d / empty (the old combined build). The service worker names its
# caches after the variant, and the two sites are told apart by name where a player sees it.
variant=${TF_VARIANT:-}
sed -e "s/__VERSION__/$stamp/" -e "s/__VARIANT__/$variant/" "$here"/sw.js > "$dst"/sw.js
if [ "$variant" = "3d" ]; then
  sed -i -e 's/"name": "Town Forge"/"name": "Town Forge 3D"/' -e 's/"short_name": "Town Forge"/"short_name": "Town Forge 3D"/' "$dst"/manifest.webmanifest
  sed -i -e 's#<title>Town Forge</title>#<title>Town Forge 3D</title>#' -e 's#content="Town Forge"name=apple-mobile-web-app-title#content="Town Forge 3D"name=apple-mobile-web-app-title#' "$dst"/index.html
elif [ "$variant" = "2d" ]; then
  sed -i -e 's/in a living 3D world\./in a classic top-down world./' "$dst"/manifest.webmanifest
fi
echo "deployed web app version $stamp${variant:+ ($variant)}"
