// CasterEcho BLE packet codec
// Author: jayis1
// Copyright (c) 2026 jayis1
// SPDX-License-Identifier: MIT
'use strict';

const MAGIC = 0x4345;
const COMMAND = Object.freeze({ INFO: 0x01, LATEST: 0x02, HISTORY: 0x03, CALIBRATE: 0x04, CLEAR: 0x05, SET_WHEEL: 0x06, LIVE: 0x07 });
const MESSAGE = Object.freeze({ INFO: 0x81, PASS: 0x82, HISTORY: 0x83, ACK: 0x84, ERROR: 0x85, LIVE: 0x86 });

function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function encodePacket(type, sequence = 0, payload = new Uint8Array()) {
  if (!(payload instanceof Uint8Array) || payload.length > 148) throw new RangeError('payload must be Uint8Array <= 148 bytes');
  const output = new Uint8Array(14 + payload.length);
  const view = new DataView(output.buffer);
  view.setUint16(0, MAGIC, true); view.setUint8(2, 1); view.setUint8(3, type);
  view.setUint16(4, payload.length, true); view.setUint32(6, sequence, true);
  output.set(payload, 10); view.setUint32(10 + payload.length, crc32(output.subarray(0, 10 + payload.length)), true);
  return output;
}

function decodePacket(input) {
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (bytes.length < 14) throw new RangeError('packet too short');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const length = view.getUint16(4, true);
  if (view.getUint16(0, true) !== MAGIC) throw new Error('bad magic');
  if (view.getUint8(2) !== 1) throw new Error('unsupported protocol');
  if (bytes.length !== length + 14) throw new RangeError('length mismatch');
  const expected = view.getUint32(10 + length, true);
  if (crc32(bytes.subarray(0, 10 + length)) !== expected) throw new Error('CRC mismatch');
  return { type: view.getUint8(3), sequence: view.getUint32(6, true), payload: bytes.slice(10, 10 + length) };
}

function parsePass(payload) {
  if (payload.length < 56) throw new RangeError('pass payload truncated');
  const v = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  return {
    sequence: v.getUint32(0, true), startedMs: v.getUint32(4, true), durationMs: v.getUint32(8, true),
    rms: v.getFloat32(12, true), crest: v.getFloat32(16, true), wheelOrder: v.getFloat32(20, true),
    highBand: v.getFloat32(24, true), coastDecay: v.getFloat32(28, true), drag: v.getFloat32(32, true),
    chatter: v.getFloat32(36, true), shock: v.getFloat32(40, true), speed: v.getFloat32(44, true),
    temperature: v.getFloat32(48, true), revolutions: v.getUint16(52, true), flags: v.getUint16(54, true),
    health: payload.length >= 58 ? v.getUint16(56, true) : 0
  };
}

const api = { MAGIC, COMMAND, MESSAGE, crc32, encodePacket, decodePacket, parsePass };
if (typeof module !== 'undefined') module.exports = api;
if (typeof window !== 'undefined') window.CasterEchoProtocol = api;
