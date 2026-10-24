// Author: jayis1. SPDX-License-Identifier: MIT
export const states = ['CLEAR', 'READY', 'HOLD', 'RELEASE', 'REJECT'];
export const reasons = ['NONE', 'ORDER', 'OVERLAP', 'SHORT_HOLD', 'TIMEOUT', 'SAMPLE_GAP'];
export function crc(text) {
  let value = 0xffff;
  for (const c of text) {
    value ^= c.charCodeAt(0) << 8;
    for (let bit = 0; bit < 8; bit++)
      value = ((value << 1) ^ ((value & 0x8000) ? 0x1021 : 0)) & 0xffff;
  }
  return value;
}
export function parse(line) {
  if (line.length > 256) throw Error('Frame too long');
  const match = /^(FT1(?:,\d+){15})\*([0-9A-F]{4})\n?$/.exec(line);
  if (!match || crc(match[1]) !== Number.parseInt(match[2], 16)) throw Error('Invalid frame or CRC');
  const numbers = match[1].split(',').slice(1).map(Number);
  if (numbers.some(n => !Number.isSafeInteger(n) || n > 0xffffffff)) throw Error('Integer range');
  const [seq, uptime, state, reason, step, mask, good, bad, gaps, duration, d0, d1, d2, d3, loss] = numbers;
  if (state > 4 || reason > 5 || step > 4 || mask > 15) throw Error('Enumeration range');
  return {seq, uptime, state, reason, step, mask, good, bad, gaps, duration, dwell: [d0,d1,d2,d3], loss};
}
export class Stream {
  constructor() { this.buffer = ''; this.dropping = false; this.errors = 0; }
  push(chunk) {
    const records = [];
    for (const c of chunk) {
      if (c === '\n') {
        if (!this.dropping) {
          try { records.push(parse(this.buffer)); } catch { this.errors++; }
        }
        this.buffer = ''; this.dropping = false;
      } else if (!this.dropping) {
        this.buffer += c;
        if (this.buffer.length > 255) {
          this.buffer = ''; this.dropping = true; this.errors++;
        }
      }
    }
    return records;
  }
}
export class Session {
  constructor() { this.last = null; this.received = -Infinity; this.epoch = 0; this.missing = 0; }
  accept(record, now) {
    if (this.last) {
      if (record.uptime < this.last.uptime) {
        this.epoch++; // Reset or uptime wrap: conservatively separate sessions.
      } else {
        const delta = (record.seq - this.last.seq) >>> 0;
        if (!delta || delta > 0x7fffffff) return false;
        this.missing += delta - 1;
      }
    }
    this.last = record; this.received = now;
    return true;
  }
  fresh(now) { return now - this.received <= 1000; }
}
