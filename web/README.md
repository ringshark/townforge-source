# Web app files

Deployed next to the game build (`index.html`, `townforge.js/.wasm/.data`):

- `manifest.webmanifest` - makes the game installable (home-screen icon, full screen).
- `sw.js` - service worker: offline play and instant start. `__VERSION__` must be
  replaced with a unique stamp on every deploy (see `deploy_web.sh`), so players
  pick up the new build on their next launch.
- `icons/` - app icons (`apple-touch-icon.png` is the iPhone/iPad home-screen icon).
