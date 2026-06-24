/**
 * protocol.js — BLE protocol parser and encoder for WattLens
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Parses binary frames received over BLE from the WattLens device and
 * encodes commands sent to the device.  Frame format matches the firmware
 * BLE driver (ble.c):
 *
 *   [SOF:0xAA] [LEN] [CMD] [PAYLOAD...] [CRC8] [EOF:0x55]
 *
 * Measurement frame (CMD=0x11):
 *   P_total_real(4, int32, 0.1W) | Q(4) | S(4) | PF(2, int16, 0.001)
 *   freq(2, int16, 0.01Hz) | V1(2) | V2(2) | V3(2) | I1(2) | I2(2) | I3(2)
 *   THD_V(1, 0.1%) | THD_I(1, 0.1%)
 */

// GATT service and characteristic UUIDs
export const WATTLENS_SERVICE_UUID    = '0000ff10-1212-ef12-1234-56789abcdef0';
export const CHAR_METRICS_UUID        = '0000ff11-1212-ef12-1234-56789abcdef0';
export const CHAR_HARMONICS_UUID      = '0000ff12-1212-ef12-1234-56789abcdef0';
export const CHAR_NILM_UUID           = '0000ff13-1212-ef12-1234-56789abcdef0';
export const CHAR_EVENT_UUID          = '0000ff14-1212-ef12-1234-56789abcdef0';
export const CHAR_COMMAND_UUID        = '0000ff15-1212-ef12-1234-56789abcdef0';
export const CHAR_MODEL_UPLOAD_UUID   = '0000ff16-1212-ef12-1234-56789abcdef0';
export const CHAR_STATUS_UUID         = '0000ff17-1212-ef12-1234-56789abcdef0';

// Frame markers
const SOF = 0xAA;
const EOF = 0x55;

// CRC-8 (Dallas/Maxim)
function crc8(data, start, length) {
  let crc = 0;
  for (let i = start; i < start + length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) crc = ((crc << 1) ^ 0x07) & 0xFF;
      else crc = (crc << 1) & 0xFF;
    }
  }
  return crc;
}

// ========================================================================
// Decode metrics frame (CMD=0x11)
// ========================================================================
export function decodeMetrics(payload) {
  if (payload.length < 22) return null;

  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  let off = 0;

  const pTotalReal   = dv.getInt32(off, true) / 10.0;       off += 4; // Watts
  const pTotalReactive = dv.getInt32(off, true) / 10.0;     off += 4; // var
  const pTotalApparent = dv.getInt32(off, true) / 10.0;     off += 4; // VA
  const pf           = dv.getInt16(off, true) / 1000.0;     off += 2;
  const freq         = dv.getInt16(off, true) / 100.0;      off += 2; // Hz

  const vRms = [0, 0, 0];
  for (let i = 0; i < 3; i++) { vRms[i] = dv.getUint16(off, true) / 10.0; off += 2; }

  const iRms = [0, 0, 0];
  for (let i = 0; i < 3; i++) { iRms[i] = dv.getUint16(off, true) / 100.0; off += 2; }

  const thdV = dv.getUint8(off++) / 10.0;
  const thdI = dv.getUint8(off++) / 10.0;

  return {
    pTotalReal,
    pTotalReactive,
    pTotalApparent,
    pf,
    freq,
    vRms,
    iRms,
    thdV,
    thdI,
    timestamp: Date.now(),
  };
}

// ========================================================================
// Decode NILM state frame (CMD=0x13)
// ========================================================================
export function decodeNilm(payload) {
  if (payload.length < 2) return null;

  const numClasses = payload[0];
  const results = [];

  for (let c = 0; c < numClasses && c < 16; c++) {
    const base = 2 + c * 2;
    if (base + 1 >= payload.length) break;
    const state = payload[base];        // 0 = off, 1 = on
    const prob  = payload[base + 1] / 255.0;
    results.push({ classId: c, state, prob });
  }

  return { numClasses, appliances: results };
}

// ========================================================================
// Decode harmonics frame (CMD=0x12)
// ========================================================================
export function decodeHarmonics(payload) {
  if (payload.length < 101) return null;

  const phase = payload[0];
  const magnitudes = new Float32Array(50);

  for (let n = 1; n <= 50; n++) {
    const base = 1 + (n - 1) * 2;
    const lo = payload[base];
    const hi = payload[base + 1];
    magnitudes[n - 1] = ((hi << 8) | lo) / 100.0; // 0.01 V units
  }

  return { phase, magnitudes };
}

// ========================================================================
// Decode event frame (CMD=0x14)
// ========================================================================
export function decodeEvent(payload) {
  if (payload.length < 12) return null;

  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const timestamp = dv.getUint32(0, true);
  const type      = payload[4];
  const phase     = payload[5];
  const severity  = dv.getFloat32(6, true);
  const durationMs = dv.getUint16(10, true);

  const typeNames = {
    0: 'NONE', 1: 'SAG', 2: 'SWELL', 3: 'INTERRUPTION',
    4: 'FLICKER', 5: 'HARMONIC_EXCEED', 6: 'NILM_ON', 7: 'NILM_OFF',
  };

  return {
    timestamp,
    type: typeNames[type] || `UNKNOWN(${type})`,
    phase,
    severity,
    durationMs,
  };
}

// ========================================================================
// Decode status frame (CMD=0x17)
// ========================================================================
export function decodeStatus(payload) {
  if (payload.length < 16) return null;

  const dv = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
  const uptime    = dv.getUint32(0, true);
  const battery   = payload[4];
  const sdPresent = payload[5] === 1;
  const bleConn   = payload[6] === 1;
  const loraJoined = payload[7] === 1;
  const errorFlags = dv.getUint16(8, true);
  const sampleCount = dv.getUint32(10, true);
  const numClasses  = payload[14];

  return { uptime, battery, sdPresent, bleConn, loraJoined, errorFlags, sampleCount, numClasses };
}

// ========================================================================
// Encode command frames
// ========================================================================
export function encodeCommand(cmd, payload = []) {
  const len = payload.length;
  const frame = new Uint8Array(5 + len);

  frame[0] = SOF;
  frame[1] = len;
  frame[2] = cmd;
  for (let i = 0; i < len; i++) frame[3 + i] = payload[i];
  frame[3 + len] = crc8(frame, 2, 1 + len);
  frame[3 + len + 1] = EOF;

  return frame;
}

export const Commands = {
  START:        () => encodeCommand(0x01),
  STOP:         () => encodeCommand(0x02),
  SET_CT:       (channel, ratio) => {
    const buf = new ArrayBuffer(5);
    const dv = new DataView(buf);
    dv.setUint8(0, channel);
    dv.setFloat32(1, ratio, true);
    return encodeCommand(0x03, new Uint8Array(buf));
  },
  SET_GRID:     (freq) => encodeCommand(0x04, [freq]),
  CALIBRATE:    () => encodeCommand(0x05),
  GET_HARMONICS: () => encodeCommand(0x06),
  GET_EVENTS:   () => encodeCommand(0x07),
  GET_STATUS:   () => encodeCommand(0x08),
  SET_NAME:     (classId, name) => {
    const nameBytes = new TextEncoder().encode(name).slice(0, 22);
    const payload = new Uint8Array(1 + nameBytes.length);
    payload[0] = classId;
    payload.set(nameBytes, 1);
    return encodeCommand(0x0A, payload);
  },
  CAPTURE:      (durationS) => encodeCommand(0x0B, [durationS]),
};

// Default appliance names (matches firmware nilm.c)
export const APPLIANCE_NAMES = [
  'Refrigerator', 'Washing Machine', 'Dishwasher', 'Microwave',
  'Electric Kettle', 'Air Conditioner', 'Water Heater', 'EV Charger',
  'Lighting', 'Television', 'Computer', 'Pool Pump',
  'Well Pump', 'Oven', 'Dryer', 'Unknown',
];