/**
 * protocol.js — BLE protocol encoder/decoder for BreathPrint
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Handles encoding/decoding of the binary protocol used between
 * the BreathPrint device and the companion app.
 */

// Metabolic state constants
export const METABOLIC_STATE = {
  BASELINE: 0,
  KETOTIC: 1,
  POST_MEAL: 2,
  POST_EXERCISE: 3,
  GUT_FERMENT: 4,
  UNKNOWN: 5,
};

export const STATE_NAMES = {
  0: 'Baseline',
  1: 'Ketotic',
  2: 'Post-Meal',
  3: 'Post-Exercise',
  4: 'Gut Fermentation',
  5: 'Unknown',
};

export const STATE_COLORS = {
  0: '#4CAF50',   // Green
  1: '#FF9800',   // Orange
  2: '#2196F3',   // Blue
  3: '#9C27B0',   // Purple
  4: '#F44336',   // Red
  5: '#9E9E9E',   // Gray
};

// Breath quality constants
export const BREATH_QUALITY = {
  VALID: 0,
  DEAD_SPACE: 1,
  CONTAMINATED: 2,
  RETRY: 3,
};

export const QUALITY_NAMES = {
  0: 'Valid',
  1: 'Dead Space Air',
  2: 'Contaminated',
  3: 'Retry',
};

// Command opcodes
export const COMMANDS = {
  START_SAMPLE: 0x01,
  CALIBRATE: 0x02,
  SET_CONFIG: 0x03,
  GET_STATUS: 0x04,
  DOWNLOAD_LOG: 0x05,
  OTA_BEGIN: 0x06,
  OTA_DATA: 0x07,
  OTA_END: 0x08,
  SET_TIME: 0x09,
  FACTORY_RESET: 0x0A,
};

// Device states
export const DEVICE_STATE = {
  IDLE: 0,
  WARMUP: 1,
  SAMPLE: 2,
  EXHALE_WAIT: 3,
  ANALYZE: 4,
  RESULT: 5,
  CALIBRATION: 6,
  SLEEP: 7,
  ERROR: 8,
};

export const DEVICE_STATE_NAMES = {
  0: 'Idle',
  1: 'Warming Up',
  2: 'Sampling',
  3: 'Processing',
  4: 'Analyzing',
  5: 'Result Ready',
  6: 'Calibrating',
  7: 'Sleeping',
  8: 'Error',
};

// ========================================================================
// Base64 helpers
// ========================================================================

const B64_CHARS =
  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';

export function bytesToBase64(bytes) {
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b1 = bytes[i] || 0;
    const b2 = bytes[i + 1] || 0;
    const b3 = bytes[i + 2] || 0;
    const e1 = b1 >> 2;
    const e2 = ((b1 & 0x03) << 4) | (b2 >> 4);
    const e3 = ((b2 & 0x0f) << 2) | (b3 >> 6);
    const e4 = b3 & 0x3f;
    result +=
      B64_CHARS[e1] +
      B64_CHARS[e2] +
      (i + 1 < bytes.length ? B64_CHARS[e3] : '=') +
      (i + 2 < bytes.length ? B64_CHARS[e4] : '=');
  }
  return result;
}

export function base64ToBytes(base64) {
  const bytes = [];
  for (let i = 0; i < base64.length; i += 4) {
    const c1 = B64_CHARS.indexOf(base64[i]);
    const c2 = B64_CHARS.indexOf(base64[i + 1]);
    const c3 = B64_CHARS.indexOf(base64[i + 2]);
    const c4 = B64_CHARS.indexOf(base64[i + 3]);
    const bits = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
    bytes.push((bits >> 16) & 0xff);
    if (base64[i + 2] !== '=') bytes.push((bits >> 8) & 0xff);
    if (base64[i + 3] !== '=') bytes.push(bits & 0xff);
  }
  return bytes;
}

// ========================================================================
// Encode a command to send to the device
// ========================================================================

export function encodeCommand(command, data = null) {
  const bytes = [command];
  if (data) {
    for (let i = 0; i < data.length; i++) {
      bytes.push(data[i]);
    }
  }
  return bytesToBase64(bytes);
}

// ========================================================================
// Decode a 64-byte breath result packet
// struct breath_result_t (packed, little-endian)
// ========================================================================

export function decodeBreathResult(base64) {
  const bytes = base64ToBytes(base64);
  if (bytes.length < 56) return null;

  const view = new DataView(new ArrayBuffer(64));
  for (let i = 0; i < bytes.length && i < 64; i++) {
    view.setUint8(i, bytes[i]);
  }

  return {
    timestamp: view.getUint32(0, true),
    metabolicState: view.getUint8(4),
    breathQuality: view.getUint8(5),
    acetonePpm: view.getUint16(6, true) / 100.0,
    h2Ppm: view.getUint16(8, true) / 10.0,
    ch4Ppm: view.getUint16(10, true) / 10.0,
    ethanolPpm: view.getUint16(12, true) / 100.0,
    isoprenePpb: view.getUint16(14, true),
    nh3Ppm: view.getUint16(16, true) / 10.0,
    h2sPpb: view.getUint16(18, true),
    vocIndex: view.getUint16(20, true),
    co2Ppm: view.getUint16(22, true),
    temperature: view.getInt16(24, true) / 10.0,
    humidity: view.getUint16(26, true) / 10.0,
    pressure: view.getUint16(28, true),
    batteryPct: view.getUint8(30),
    sensorHealth: view.getUint8(31),
    stateName: STATE_NAMES[view.getUint8(4)] || 'Unknown',
    stateColor: STATE_COLORS[view.getUint8(4)] || '#9E9E9E',
    qualityName: QUALITY_NAMES[view.getUint8(5)] || 'Unknown',
    isValid: view.getUint8(5) === 0,
  };
}

// ========================================================================
// Decode a 20-byte live sample packet
// ========================================================================

export function decodeSampleData(base64) {
  const bytes = base64ToBytes(base64);
  if (bytes.length < 20) return null;

  const view = new DataView(new ArrayBuffer(20));
  for (let i = 0; i < 20; i++) {
    view.setUint8(i, bytes[i]);
  }

  return {
    index: view.getUint16(0, true),
    bme688_1_gas: view.getUint16(2, true),
    bme688_2_gas: view.getUint16(4, true),
    co2: view.getUint16(6, true),
    ch4: view.getUint16(8, true),
    ethanol: view.getUint16(10, true),
    nh3: view.getUint16(12, true),
    h2s: view.getUint16(14, true),
    h2: view.getUint16(16, true),
    temperature: view.getUint16(18, true) / 100.0,
  };
}

// ========================================================================
// Format helpers
// ========================================================================

export function formatPpm(value) {
  if (value < 1) return `${(value * 1000).toFixed(0)} ppb`;
  return `${value.toFixed(2)} ppm`;
}

export function formatTimestamp(ts) {
  const date = new Date(ts * 1000);
  return date.toLocaleString();
}

export function formatTime(ts) {
  const date = new Date(ts * 1000);
  return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

export function formatDuration(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  if (h > 0) return `${h}h ${m}m ago`;
  if (m > 0) return `${m}m ago`;
  return 'just now';
}

// ========================================================================
// Get ketosis level from acetone
// ========================================================================

export function getKetosisLevel(acetonePpm) {
  if (acetonePpm < 0.5) return { level: 'None', color: '#9E9E9E' };
  if (acetonePpm < 2.0) return { level: 'Light', color: '#4CAF50' };
  if (acetonePpm < 5.0) return { level: 'Moderate', color: '#FF9800' };
  if (acetonePpm < 10.0) return { level: 'Deep', color: '#F44336' };
  return { level: 'Very Deep', color: '#9C27B0' };
}

// ========================================================================
// Get gut fermentation indicator from H2 and CH4
// ========================================================================

export function getGutFermentationLevel(h2Ppm, ch4Ppm) {
  const score = h2Ppm + ch4Ppm * 2;
  if (score < 5) return { level: 'Normal', color: '#4CAF50' };
  if (score < 20) return { level: 'Mild', color: '#FF9800' };
  if (score < 50) return { level: 'Moderate', color: '#F44336' };
  return { level: 'High', color: '#9C27B0' };
}

// ========================================================================
// Get VOC air quality category
// ========================================================================

export function getVocCategory(vocIndex) {
  if (vocIndex < 50) return { category: 'Excellent', color: '#4CAF50' };
  if (vocIndex < 100) return { category: 'Good', color: '#8BC34A' };
  if (vocIndex < 150) return { category: 'Moderate', color: '#FF9800' };
  if (vocIndex < 200) return { category: 'Poor', color: '#FF5722' };
  return { category: 'Bad', color: '#F44336' };
}