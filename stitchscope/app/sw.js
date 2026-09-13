/* StitchScope offline worker | Author: jayis1 | Copyright (C) 2026 jayis1 */
'use strict';
const CACHE = 'stitchscope-jayis1-v1';
const ASSETS = ['./', './index.html', './styles.css', './protocol.js', './app.js', './manifest.webmanifest'];
self.addEventListener('install', event => event.waitUntil(caches.open(CACHE).then(cache => cache.addAll(ASSETS))));
self.addEventListener('activate', event => event.waitUntil(caches.keys().then(keys => Promise.all(keys.filter(k => k !== CACHE).map(k => caches.delete(k))))));
self.addEventListener('fetch', event => {
  if (event.request.method !== 'GET') return;
  event.respondWith(caches.match(event.request).then(hit => hit || fetch(event.request)));
});
