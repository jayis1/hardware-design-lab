/* StitchScope BLE protocol codec
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
(function (global) {
  'use strict';
  const PREAMBLE = [0x53, 0x53];
  const VERSION = 1;
  const SERVICE = '7c310000-9e3d-4d4b-9427-a948a2310001';
  const CONTROL = '7c310001-9e3d-4d4b-9427-a948a2310001';
  const LIVE = '7c310002-9e3d-4d4b-9427-a948a2310001';
  const LOG = '7c310003-9e3d-4d4b-9427-a948a2310001';

  function crc16(bytes) {
    let crc = 0xffff;
    for (const byte of bytes) {
      crc ^= byte << 8;
      for (let bit = 0; bit < 8; bit += 1) {
        crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        crc &= 0xffff;
      }
    }
    return crc;
  }

  function encode(type, sequence, payload = new Uint8Array()) {
    if (payload.length > 234) throw new RangeError('Payload exceeds negotiated frame size');
    const output = new Uint8Array(10 + payload.length);
    const view = new DataView(output.buffer);
    output.set(PREAMBLE, 0); output[2] = VERSION; output[3] = type;
    view.setUint16(4, sequence, true); view.setUint16(6, payload.length, true);
    output.set(payload, 8);
    view.setUint16(8 + payload.length, crc16(output.slice(2, 8 + payload.length)), true);
    return output;
  }

  function decode(input) {
    const bytes = input instanceof Uint8Array ? input : new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
    if (bytes.length < 10 || bytes[0] !== PREAMBLE[0] || bytes[1] !== PREAMBLE[1]) throw new Error('Invalid StitchScope frame');
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const length = view.getUint16(6, true);
    if (bytes[2] !== VERSION || bytes.length !== length + 10) throw new Error('Unsupported or truncated frame');
    const expected = view.getUint16(8 + length, true);
    if (crc16(bytes.slice(2, 8 + length)) !== expected) throw new Error('Frame checksum mismatch');
    return { type: bytes[3], sequence: view.getUint16(4, true), payload: bytes.slice(8, 8 + length) };
  }

  function decodeLive(payload) {
    if (payload.length < 20) throw new Error('Live feature packet is too short');
    const v = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
    return { cycle: v.getUint32(0, true), classification: v.getUint8(4), quality: v.getUint16(5, true) / 4096,
      rate: v.getUint16(7, true), force: v.getInt16(9, true), feed: v.getInt16(11, true) / 1000,
      impulse: v.getUint16(13, true), battery: v.getUint8(15), capabilities: v.getUint16(16, true) };
  }

  global.StitchProtocol = Object.freeze({ SERVICE, CONTROL, LIVE, LOG, crc16, encode, decode, decodeLive });
}(window));
