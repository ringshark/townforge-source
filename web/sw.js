// Town Forge service worker (2026-09-27): makes the installed app start instantly
// and play offline. Every deploy stamps a new VERSION, so the browser installs a
// fresh copy of the whole game (page, script, wasm, data - always as one matching
// set) in the background; the new version is used from the next launch.
const VERSION = '__VERSION__';
const CACHE = 'townforge-' + VERSION;
const CORE = [
  './', './index.html', './townforge.js', './townforge.wasm', './townforge.data',
  './manifest.webmanifest', './icons/icon-192.png', './icons/icon-512.png', './icons/apple-touch-icon.png',
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
      .then((keys) => Promise.all(keys.filter((k) => k.startsWith('townforge-') && k !== CACHE).map((k) => caches.delete(k))))
      .then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET' || new URL(req.url).origin !== self.location.origin) return;
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
