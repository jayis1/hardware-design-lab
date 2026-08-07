/**
 * protocol.js — Binary Protocol Parser/Encoder for FermenTiq BLE
 *
 * Parses the binary live data packet (40 bytes) and configuration
 * packet (56 bytes) exchanged between the firmware and the companion
 * app over BLE GATT characteristics.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

// Fermentation types (must match firmware board.h)
export const FERM_TYPES = {
  BEER: 0,
  WINE: 1,
  CIDER: 2,
  KOMBUCHA: 3,
  YOGURT: 4,
  KEFIR: 5,
  KIMCHI: 6,
  SAUERKRAUT: 7,
  SOURDOUGH: 8,
  CUSTOM: 9,
};

export const FERM_TYPE_LABELS = {
  0: 'Beer',
  1: 'Wine',
  2: 'Cider',
  3: 'Kombucha',
  4: 'Yogurt',
  5: 'Kefir',
  6: 'Kimchi',
  7: 'Sauerkraut',
  8: 'Sourdough',
  9: 'Custom',
};

// Fermentation phases (must match firmware board.h)
export const PHASES = {
  0: 'Idle',
  1: 'Lag',
  2: 'Exponential',
  3: 'Stationary',
  4: 'Decline',
  5: 'Stuck',
  6: 'Spoiled',
  7: 'Unknown',
};

export const PHASE_COLORS = {
  0: '#888888',
  1: '#FFA726',  // orange (lag)
  2: '#66BB6A',  // green (exponential — healthy growth)
  3: '#42A5F5',  // blue (stationary)
  4: '#78909C',  // grey-blue (decline)
  5: '#EF5350',  // red (stuck)
  6: '#D32F2F',  // dark red (spoiled)
  7: '#888888',
};

/**
 * Parse a 40-byte live data packet from base64-encoded BLE value.
 *
 * Packet format (little-endian):
 *   [0-3]   cell_density (float32)
 *   [4-7]   cole_r0 (float32)
 *   [8-9]   co2_ppm (uint16)
 *   [10-13] cer_mmol_lh (float32)
 *   [12-15] co2_dissolved (float32)
 *   [16-19] ph (float32)
 *   [20-23] ph_rate (float32)
 *   [24-27] temp_c (float32)
 *   [28-31] abv (float32)
 *   [32]    phase (uint8)
 *   [33]    spoilage_risk (uint8)
 *   [34]    health_score (uint8)
 *   [35]    batch_age_hours (uint8)
 *   [36-39] bubble_rate (float32)
 */
export function parseLiveData(base64Value) {
  const bytes = base64ToUint8Array(base64Value);
  if (bytes.length < 40) {
    console.warn('Live data packet too short:', bytes.length);
    return null;
  }

  const view = new DataView(bytes.buffer);

  return {
    impedance: {
      cellDensity: view.getFloat32(0, true),
      coleR0: view.getFloat32(4, true),
    },
    co2: {
      ppm: view.getUint16(8, true),
      cerMmolLh: view.getFloat32(10, true),
      dissolved: view.getFloat32(12, true),
    },
    ph: {
      value: view.getFloat32(16, true),
      rate: view.getFloat32(20, true),
    },
    temperature: {
      liquidC: view.getFloat32(24, true),
    },
    fusion: {
      abv: view.getFloat32(28, true),
      phase: bytes[32],
      phaseName: PHASES[bytes[32]] || 'Unknown',
      phaseColor: PHASE_COLORS[bytes[32]] || '#888888',
      spoilageRisk: bytes[33],
      healthScore: bytes[34],
      batchAgeHours: bytes[35],
    },
    acoustic: {
      bubbleRate: view.getFloat32(36, true),
    },
    timestamp: Date.now(),
  };
}

/**
 * Parse a 56-byte configuration packet.
 */
export function parseConfig(base64Value) {
  const bytes = base64ToUint8Array(base64Value);
  if (bytes.length < 54) {
    console.warn('Config packet too short:', bytes.length);
    return null;
  }

  const view = new DataView(bytes.buffer);

  // Extract batch name (bytes 1-31, null-terminated)
  let batchName = '';
  for (let i = 1; i < 32; i++) {
    if (bytes[i] === 0) break;
    batchName += String.fromCharCode(bytes[i]);
  }

  return {
    type: bytes[0],
    typeName: FERM_TYPE_LABELS[bytes[0]] || 'Custom',
    batchName,
    vesselVolume: view.getFloat32(33, true),
    tempMin: view.getFloat32(37, true),
    tempMax: view.getFloat32(41, true),
    phMin: view.getFloat32(45, true),
    phMax: view.getFloat32(49, true),
    active: bytes[53] !== 0,
  };
}

/**
 * Pack a configuration object into a 56-byte base64-encoded BLE value.
 */
export function packConfig(config) {
  const bytes = new Uint8Array(56);
  const view = new DataView(bytes.buffer);

  bytes[0] = config.type || 0;

  // Batch name (max 31 chars)
  const name = config.batchName || '';
  for (let i = 0; i < Math.min(name.length, 31); i++) {
    bytes[1 + i] = name.charCodeAt(i);
  }

  view.setFloat32(33, config.vesselVolume || 19.0, true);
  view.setFloat32(37, config.tempMin || 15.0, true);
  view.setFloat32(41, config.tempMax || 35.0, true);
  view.setFloat32(45, config.phMin || 3.0, true);
  view.setFloat32(49, config.phMax || 7.5, true);
  bytes[53] = config.active ? 1 : 0;

  return uint8ArrayToBase64(bytes);
}

/**
 * Pack a command string into a base64-encoded BLE value.
 * Commands: "start", "stop", "calibrate_ph", "calibrate_co2", "export"
 */
export function packCommand(command) {
  const bytes = new Uint8Array(32);
  for (let i = 0; i < Math.min(command.length, 31); i++) {
    bytes[i] = command.charCodeAt(i);
  }
  return uint8ArrayToBase64(bytes);
}

/**
 * Base64 to Uint8Array converter
 */
function base64ToUint8Array(base64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = base64.replace(/=+$/, '');
  const len = Math.floor(clean.length * 3 / 4);
  const bytes = new Uint8Array(len);
  let byteIdx = 0;

  for (let i = 0; i < clean.length; i += 4) {
    const n = (chars.indexOf(clean[i]) << 18) |
              (chars.indexOf(clean[i+1]) << 12) |
              ((clean[i+2] ? chars.indexOf(clean[i+2]) : 0) << 6) |
              (clean[i+3] ? chars.indexOf(clean[i+3]) : 0);
    if (byteIdx < len) bytes[byteIdx++] = (n >> 16) & 0xFF;
    if (byteIdx < len && clean[i+2]) bytes[byteIdx++] = (n >> 8) & 0xFF;
    if (byteIdx < len && clean[i+3]) bytes[byteIdx++] = n & 0xFF;
  }
  return bytes;
}

/**
 * Uint8Array to base64 converter
 */
function uint8ArrayToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const n = (bytes[i] << 16) |
              ((bytes[i+1] || 0) << 8) |
              (bytes[i+2] || 0);
    result += chars[(n >> 18) & 0x3F];
    result += chars[(n >> 12) & 0x3F];
    result += (i + 1 < bytes.length) ? chars[(n >> 6) & 0x3F] : '=';
    result += (i + 2 < bytes.length) ? chars[n & 0x3F] : '=';
  }
  return result;
}