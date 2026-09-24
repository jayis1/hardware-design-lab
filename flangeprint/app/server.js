/* FlangePrint dependency-free local server.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
'use strict';
const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const types = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8'
};
const root = __dirname;
const server = http.createServer((request, response) => {
  const pathname = request.url === '/' ? '/index.html' : request.url.split('?')[0];
  const resolved = path.resolve(root, '.' + pathname);
  if (!resolved.startsWith(root)) {
    response.writeHead(403);
    response.end('Forbidden');
    return;
  }
  fs.readFile(resolved, (error, data) => {
    if (error) {
      response.writeHead(404);
      response.end('Not found');
      return;
    }
    response.writeHead(200, {
      'Content-Type': types[path.extname(resolved)] || 'application/octet-stream',
      'Cache-Control': 'no-store'
    });
    response.end(data);
  });
});
if (require.main === module) {
  server.listen(Number(process.env.PORT || 4173), '127.0.0.1', () =>
    console.log('FlangePrint: http://127.0.0.1:4173'));
}
module.exports = server;
