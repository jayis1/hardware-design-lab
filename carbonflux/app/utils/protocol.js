/**
 * protocol.js — BLE / serial data protocol decoder for CarbonFlux.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Parses binary flux records (32-byte) and USB CDC command responses.
 */

/**
 * Parse a 32-byte binary flux record into a JavaScript object.
 * @param {ArrayBuffer} buffer — 32-byte binary record
 * @returns {Object} Parsed flux data
 */
export function parseFluxRecord(buffer) {
  const dv = new DataView(buffer);
  return {
    timestamp: dv.getUint32(0, true),           // Little-endian
    co2_ppm: dv.getInt32(4, true) / 1000.0,
    flux_umol: dv.getInt32(8, true) / 1000.0,
    soil_temp_5cm: dv.getInt16(12, true) / 100.0,
    soil_temp_15cm: dv.getInt16(14, true) / 100.0,
    soil_temp_30cm: dv.getInt16(16, true) / 100.0,
    vwc_pct: dv.getUint16(18, true) / 100.0,
    pressure_hpa: dv.getUint16(20, true) / 10.0,
    air_temp_c: dv.getInt16(22, true) / 100.0,
    par_umol: dv.getUint16(24, true),
    battery_soc: dv.getUint8(26),
    flags: dv.getUint8(27),
    timestampStr: new Date(dv.getUint32(0, true) * 1000).toISOString(),
  };
}

/**
 * Parse a STATUS response from USB CDC serial.
 * Format: "OK <state>:<co2_ppm>:<temp_c>:<flux_umol>:<battery_soc>:<measurement_count>"
 * @param {string} response — Raw serial response
 * @returns {Object|null} Parsed device status
 */
export function parseStatusResponse(response) {
  if (!response || !response.startsWith('OK ')) return null;
  const parts = response.substring(3).split(':');
  if (parts.length < 6) return null;
  return {
    state: parseInt(parts[0], 10),
    co2_ppm: parseFloat(parts[1]),
    air_temp_c: parseFloat(parts[2]),
    flux_umol: parseFloat(parts[3]),
    battery_soc: parseInt(parts[4], 10),
    measurement_count: parseInt(parts[5], 10),
  };
}

/**
 * Parse CONFIG GET response.
 * Format: "OK interval=<val> accum=<val>"
 * @param {string} response
 * @returns {Object|null}
 */
export function parseConfigResponse(response) {
  if (!response || !response.startsWith('OK ')) return null;
  const config = {};
  const parts = response.substring(3).split(' ');
  for (const part of parts) {
    const kv = part.split('=');
    if (kv.length === 2) {
      config[kv[0]] = parseInt(kv[1], 10);
    }
  }
  return config;
}

/**
 * Encode a configuration command for USB CDC.
 * @param {string} key — Config key (e.g., 'interval', 'accum')
 * @param {number} value — Integer value
 * @returns {string} Command string
 */
export function encodeConfigCommand(key, value) {
  return `CONFIG SET ${key}=${value}\n`;
}

/**
 * Encode a STATUS query command.
 * @returns {string}
 */
export function encodeStatusCommand() {
  return 'STATUS\n';
}

/**
 * Encode a calibration command.
 * @param {'zero'|'span'} type
 * @param {number} [referencePpm] — For span calibration
 * @returns {string}
 */
export function encodeCalCommand(type, referencePpm) {
  if (type === 'zero') return 'CAL ZERO\n';
  if (type === 'span' && referencePpm) return `CAL SPAN ${referencePpm}\n`;
  return `CAL ${type.toUpperCase()}\n`;
}

/**
 * Encode a LOG DUMP command.
 * @returns {string}
 */
export function encodeLogDumpCommand() {
  return 'LOG DUMP\n';
}