/**
 * protocol.js — BLE Protocol Parser and Command Builder
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Implements the binary BLE protocol used to communicate with the
 * FloraPulse device.  Frame format:
 *   [SOF=0xAA] [LEN] [OPCODE] [PAYLOAD...] [CRC] [EOF=0x55]
 */

// Frame markers
const SOF = 0xAA;
const EOF = 0x55;

// Command opcodes (sent from app to device)
const CMD = {
  PING: 0x01,
  GET_STATUS: 0x02,
  GET_SAMPLE: 0x03,
  START_STREAM: 0x04,
  STOP_STREAM: 0x05,
  START_CALIB: 0x06,
  GET_CALIB: 0x07,
  SET_RATE: 0x08,
  GET_RATE: 0x09,
  SET_THRESHOLD: 0x0A,
  GET_THRESHOLD: 0x0B,
  DOWNLOAD_LOG: 0x0C,
  ERASE_LOG: 0x0D,
  SET_TIME: 0x0E,
  GET_VERSION: 0x0F,
  GET_WAVEFORM: 0x10,
  TRIGGER_HEATER: 0x11,
};

// Response opcodes (sent from device to app)
const RSP = {
  OK: 0x80,
  ERROR: 0x81,
  DATA: 0x82,
  STREAM: 0x83,
  LOG_CHUNK: 0x84,
  CALIB_DATA: 0x85,
  STATUS: 0x86,
  VERSION: 0x87,
  WAVEFORM: 0x88,
};

// Device modes
const MODE = {
  BOOT: 0,
  IDLE: 1,
  MONITOR: 2,
  LOGGING: 3,
  CALIB: 4,
  STREAM: 5,
  LORA: 6,
  SLEEP: 7,
};

// Event classification codes
const EVENT_CLASS = {
  UNKNOWN: 0,
  DROUGHT: 1,
  WOUNDING: 2,
  LIGHT: 3,
  HERBIVORY: 4,
};

// Log flags
const LOG_FLAGS = {
  HEATER_PULSE: 0x01,
  BLE_CONNECTED: 0x02,
  LORA_TX: 0x04,
  ANOMALY: 0x08,
  SD_FULL: 0x20,
};

/**
 * Compute XOR checksum over a byte array
 */
function computeCRC(data) {
  let crc = 0;
  for (const byte of data) {
    crc ^= byte;
  }
  return crc & 0xFF;
}

/**
 * Build a BLE command frame
 * @param {number} opcode - Command opcode
 * @param {number[]} payload - Array of byte values
 * @returns {number[]} Complete frame bytes
 */
function buildFrame(opcode, payload = []) {
  const len = 1 + payload.length;
  const frame = [SOF, len, opcode, ...payload];
  const crc = computeCRC(frame.slice(1));
  frame.push(crc);
  frame.push(EOF);
  return frame;
}

/**
 * Parse a received BLE frame
 * @param {number[]} frame - Raw frame bytes (including SOF and EOF)
 * @returns {object|null} Parsed frame { opcode, payload } or null if invalid
 */
function parseFrame(frame) {
  if (frame.length < 5) return null;
  if (frame[0] !== SOF) return null;
  if (frame[frame.length - 1] !== EOF) return null;

  const len = frame[1];
  if (frame.length !== len + 4) return null; // SOF + LEN + CRC + EOF

  const opcode = frame[2];
  const payload = frame.slice(3, 3 + len - 1);
  const expectedCrc = computeCRC(frame.slice(1, 1 + len));
  const actualCrc = frame[frame.length - 2];

  if (expectedCrc !== actualCrc) return null;

  return { opcode, payload };
}

/**
 * Parse a status response (8 bytes)
 * @param {number[]} payload
 * @returns {object} Parsed status
 */
function parseStatus(payload) {
  return {
    mode: payload[0],
    batteryPct: payload[1],
    sdPresent: payload[2] !== 0,
    recordCount: (payload[3]) | (payload[4] << 8) | (payload[5] << 16) | (payload[6] << 24),
    sapFlowMeasuring: payload[7] !== 0,
  };
}

/**
 * Parse a sample data response (20 bytes)
 * @param {number[]} payload
 * @returns {object} Parsed sample
 */
function parseSample(payload) {
  const ts = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
  const apCh1 = toInt32(payload, 4);
  const apCh2 = toInt32(payload, 8);
  const tempC = toInt16(payload, 12) / 100.0;
  const rhPct = toUint16(payload, 14) / 100.0;
  const sapFlow = toUint16(payload, 16) / 10.0;
  const battPct = payload[18];
  const flags = payload[19];

  return {
    timestamp: ts,
    apCh1Uv: apCh1,
    apCh2Uv: apCh2,
    tempC: tempC,
    rhPct: rhPct,
    sapFlowVh: sapFlow,
    batteryPct: battPct,
    flags: flags,
    anomaly: (flags & LOG_FLAGS.ANOMALY) !== 0,
    bleConnected: (flags & LOG_FLAGS.BLE_CONNECTED) !== 0,
  };
}

/**
 * Parse a stream data packet (20 bytes)
 * @param {number[]} payload
 * @returns {object} Parsed stream data
 */
function parseStream(payload) {
  return {
    apCh1Uv: toInt32(payload, 0),
    apCh2Uv: toInt32(payload, 4),
    tempC: toInt16(payload, 8) / 100.0,
    rhPct: toUint16(payload, 10) / 100.0,
    sapFlowVh: toUint16(payload, 12) / 10.0,
    leafWetHz: toUint16(payload, 14),
    apRmsUv: toUint16(payload, 16),
    apRateHz: toUint16(payload, 18) / 10.0,
  };
}

/**
 * Parse a full log record (48 bytes)
 * @param {number[]} payload
 * @returns {object} Parsed log record
 */
function parseLogRecord(payload) {
  return {
    timestamp: toUint32(payload, 0),
    apCh1Uv: toInt32(payload, 4),
    apCh2Uv: toInt32(payload, 8),
    tUpperC: toInt32(payload, 12) / 100.0,
    tLowerC: toInt32(payload, 16) / 100.0,
    tempC: toInt16(payload, 20) / 100.0,
    rhPct: toUint16(payload, 22) / 100.0,
    pressurePa: toUint32(payload, 24),
    lux: toUint32(payload, 28),
    leafWetHz: toUint16(payload, 32),
    sapFlowVh: toUint16(payload, 34) / 10.0,
    apRmsUv: toUint16(payload, 36),
    apRateHz: toUint16(payload, 38) / 10.0,
    batteryPct: payload[40],
    flags: payload[41],
  };
}

/**
 * Parse a version response (8 bytes)
 */
function parseVersion(payload) {
  return {
    major: payload[0],
    minor: payload[1],
    patch: payload[2],
    hardwareRev: payload[3],
  };
}

// --- Helper functions for little-endian conversion ---

function toInt32(buf, offset) {
  const val = buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24);
  return val | 0; // Force signed
}

function toUint32(buf, offset) {
  return (buf[offset] | (buf[offset+1] << 8) | (buf[offset+2] << 16) | (buf[offset+3] << 24)) >>> 0;
}

function toInt16(buf, offset) {
  const val = buf[offset] | (buf[offset+1] << 8);
  return val > 0x7FFF ? val - 0x10000 : val;
}

function toUint16(buf, offset) {
  return buf[offset] | (buf[offset+1] << 8);
}

// --- VPD (Vapor Pressure Deficit) calculation ---

/**
 * Compute VPD from temperature and relative humidity.
 * Uses the Tetens formula: es = 0.6108 × exp(17.27 × T / (T + 237.3))
 * VPD = es × (1 - RH/100)
 * @param {number} tempC - Temperature in °C
 * @param {number} rhPct - Relative humidity in %
 * @returns {number} VPD in kPa
 */
function computeVPD(tempC, rhPct) {
  const es = 0.6108 * Math.exp((17.27 * tempC) / (tempC + 237.3));
  return es * (1 - rhPct / 100.0);
}

/**
 * Classify VPD level for plant stress
 * @param {number} vpd - VPD in kPa
 * @returns {string} Stress level description
 */
function classifyVPD(vpd) {
  if (vpd < 0.4) return 'Too low (fog/dew)';
  if (vpd < 0.8) return 'Optimal (vegetative)';
  if (vpd < 1.2) return 'Optimal (generative)';
  if (vpd < 1.6) return 'Elevated';
  if (vpd < 2.0) return 'High stress';
  return 'Severe stress';
}

/**
 * Convert leaf-wetness frequency to percentage
 */
function leafWetToPct(freqHz) {
  const DRY = 10000;
  const WET = 5000;
  if (freqHz >= DRY) return 0;
  if (freqHz <= WET) return 100;
  return Math.round(((DRY - freqHz) / (DRY - WET)) * 100);
}

module.exports = {
  SOF, EOF, CMD, RSP, MODE, EVENT_CLASS, LOG_FLAGS,
  buildFrame, parseFrame, parseStatus, parseSample, parseStream,
  parseLogRecord, parseVersion,
  computeVPD, classifyVPD, leafWetToPct,
  computeCRC,
};