/**
 * protocol.js — Binary protocol parser/encoder for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the MycoMesh binary command/response protocol used over
 * USB CDC and BLE.  Frame format:
 *   [SYNC 0xA5] [CMD 1B] [LEN 2B LE] [PAYLOAD N B] [CRC8 1B]
 */

// Command opcodes (matching firmware).
export const CMD = {
  GET_STATUS:    0x01,
  START_ACQ:     0x02,
  STOP_ACQ:      0x03,
  GET_SPIKES:    0x04,
  SET_CONFIG:    0x05,
  GET_ENV:       0x06,
  CALIBRATE:     0x07,
  DFU_ENTER:     0x08,
  GET_LOG_LIST:  0x09,
  DOWNLOAD_LOG:  0x0A,
};

export const SYNC_CMD = 0xA5;
export const SYNC_RESP = 0x5A;
export const ACK = 0xA5;
export const NACK = 0x5A;

// CRC-8 polynomial 0x07 (CRC-8/SMBUS).
export function crc8(data) {
  let crc = 0x00;
  for (const byte of data) {
    crc ^= byte;
    for (let b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = ((crc << 1) ^ 0x07) & 0xFF;
      } else {
        crc = (crc << 1) & 0xFF;
      }
    }
  }
  return crc;
}

// Encode a command frame.
export function encodeCommand(opcode, payload = []) {
  const len = payload.length;
  const header = [SYNC_CMD, opcode, len & 0xFF, (len >> 8) & 0xFF];
  const crcData = [...header.slice(1), ...payload];
  const crc = crc8(crcData);
  return [...header, ...payload, crc];
}

// Decode a response frame.
export function decodeResponse(data) {
  if (data.length < 5) return null;
  if (data[0] !== SYNC_RESP) return null;

  const cmd = data[1];
  const len = data[2] | (data[3] << 8);
  const payload = data.slice(4, 4 + len);
  const crc = data[4 + len];

  // Verify CRC.
  const expectedCrc = crc8([data[1], data[2], data[3], ...payload]);
  if (crc !== expectedCrc) {
    return { cmd, error: 'CRC mismatch', payload: null };
  }

  return { cmd, payload, error: null };
}

// Parse a status payload (16 bytes).
export function parseStatusPayload(payload) {
  if (!payload || payload.length < 16) return null;
  return {
    uptime: (payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24)) >>> 0,
    batteryMv: payload[4] | (payload[5] << 8),
    state: payload[6],
    dutyCyclePct: payload[7],
    channels: payload[8],
    classLabel: payload[10],
    sampleRate: payload[14] | (payload[15] << 8),
  };
}

// Parse an environmental data payload (12 bytes).
export function parseEnvPayload(payload) {
  if (!payload || payload.length < 12) return null;
  const readInt16 = (off) => {
    const v = payload[off] | (payload[off + 1] << 8);
    return v > 0x7FFF ? v - 0x10000 : v;
  };
  const readUInt16 = (off) => payload[off] | (payload[off + 1] << 8);
  return {
    moisturePct: readInt16(0) / 10.0,
    tempC: readInt16(2) / 10.0,
    co2Ppm: readUInt16(4),
    humidityPct: payload[6],
    timestamp: (payload[8] | (payload[9] << 8) | (payload[10] << 16) | (payload[11] << 24)) >>> 0,
  };
}

// Build a config command payload.
export function buildConfigPayload(dutyCycle, mainsHz, region) {
  return [dutyCycle & 0xFF, mainsHz & 0xFF, region & 0xFF];
}

// Build a start acquisition command.
export function buildStartAcqPayload(sampleRate, gain, filterConfig) {
  return [sampleRate & 0xFF, gain & 0xFF, filterConfig & 0xFF, 0x00];
}