/**
 * protocol.js — BLE Protocol Parser and Command Builder
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Implements the binary BLE protocol used to communicate with the
 * FerroProbe device.  Frame format:
 *   [SOF=0xAA] [LEN] [OPCODE] [PAYLOAD...] [CRC] [EOF=0x55]
 */

// Frame markers
const SOF = 0xAA;
const EOF = 0x55;

// Command opcodes (sent from app to device)
const CMD = {
  PING: 0x01,
  GET_STATUS: 0x02,
  GET_FIELD: 0x03,
  START_SURVEY: 0x04,
  STOP_SURVEY: 0x05,
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
  STREAM_START: 0x10,
  STREAM_STOP: 0x11,
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
};

// Device modes
const MODE = {
  BOOT: 0,
  IDLE: 1,
  SURVEY: 2,
  CALIB: 3,
  MONITOR: 4,
  CONFIG: 5,
};

// Log flags
const LOG_FLAGS = {
  GPS_FIX: 0x01,
  ANOMALY: 0x02,
  CALIB_OK: 0x04,
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
  const len = 1 + payload.length; // opcode + payload
  const frame = [SOF, len, opcode, ...payload];
  const crc = computeCRC(frame.slice(1)); // CRC over LEN + OPCODE + PAYLOAD
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
  if (frame[0] !== SOF || frame[frame.length - 1] !== EOF) return null;

  const len = frame[1];
  const opcode = frame[2];
  const payloadLen = len - 1; // subtract opcode
  const payload = frame.slice(3, 3 + payloadLen);
  const expectedCRC = frame[frame.length - 2];
  const actualCRC = computeCRC(frame.slice(1, frame.length - 2));

  if (expectedCRC !== actualCRC) {
    console.warn('BLE CRC mismatch:', { expected: expectedCRC, actual: actualCRC });
    return null;
  }

  return { opcode, payload };
}

/**
 * Parse a STATUS response payload
 * @param {number[]} payload - 10-byte status payload
 * @returns {object} Parsed status
 */
function parseStatus(payload) {
  if (payload.length < 10) return null;
  return {
    battery: payload[0],
    gpsFix: payload[1],
    mode: payload[2],
    calibValid: payload[3],
    temperature: payload[4] > 127 ? payload[4] - 256 : payload[4], // signed int8
    recordCount: payload[5] | (payload[6] << 8) | (payload[7] << 16) | (payload[8] << 24),
    reserved: payload[9],
  };
}

/**
 * Parse a STREAM (field measurement) response payload
 * @param {number[]} payload - 24-byte field data
 * @returns {object} Parsed field measurement
 */
function parseFieldData(payload) {
  if (payload.length < 24) return null;

  // Helper: read 32-bit signed little-endian integer from byte array
  const readInt32LE = (buf, offset) => {
    const val = buf[offset] | (buf[offset + 1] << 8) |
                (buf[offset + 2] << 16) | (buf[offset + 3] << 24);
    return val > 0x7FFFFFFF ? val - 0x100000000 : val;
  };

  const bx10 = readInt32LE(payload, 0);   // ×10 = 0.1 nT resolution
  const by10 = readInt32LE(payload, 4);
  const bz10 = readInt32LE(payload, 8);
  const bt10 = readInt32LE(payload, 12);

  return {
    bx: bx10 / 10.0,   // Convert to nT
    by: by10 / 10.0,
    bz: bz10 / 10.0,
    bTotal: bt10 / 10.0,
    fixQuality: payload[16],
    satellites: payload[17],
    flags: payload[18],
    temperature: payload[19] > 127 ? payload[19] - 256 : payload[19],
    timestamp: readInt32LE(payload, 20),
    isAnomaly: (payload[18] & LOG_FLAGS.ANOMALY) !== 0,
    hasGpsFix: (payload[18] & LOG_FLAGS.GPS_FIX) !== 0,
  };
}

/**
 * Parse a CALIB_DATA response payload
 * @param {number[]} payload - 60-byte calibration data
 * @returns {object} Parsed calibration parameters
 */
function parseCalibData(payload) {
  if (payload.length < 60) return null;

  const readFloat32LE = (buf, offset) => {
    const buffer = new ArrayBuffer(4);
    const view = new DataView(buffer);
    view.setUint8(0, buf[offset]);
    view.setUint8(1, buf[offset + 1]);
    view.setUint8(2, buf[offset + 2]);
    view.setUint8(3, buf[offset + 3]);
    return view.getFloat32(0, true); // little-endian
  };

  const offset = [0, 4, 8].map(o => readFloat32LE(payload, o));
  const scale = [12, 16, 20].map(o => readFloat32LE(payload, o));
  const cross = [];
  for (let i = 0; i < 3; i++) {
    cross.push([]);
    for (let j = 0; j < 3; j++) {
      cross[i].push(readFloat32LE(payload, 24 + (i * 3 + j) * 4));
    }
  }

  return { offset, scale, cross };
}

/**
 * Parse a VERSION response payload
 * @param {number[]} payload - 8-byte version data
 * @returns {object} Parsed version info
 */
function parseVersion(payload) {
  if (payload.length < 8) return null;
  return {
    major: payload[0],
    minor: payload[1],
    patch: payload[2],
    hardwareRev: payload[3],
    author: String.fromCharCode(...payload.slice(4, 8)),
  };
}

/**
 * Parse a binary log record (32 bytes)
 * @param {number[]} record - 32-byte log record
 * @returns {object} Parsed survey data point
 */
function parseLogRecord(record) {
  if (record.length < 32) return null;

  const readInt32LE = (buf, offset) => {
    const val = buf[offset] | (buf[offset + 1] << 8) |
                (buf[offset + 2] << 16) | (buf[offset + 3] << 24);
    return val > 0x7FFFFFFF ? val - 0x100000000 : val;
  };

  return {
    timestamp: readInt32LE(record, 0),
    latitude: readInt32LE(record, 4) / 1e7,
    longitude: readInt32LE(record, 8) / 1e7,
    altitude: readInt32LE(record, 12) / 1000, // mm → m
    bx: readInt32LE(record, 16) / 10.0,       // 0.1 nT → nT
    by: readInt32LE(record, 20) / 10.0,
    bz: readInt32LE(record, 24) / 10.0,
    roll: record[28] > 127 ? record[28] - 256 : record[28],
    pitch: record[29] > 127 ? record[29] - 256 : record[29],
    heading: record[30] * 2,                   // ×2 → degrees
    flags: record[31],
    isAnomaly: (record[31] & LOG_FLAGS.ANOMALY) !== 0,
    hasGpsFix: (record[31] & LOG_FLAGS.GPS_FIX) !== 0,
  };
}

/**
 * Build a SET_RATE command
 * @param {number} rate - 0=Fast, 1=Survey, 2=Precision, 3=Ultra
 * @returns {number[]} Frame bytes
 */
function buildSetRate(rate) {
  return buildFrame(CMD.SET_RATE, [rate & 0xFF]);
}

/**
 * Build a SET_THRESHOLD command
 * @param {number} thresholdNt - Threshold in nanotesla
 * @returns {number[]} Frame bytes
 */
function buildSetThreshold(thresholdNt) {
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  view.setInt32(0, thresholdNt, true);
  const payload = Array.from(new Uint8Array(buf));
  return buildFrame(CMD.SET_THRESHOLD, payload);
}

/**
 * Build a START_SURVEY command
 */
function buildStartSurvey() {
  return buildFrame(CMD.START_SURVEY);
}

/**
 * Build a STOP_SURVEY command
 */
function buildStopSurvey() {
  return buildFrame(CMD.STOP_SURVEY);
}

/**
 * Build a START_CALIB command
 */
function buildStartCalib() {
  return buildFrame(CMD.START_CALIB);
}

/**
 * Build a STREAM_START command (real-time monitoring)
 */
function buildStreamStart() {
  return buildFrame(CMD.STREAM_START);
}

/**
 * Build a STREAM_STOP command
 */
function buildStreamStop() {
  return buildFrame(CMD.STREAM_STOP);
}

/**
 * Build a GET_STATUS command
 */
function buildGetStatus() {
  return buildFrame(CMD.GET_STATUS);
}

/**
 * Build a GET_VERSION command
 */
function buildGetVersion() {
  return buildFrame(CMD.GET_VERSION);
}

/**
 * Build a GET_CALIB command
 */
function buildGetCalib() {
  return buildFrame(CMD.GET_CALIB);
}

/**
 * Build a DOWNLOAD_LOG command
 * @param {number} offset - Byte offset into log file
 * @param {number} length - Number of bytes to request
 */
function buildDownloadLog(offset, length) {
  const buf = new ArrayBuffer(8);
  const view = new DataView(buf);
  view.setUint32(0, offset, true);
  view.setUint32(4, length, true);
  const payload = Array.from(new Uint8Array(buf));
  return buildFrame(CMD.DOWNLOAD_LOG, payload);
}

/**
 * Build an ERASE_LOG command
 */
function buildEraseLog() {
  return buildFrame(CMD.ERASE_LOG);
}

/**
 * Build a PING command
 */
function buildPing() {
  return buildFrame(CMD.PING);
}

// Export everything
module.exports = {
  SOF,
  EOF,
  CMD,
  RSP,
  MODE,
  LOG_FLAGS,
  computeCRC,
  buildFrame,
  parseFrame,
  parseStatus,
  parseFieldData,
  parseCalibData,
  parseVersion,
  parseLogRecord,
  buildSetRate,
  buildSetThreshold,
  buildStartSurvey,
  buildStopSurvey,
  buildStartCalib,
  buildStreamStart,
  buildStreamStop,
  buildGetStatus,
  buildGetVersion,
  buildGetCalib,
  buildDownloadLog,
  buildEraseLog,
  buildPing,
};