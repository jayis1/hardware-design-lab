/**
 * proto.ts — wire-protocol mirror of firmware/proto.h for the companion app
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Decodes the compact binary uplink frames the Soilchord probe sends over
 * LoRa into the TypeScript types the UI consumes.
 */

export const NCHORDS = 6;

export const UL_HDR_MAGIC = 0x5C;
export const UL_VER = 1;
export const DL_MAGIC = 0xC5;
export const DL_VER = 1;

export interface ChordPayload {
  f0Hz: number;          // fundamental frequency, Hz
  qFreq: number;         // Q from -3 dB bandwidth
  qTime: number;         // Q from time-domain ring-down
  tempC: number;         // °C
  flags: number;         // per-chord flags
}

export interface UplinkFrame {
  seq: number;
  unixTime: number;
  batteryMv: number;
  solarMv: number;
  intervalS: number;
  resetCause: number;
  urgent: number;
  chords: ChordPayload[];
}

export const UL_FLAG_DEAD_PLUCK = 1 << 0;
export const UL_FLAG_SUSPECT = 1 << 1;
export const UL_FLAG_ALERT = 1 << 2;
export const UL_FLAG_URGENT = 1 << 3;

export function decodeUplink(buf: Uint8Array): UplinkFrame | null {
  if (buf.length < 13) return null;
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  const magic = dv.getUint8(0);
  const ver = dv.getUint8(1);
  if (magic !== UL_HDR_MAGIC || ver !== UL_VER) return null;

  const seq = dv.getUint32(2, true);
  const unixTime = dv.getUint32(6, true);
  const batteryMv = dv.getInt16(10, true);
  const solarMv = dv.getInt16(12, true);
  const intervalS = dv.getUint8(14);
  const resetCause = dv.getUint8(15);
  const urgent = dv.getUint8(16);
  const nchords = dv.getUint8(17);

  const chords: ChordPayload[] = [];
  let off = 18;
  for (let i = 0; i < nchords && off + 6 <= buf.length; i++) {
    const f0q8 = dv.getUint16(off, true); off += 2;
    const qFreqU8 = dv.getUint8(off); off += 1;
    const qTimeU8 = dv.getUint8(off); off += 1;
    const tempU8 = dv.getUint8(off); off += 1;
    const flags = dv.getUint8(off); off += 1;
    chords.push({
      f0Hz: f0q8 / 256,
      qFreq: qFreqU8 / 4,
      qTime: qTimeU8 / 4,
      tempC: tempU8 - 64,
      flags,
    });
  }
  return { seq, unixTime, batteryMv, solarMv, intervalS, resetCause, urgent, chords };
}

export function encodeDownlink(cmd: number, arg: number): Uint8Array {
  const buf = new Uint8Array(7);
  const dv = new DataView(buf.buffer);
  dv.setUint8(0, DL_MAGIC);
  dv.setUint8(1, DL_VER);
  dv.setUint8(2, cmd & 0xff);
  dv.setUint32(3, arg, true);
  return buf;
}

export const DL_CMD = {
  NOP: 0,
  SET_INTERVAL: 1,
  CALIBRATE_BASELINE: 2,
  REQUEST_BACKFILL: 3,
  FORCE_MEASURE: 4,
} as const;