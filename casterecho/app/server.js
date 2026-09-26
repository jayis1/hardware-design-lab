// CasterEcho zero-dependency static server
// Author: jayis1
// Copyright (c) 2026 jayis1
// SPDX-License-Identifier: MIT
'use strict';
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const root = __dirname;
const types = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8' };
const server = http.createServer((request, response) => {
  const requested = request.url === '/' ? '/index.html' : request.url.split('?')[0];
  const file = path.resolve(root, `.${requested}`);
  if (!file.startsWith(root) || !Object.hasOwn(types, path.extname(file))) { response.writeHead(404); response.end('Not found'); return; }
  fs.readFile(file, (error, data) => {
    if (error) { response.writeHead(404); response.end('Not found'); return; }
    response.writeHead(200, { 'Content-Type': types[path.extname(file)], 'Cache-Control': 'no-store', 'X-Content-Type-Options': 'nosniff' });
    response.end(data);
  });
});
if (require.main === module) server.listen(Number(process.env.PORT || 4173), '127.0.0.1', () => console.log('CasterEcho app: http://127.0.0.1:4173'));
module.exports = server;
