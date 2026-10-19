// Town Forge service worker (2026-09-27): makes the installed app start instantly
// and play offline. Every deploy stamps a new VERSION, so the browser installs a
// fresh copy of the whole game (page, script, wasm, data - always as one matching
// set) in the background; the new version is used from the next launch.
const VERSION = '__VERSION__';
// The 2D and 3D games are separate sites on one web address (2026-10-19), and browsers keep
// cache storage per address, not per site. Each build therefore names its caches with its own
// variant and only ever deletes its own, so an update to one site never wipes the other's
// offline copy. VARIANT is '2d', '3d' or empty (the old combined build).
const VARIANT = '__VARIANT__';
const PREFIX = 'townforge-' + (VARIANT ? VARIANT + '-' : '');
const CACHE = PREFIX + VERSION;
// A cache this build owns: its own variant's, plus (3D, which grew out of the combined site) the
// old untagged ones.
const legacy = (k) => /^townforge-[0-9]{14}-/.test(k); // the old untagged names: townforge-<timestamp>-<commit>
const mine = (k) => VARIANT ? (k.startsWith(PREFIX) || (VARIANT === '3d' && legacy(k))) : legacy(k);
const CORE = [
  './', './index.html', './townforge.js', './townforge.wasm', './townforge.data',
  './manifest.webmanifest', './landscape.js', './cloud.js', './cloud-config.js', './guildnet.js', './mp-config.js', './mpnet.js', './icons/icon-192.png', './icons/icon-512.png', './icons/apple-touch-icon.png',
];

self.addEventListener('install', (e) => {
  e.waitUntil(
    caches.open(CACHE)
      .then((c) => c.addAll(CORE.map((u) => new Request(u, { cache: 'reload' }))))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', (e) => {
  e.waitUntil(
    caches.keys()
      .then((keys) => Promise.all(keys.filter((k) => mine(k) && k !== CACHE).map((k) => caches.delete(k))))
      .then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET' || new URL(req.url).origin !== self.location.origin) return;
  if (new URL(req.url).pathname.indexOf('/wiki/') !== -1) return; // the wiki is a normal web page, never the game
  // Release checks must read the published marker, not the cached game shell.
  if (new URL(req.url).pathname.endsWith('/source-version.txt')) return;
  e.respondWith((async () => {
    const cache = await caches.open(CACHE);
    const hit = await cache.match(req, { ignoreSearch: true }) ||
                (req.mode === 'navigate' ? await cache.match('./index.html') : undefined);
    if (hit) return hit;
    const res = await fetch(req);
    if (res.ok && res.type === 'basic') cache.put(req, res.clone());
    return res;
  })());
});
