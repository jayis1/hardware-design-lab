// CondenScope wire protocol codec
// Author: jayis1
// SPDX-License-Identifier: MIT

export const MAGIC = 0x4353;
export const VERSION = 1;
export const Types = Object.freeze({
  HELLO: 0x01,
  STATUS: 0x02,
  THERMAL: 0x03,
  MAP_POINT: 0x04,
  SUMMARY: 0x05,
  START: 0x40,
  STOP: 0x41,
  SET_EMISSIVITY: 0x42,
  SET_THRESHOLDS: 0x43,
  CALIBRATE: 0x44,
  ERASE: 0x45,
  ACK: 0x7e,
  ERROR: 0x7f,
});

export function crc16(bytes) {
  let crc = 0xffff;
  for (const value of bytes) {
    crc ^= value;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc & 1) ? ((crc >>> 1) ^ 0xa001) : (crc >>> 1);
    }
  }
  return crc & 0xffff;
}

const u16 = (b, i) => b[i] | (b[i + 1] << 8);
const s16 = (b, i) => {
  const value = u16(b, i);
  return value > 0x7fff ? value - 0x10000 : value;
};
const u32 = (b, i) => (b[i] | (b[i + 1] << 8) | (b[i + 2] << 16) |
  (b[i + 3] << 24)) >>> 0;
const push16 = (array, value) => array.push(value & 255, (value >>> 8) & 255);

export function encode(type, payload = [], sequence = 0) {
  const bytes = [];
  push16(bytes, MAGIC);
  bytes.push(VERSION, type);
  push16(bytes, sequence);
  push16(bytes, payload.length);
  bytes.push(...payload);
  push16(bytes, crc16(bytes));
  return Uint8Array.from(bytes);
}

export function parseFrame(input) {
  const b = input instanceof Uint8Array ? input : Uint8Array.from(input);
  if (b.length < 10 || u16(b, 0) !== MAGIC || b[2] !== VERSION) {
    throw new Error('Invalid CondenScope frame header');
  }
  const length = u16(b, 6);
  if (b.length !== length + 10) throw new Error('Frame length mismatch');
  if (crc16(b.slice(0, b.length - 2)) !== u16(b, b.length - 2)) {
    throw new Error('Frame CRC mismatch');
  }
  return { type: b[3], sequence: u16(b, 4), payload: b.slice(8, 8 + length) };
}

export function decodeMessage(frame) {
  const p = frame.payload;
  if (frame.type === Types.STATUS) {
    return {
      kind: 'status', airC: s16(p, 0) / 100, humidity: u16(p, 2) / 100,
      dewC: s16(p, 4) / 100, distanceMm: u16(p, 6), batteryMv: u16(p, 8),
      battery: p[10], state: p[11], uptimeMs: u32(p, 12),
    };
  }
  if (frame.type === Types.THERMAL) {
    const ambient = s16(p, 4) / 100;
    const pixels = [];
    let cursor = 12;
    while (cursor < p.length && pixels.length < 768) {
      if (p[cursor] === 0x80) {
        pixels.push(s16(p, cursor + 1) / 100);
        cursor += 3;
      } else {
        const delta = p[cursor] > 127 ? p[cursor] - 256 : p[cursor];
        pixels.push(ambient + delta / 10);
        cursor += 1;
      }
    }
    return { kind: 'thermal', sequence: u32(p, 0), ambient,
      minC: s16(p, 6) / 100, maxC: s16(p, 8) / 100,
      subpage: p[10], complete: p[11] === 1, pixels };
  }
  if (frame.type === Types.MAP_POINT) {
    return { kind: 'point', pixel: u16(p, 0), surfaceC: s16(p, 2) / 100,
      marginC: s16(p, 4) / 100, risk: p[6], distanceMm: u16(p, 7),
      bearing: s16(p, 9) / 100, elevation: s16(p, 11) / 100 };
  }
  if (frame.type === Types.SUMMARY) {
    return { kind: 'summary', durationMs: u32(p, 0), samples: u32(p, 4),
      dangerPoints: u32(p, 8), minimumMargin: s16(p, 12) / 100,
      coldestPixel: u16(p, 14), battery: p[16] };
  }
  return { kind: frame.type === Types.ERROR ? 'error' : 'ack', payload: [...p] };
}

export const commands = {
  start: sequence => encode(Types.START, [], sequence),
  stop: sequence => encode(Types.STOP, [], sequence),
  emissivity: (value, sequence) => {
    const p = []; push16(p, Math.round(value * 32768));
    return encode(Types.SET_EMISSIVITY, p, sequence);
  },
  thresholds: (warningC, dangerC, sequence) => {
    const p = []; push16(p, Math.round(warningC * 100) & 0xffff);
    push16(p, Math.round(dangerC * 100) & 0xffff);
    return encode(Types.SET_THRESHOLDS, p, sequence);
  },
};
