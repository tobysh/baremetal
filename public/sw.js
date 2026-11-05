// Baremetal service worker. Deliberately small.
//  - Caches only immutable build assets (/_next/static/*) and the icons, so repeat loads are fast.
//  - Navigations always go to the network (pages can be account-specific, so they are never stored). If the network is
//    unreachable, the pre-cached /offline page is shown instead.
//  - /api/*, auth routes, non-GET requests and anything cross-origin are left alone: no cached account data, no cached
//    compile/test/tutor responses.
const VERSION = "bm-v1";
const STATIC = `${VERSION}-static`;
const OFFLINE_URL = "/offline";

self.addEventListener("install", (event) => {
  event.waitUntil(caches.open(STATIC).then((c) => c.add(new Request(OFFLINE_URL, { cache: "reload" }))).then(() => self.skipWaiting()));
});

self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys().then((keys) => Promise.all(keys.filter((k) => !k.startsWith(VERSION)).map((k) => caches.delete(k)))).then(() => self.clients.claim()),
  );
});

self.addEventListener("fetch", (event) => {
  const req = event.request;
  if (req.method !== "GET") return;
  const url = new URL(req.url);
  if (url.origin !== self.location.origin) return;

  if (req.mode === "navigate") {
    event.respondWith(fetch(req).catch(() => caches.match(OFFLINE_URL).then((r) => r ?? Response.error())));
    return;
  }

  if (url.pathname.startsWith("/_next/static/") || /^\/icon-\d+\.png$/.test(url.pathname)) {
    event.respondWith(
      caches.open(STATIC).then(async (cache) => {
        const hit = await cache.match(req);
        if (hit) return hit;
        const res = await fetch(req);
        if (res.ok) cache.put(req, res.clone());
        return res;
      }),
    );
  }
});
