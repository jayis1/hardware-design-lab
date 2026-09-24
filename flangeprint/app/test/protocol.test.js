/* FlangePrint protocol tests.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const {crc16, command, simulatedStatus} = require('../protocol');
test('CRC-16/CCITT-FALSE reference vector', () =>
  assert.equal(crc16(Buffer.from('123456789')), 0x29b1));
test('command appends little-endian CRC', () => {
  const packet = command(3, [1, 2]);
  assert.equal(packet.length, 5);
  assert.equal(packet[3] | packet[4] << 8, crc16(packet.slice(0, 3)));
});
test('simulator closes flange and keeps eight sectors', () => {
  const early = simulatedStatus(0);
  const late = simulatedStatus(50);
  assert.equal(early.gaps.length, 8);
  assert.ok(late.meanGap < early.meanGap);
  assert.ok(late.suggestedSector >= 0 && late.suggestedSector < 8);
});
