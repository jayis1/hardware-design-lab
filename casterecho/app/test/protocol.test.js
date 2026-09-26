// CasterEcho protocol tests
// Author: jayis1
// Copyright (c) 2026 jayis1
// SPDX-License-Identifier: MIT
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { crc32, encodePacket, decodePacket, COMMAND } = require('../protocol');

test('standard CRC-32 check vector', () => {
  assert.equal(crc32(new TextEncoder().encode('123456789')), 0xcbf43926);
});

test('packet round trip preserves fields', () => {
  const packet = encodePacket(COMMAND.HISTORY, 99, new Uint8Array([4, 0]));
  const decoded = decodePacket(packet);
  assert.equal(decoded.type, COMMAND.HISTORY);
  assert.equal(decoded.sequence, 99);
  assert.deepEqual([...decoded.payload], [4, 0]);
});

test('corruption and malformed payloads fail closed', () => {
  const packet = encodePacket(COMMAND.INFO, 1);
  packet[7] ^= 0x80;
  assert.throws(() => decodePacket(packet), /CRC/);
  assert.throws(() => encodePacket(COMMAND.INFO, 0, new Uint8Array(149)), /148/);
});
