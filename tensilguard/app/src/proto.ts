/**
 * proto.ts — TensilGuard wire protocol decoder (TypeScript mirror of proto.h)
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Decodes the LoRa uplink packets produced by the firmware into typed
 * JavaScript objects the app can display. All multi-byte fields are
 * little-endian on the wire, matching the C structs in proto.h.
 */

export interface ULHeader {
  magic: number;       // 0x54
  ver: number;
  nodeId: string;      // hex EUI-48
  hops: number;
  rssiDb: number;
  crc16: number;
  seq: number;
  unixTime: number;
}

export interface ULPacket {
  tMagKn: number;      // magnetoelastic tension (kN)
  tVibKn: number;      // vibration tension (kN)
  dtKn: number;        // |tMag - tVib| (kN)
  f1Hz: number;        // cable fundamental frequency (Hz)
  tempC: number;       // temperature (°C)
  muEff: number;       // effective permeability (dimensionless)
  batteryPct: number;
  batteryMv: number;
  panelMv: number;
  accelRmsMg: number;
  intervalS: number;
  resetCause: number;
  urgent: number;
  aeCount: number;
  flags: number;
}

export interface AeRecord {
  unixTime: number;
  peakUv: number;
  riseUs: number;
  durUs: number;
  centroidKhz: number;
  hiLoRatio: number;
  score: number;        // 0..100
  classification: number; // 0 noise, 1 fret, 2 impact, 3 break
  waveform: number[];  // 24 downsampled samples
}

export interface DecodedUplink {
  header: ULHeader;
  packet: ULPacket;
  ae: AeRecord | null;
}

export const AE_NOISE = 0;
export const AE_FRET = 1;
export const AE_IMPACT = 2;
export const AE_BREAK = 3;

export const aeLabel = (c: number): string =>
  ['', 'Fretting', 'Impact', 'Wire Break'][c] ?? 'Unknown';

export const FLAG_URGENT = 0x01;
export const FLAG_DT_ALARM = 0x02;
export const FLAG_AE_ALARM = 0x04;
export const FLAG_LOW_BATT = 0x08;
export const FLAG_CAL_LOST = 0x10;

/** Read N bytes as an unsigned little-endian integer at offset. */
function readLE(buf: Uint8Array, off: number, n: number): number {
  let v = 0;
  for (let i = 0; i < n; i++) v |= buf[off + i] << (8 * i);
  return v >>> 0;
}

function readI16(buf: Uint8Array, off: number): number {
  const v = readLE(buf, off, 2);
  return v > 0x7fff ? v - 0x10000 : v;
}

function hexId(buf: Uint8Array, off: number, n: number): string {
  return Array.from(buf.slice(off, off + n))
    .map(b => b.toString(16).padStart(2, '0'))
    .join(':');
}

/**
 * decodeUplink — parse a raw LoRa uplink into header + payload + optional AE.
 * The layout must match build_uplink() in main.c and proto.h.
 */
export function decodeUplink(raw: Uint8Array): DecodedUplink | null {
  if (raw.length < 18 + 26) return null;
  const magic = raw[0];
  if (magic !== 0x54) return null;

  const header: ULHeader = {
    magic,
    ver: raw[1],
    nodeId: hexId(raw, 2, 6),
    hops: raw[8],
    rssiDb: raw[9] - 100,    // offset +100 on wire
    crc16: readLE(raw, 10, 2),
    seq: readLE(raw, 12, 4),
    unixTime: readLE(raw, 16, 4),
  };

  const pOff = 18;
  const packet: ULPacket = {
    tMagKn: readLE(raw, pOff + 0, 2) / 16,
    tVibKn: readLE(raw, pOff + 2, 2) / 16,
    dtKn: readLE(raw, pOff + 4, 2) / 16,
    f1Hz: readLE(raw, pOff + 6, 2) / 1000,
    tempC: readI16(raw, pOff + 8) / 10,
    muEff: readLE(raw, pOff + 10, 2) / 4096,
    batteryPct: raw[pOff + 12],
    socQ4: readLE(raw, pOff + 13, 2) / 16,
    batteryMv: readLE(raw, pOff + 15, 2),
    panelMv: readLE(raw, pOff + 17, 2),
    accelRmsMg: readI16(raw, pOff + 19),
    intervalS: raw[pOff + 21] * 10,
    resetCause: raw[pOff + 22],
    urgent: raw[pOff + 23],
    aeCount: raw[pOff + 24],
    flags: raw[pOff + 25],
  } as ULPacket;

  let ae: AeRecord | null = null;
  const aeOff = pOff + 26;
  if (raw.length >= aeOff + 38 && packet.aeCount > 0) {
    ae = {
      unixTime: readLE(raw, aeOff, 4),
      peakUv: readLE(raw, aeOff + 4, 2) / 10,
      riseUs: readLE(raw, aeOff + 6, 2),
      durUs: readLE(raw, aeOff + 8, 2),
      centroidKhz: readLE(raw, aeOff + 10, 2),
      hiLoRatio: raw[aeOff + 12] / 16,
      score: raw[aeOff + 13],
      classification: raw[aeOff + 14],
      waveform: Array.from(raw.slice(aeOff + 15, aeOff + 39)),
    };
  }

  return { header, packet, ae };
}

/** Downlink command encoder for the gateway → node path. */
export function encodeSetInterval(seconds: number): Uint8Array {
  const buf = new Uint8Array(8);
  buf[0] = 0x01; // DL_CMD_SET_INTERVAL
  buf[1] = (seconds >> 16) & 0xff;
  buf[2] = (seconds >> 8) & 0xff;
  buf[3] = seconds & 0xff;
  return buf;
}

export function encodeSetAeThreshold(uv: number): Uint8Array {
  const buf = new Uint8Array(8);
  buf[0] = 0x07; // DL_CMD_SET_AE_THRESH
  buf[1] = (uv >> 8) & 0xff;
  buf[2] = uv & 0xff;
  return buf;
}