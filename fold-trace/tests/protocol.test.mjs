// Author: jayis1. SPDX-License-Identifier: MIT
import test from 'node:test';
import assert from 'node:assert/strict';
import {crc, parse, Stream, Session} from '../app/protocol.mjs';
const frame = 'FT1,42,1084,1,0,0,0,0,0,0,0,0,0,0,0,0*BE6F\n';
test('CRC known vector and actual C encoder frame', () => {
  assert.equal(crc('123456789'), 0x29b1);
  assert.equal(parse(frame).uptime, 1084);
});
test('reject corrupted and oversized frames', () => {
  assert.throws(() => parse(frame.replace('1084', '1085')));
  assert.throws(() => parse('x'.repeat(300)));
});
test('split frames and bounded resynchronization', () => {
  const stream = new Stream();
  assert.equal(stream.push(frame.slice(0, 10)).length, 0);
  assert.equal(stream.push(frame.slice(10))[0].seq, 42);
  assert.equal(stream.push('x'.repeat(400) + '\n' + frame).length, 1);
  assert.equal(stream.errors, 1);
});
test('staleness, duplicate detection and restart inference', () => {
  const session = new Session();
  const r = parse(frame);
  assert.equal(session.accept(r, 0), true);
  assert.equal(session.accept(r, 10), false);
  assert.equal(session.fresh(999), true);
  assert.equal(session.fresh(1001), false);
  assert.equal(session.accept({...r, seq: 0, uptime: 20}, 1100), true);
  assert.equal(session.epoch, 1);
  assert.equal(session.fresh(1100), true);
});
