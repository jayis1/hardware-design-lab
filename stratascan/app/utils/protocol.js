// protocol.js — StrataScan BLE Binary Protocol Definition
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Defines the binary protocol used between the StrataScan firmware and
// this companion app.  All communication uses framed packets:
//
//   [0xAA] [0x55] [CMD] [LEN_L] [LEN_H] [PAYLOAD...] [CRC_L] [CRC_H]
//
// The BLE module (nRF52840) bridges these frames between UART (to the
// STM32H743) and BLE GATT characteristics.
//

// Command opcodes (must match firmware ble.h)
export const CMD = {
  START_SURVEY:    0x01,
  PAUSE_SURVEY:    0x02,
  STOP_SURVEY:     0x03,
  RECALIBRATE:     0x04,
  SET_BAND:        0x05,
  SET_EPS_R:       0x06,
  SET_SPACING:     0x07,
  GET_STATUS:      0x08,
  GET_BSCAN_ROW:   0x09,
  SHUTDOWN:        0x0A,
  // Response opcodes (firmware → app)
  STATUS_RESP:     0x08,
  BSCAN_ROW:       0x10,
  TRACE_READY:     0x11,
  GENERIC_DATA:    0xFF,
};

// Band preset names (must match firmware band_presets[])
export const BAND_PRESETS = [
  { id: 0, name: 'DEEP',  range: '1 MHz – 250 MHz',   depth: '2–30 m',  steps: 512 },
  { id: 1, name: 'LO',    range: '50 MHz – 800 MHz',  depth: '1–15 m',  steps: 512 },
  { id: 2, name: 'MED',   range: '200 MHz – 1.5 GHz', depth: '0.2–4 m', steps: 512 },
  { id: 3, name: 'HI',    range: '500 MHz – 3 GHz',   depth: '0.1–3 m', steps: 1024 },
  { id: 4, name: 'UHI',   range: '1 GHz – 3 GHz',     depth: '0.02–0.5 m', steps: 1024 },
];

// System states (must match firmware sys_state_t)
export const STATES = {
  0: 'BOOT',
  1: 'IDLE',
  2: 'CALIBRATE',
  3: 'SURVEY',
  4: 'PAUSE',
  5: 'SHUTDOWN',
};

// Frame sync bytes
const SYNC0 = 0xAA;
const SYNC1 = 0x55;

// CRC-16 (MODBUS polynomial)
function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let b = 0; b < 8; b++) {
      if (crc & 1) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  return crc & 0xFFFF;
}

// Build a command frame
export function buildFrame(opcode, payload = []) {
  const header = [SYNC0, SYNC1, opcode, payload.length & 0xFF, (payload.length >> 8) & 0xFF];
  const body = header.concat(payload);
  const crc = crc16(body.slice(2));  // CRC over opcode + length + payload
  const frame = body.concat([crc & 0xFF, (crc >> 8) & 0xFF]);
  return new Uint8Array(frame);
}

// Parse a received frame (returns {opcode, payload} or null)
export function parseFrame(buffer) {
  if (buffer.length < 7) return null;  // min frame: 5 header + 2 CRC
  if (buffer[0] !== SYNC0 || buffer[1] !== SYNC1) return null;
  const opcode = buffer[2];
  const len = buffer[3] | (buffer[4] << 8);
  if (buffer.length < 5 + len + 2) return null;
  const payload = buffer.slice(5, 5 + len);
  const crcRecv = buffer[5 + len] | (buffer[5 + len + 1] << 8);
  const crcCalc = crc16(buffer.slice(2, 5 + len));
  if (crcRecv !== crcCalc) return null;  // CRC mismatch
  return { opcode, payload };
}

// Parse a status response payload (18 bytes)
export function parseStatus(payload) {
  if (payload.length < 18) return null;
  const state = payload[0];
  const bandIdx = payload[1];
  const tracesAcquired = payload[2] | (payload[3] << 8) | (payload[4] << 16);
  const batteryPct = payload[6];
  const calibrated = payload[7] !== 0;
  const bscanTraces = payload[8] | (payload[9] << 8);
  // Lat/Lon as signed 32-bit, scaled by 1e5
  const latI = (payload[10]) | (payload[11] << 8) | (payload[12] << 16) | (payload[13] << 24);
  const lonI = (payload[14]) | (payload[15] << 8) | (payload[16] << 16) | (payload[17] << 24);
  const lat = (latI | 0) / 1e5;
  const lon = (lonI | 0) / 1e5;
  return {
    state,
    stateName: STATES[state] || 'UNKNOWN',
    bandIdx,
    bandName: BAND_PRESETS[bandIdx]?.name || '?',
    tracesAcquired,
    batteryPct,
    calibrated,
    bscanTraces,
    lat,
    lon,
  };
}

// Parse a B-scan row payload
export function parseBscanRow(payload) {
  if (payload.length < 4) return null;
  const row = payload[0] | (payload[1] << 8);
  const n = payload[2] | (payload[3] << 8);
  const trace = [];
  const depth = [];
  const traceBytes = Math.min(n * 2, payload.length - 4);
  for (let i = 0; i < n && (4 + i * 2 + 1) < payload.length; i++) {
    const lo = payload[4 + i * 2];
    const hi = payload[4 + i * 2 + 1];
    let val = (hi << 8) | lo;
    if (val & 0x8000) val = val - 0x10000;  // sign-extend
    trace.push(val / 1000.0);  // unscale from int16
  }
  const depthOffset = 4 + n * 2;
  for (let i = 0; i < n && (depthOffset + i * 2 + 1) < payload.length; i++) {
    const lo = payload[depthOffset + i * 2];
    const hi = payload[depthOffset + i * 2 + 1];
    const d_cm = (hi << 8) | lo;
    depth.push(d_cm / 100.0);  // cm → m
  }
  return { row, n, trace, depth };
}

// Color maps for radargram rendering
export const COLOR_MAPS = {
  greyscale: (val, maxVal) => {
    const v = Math.max(0, Math.min(1, Math.abs(val) / maxVal));
    const g = Math.floor(v * 255);
    return `rgb(${g},${g},${g})`;
  },
  jet: (val, maxVal) => {
    const v = Math.max(0, Math.min(1, Math.abs(val) / maxVal));
    let r, g, b;
    if (v < 0.25) { r = 0; g = Math.floor(v * 4 * 255); b = 255; }
    else if (v < 0.5) { r = 0; g = 255; b = Math.floor(255 - (v - 0.25) * 4 * 255); }
    else if (v < 0.75) { r = Math.floor((v - 0.5) * 4 * 255); g = 255; b = 0; }
    else { r = 255; g = Math.floor(255 - (v - 0.75) * 4 * 255); b = 0; }
    return `rgb(${r},${g},${b})`;
  },
  seismic: (val, maxVal) => {
    const v = Math.max(-1, Math.min(1, val / maxVal));
    if (v >= 0) {
      const g = Math.floor(v * 255);
      return `rgb(255,${255 - g},${255 - g})`;
    } else {
      const g = Math.floor(-v * 255);
      return `rgb(${255 - g},${255 - g},255)`;
    }
  },
};