/**
 * protocol.js — Vortex-SDR BLE wire protocol definitions
 * Must match firmware command definitions in board.h
 */

// BLE Service and Characteristic UUIDs
export const VORTEX_SDR_SERVICE_UUID = '0000FEF1-0000-1000-8000-00805F9B34FB';
export const VORTEX_SDR_CMD_CHAR_UUID = '0000FEF2-0000-1000-8000-00805F9B34FB';
export const VORTEX_SDR_DATA_CHAR_UUID = '0000FEF3-0000-1000-8000-00805F9B34FB';
export const VORTEX_SDR_STATUS_CHAR_UUID = '0000FEF4-0000-1000-8000-00805F9B34FB';
export const VORTEX_SDR_CONFIG_CHAR_UUID = '0000FEF5-0000-1000-8000-00805F9B34FB';

// Command opcodes (must match board.h CMD_* definitions)
export const CMD_GET_STATUS = 0x01;
export const CMD_START_SWEEP = 0x02;
export const CMD_STOP_SWEEP = 0x03;
export const CMD_SET_FREQ = 0x04;
export const CMD_SET_FFT_SIZE = 0x05;
export const CMD_SET_WINDOW = 0x06;
export const CMD_SET_AVERAGING = 0x07;
export const CMD_SET_ATTEN = 0x08;
export const CMD_SET_AGC = 0x09;
export const CMD_GET_PEAKS = 0x0A;
export const CMD_SET_MODE = 0x0B;
export const CMD_LOG_START = 0x0C;
export const CMD_LOG_STOP = 0x0D;
export const CMD_LOG_READ = 0x0E;
export const CMD_RESET = 0x0F;
export const CMD_SET_REF_LEVEL = 0x10;
export const CMD_SET_SCALE = 0x11;

// Response codes
export const RSP_OK = 0x00;
export const RSP_ERROR = 0x01;
export const RSP_BUSY = 0x02;
export const RSP_INVALID_PARAM = 0x03;

// System state codes (must match board.h sys_state_t)
export const SYS_STATE_INIT = 0;
export const SYS_STATE_IDLE = 1;
export const SYS_STATE_SWEEP_CONTINUOUS = 2;
export const SYS_STATE_SWEEP_SINGLE = 3;
export const SYS_STATE_ZERO_SPAN = 4;
export const SYS_STATE_DATA_LOG = 5;
export const SYS_STATE_USB_TRANSFER = 6;
export const SYS_STATE_ERROR = 7;
export const SYS_STATE_SLEEP = 8;

// Sweep mode codes (must match board.h sweep_mode_t)
export const SWEEP_MODE_CONTINUOUS = 0;
export const SWEEP_MODE_SINGLE = 1;
export const SWEEP_MODE_ZERO_SPAN = 2;
export const SWEEP_MODE_MAX_HOLD = 3;
export const SWEEP_MODE_AVERAGE = 4;

// Window function codes (must match board.h window_t)
export const WINDOW_RECTANGULAR = 0;
export const WINDOW_HANN = 1;
export const WINDOW_BLACKMAN_HARRIS = 2;
export const WINDOW_FLAT_TOP = 3;
export const WINDOW_KAISER = 4;

// Data message types (from device to app)
export const MSG_TYPE_SPECTRUM = 0x10;
export const MSG_TYPE_STATUS = 0x11;
export const MSG_TYPE_PEAKS = 0x12;
export const MSG_TYPE_LOG_DATA = 0x13;
export const MSG_TYPE_ERROR = 0xFE;

// Protocol packet structure:
// [0]     : Message type / Command opcode
// [1-2]   : Payload length (little-endian, 0-512 bytes)
// [3-4]   : Sequence number (little-endian)
// [5+]    : Payload data
//
// Maximum BLE MTU: 512 bytes (negotiated)
// Maximum payload: 507 bytes

// Packet header size
export const HEADER_SIZE = 5;

// Maximum payload sizes
export const MAX_BLE_PAYLOAD = 507;
export const MAX_SPECTRUM_BINS = 8192;

/**
 * Build a command packet for BLE transmission
 * @param {number} cmd - Command opcode
 * @param {Uint8Array} payload - Optional payload data
 * @param {number} seqNum - Sequence number
 * @returns {Uint8Array} Complete packet
 */
export function buildCommandPacket(cmd, payload = null, seqNum = 0) {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = HEADER_SIZE + payloadLen;
  const packet = new Uint8Array(totalLen);

  packet[0] = cmd;
  packet[1] = (payloadLen >> 8) & 0xFF;
  packet[2] = payloadLen & 0xFF;
  packet[3] = (seqNum >> 8) & 0xFF;
  packet[4] = seqNum & 0xFF;

  if (payload && payloadLen > 0) {
    packet.set(payload, HEADER_SIZE);
  }

  return packet;
}

/**
 * Parse a response packet from the device
 * @param {Uint8Array} data - Raw BLE data
 * @returns {Object} Parsed response
 */
export function parseResponse(data) {
  if (data.length < HEADER_SIZE) {
    return { type: 'error', error: 'Packet too short' };
  }

  const msgType = data[0];
  const payloadLen = (data[1] << 8) | data[2];
  const seqNum = (data[3] << 8) | data[4];
  const payload = data.slice(HEADER_SIZE);

  return {
    type: msgType,
    seqNum,
    payloadLen,
    payload,
  };
}

/**
 * Parse spectrum data payload
 * Payload format:
 *   [0-1]   : Number of bins (uint16, little-endian)
 *   [2-9]   : Center frequency (uint64, little-endian, Hz)
 *   [10-13] : Bandwidth per bin (uint32, little-endian, Hz)
 *   [14-15] : Flags (uint16)
 *   [16-17] : Timestamp (uint16, ms, low 16 bits)
 *   [18+]   : Bin data (int16, dBm * 10, little-endian)
 */
export function parseSpectrumPayload(payload) {
  if (payload.length < 18) return null;

  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);

  const numBins = view.getUint16(0, true);
  const centerFreq = Number(view.getBigUint64(2, true));
  const binBw = view.getUint32(10, true);
  const flags = view.getUint16(14, true);
  const timestamp = view.getUint16(16, true);

  const bins = new Int16Array(numBins);
  const binOffset = 18;
  for (let i = 0; i < numBins; i++) {
    if (binOffset + i * 2 + 2 <= payload.length) {
      bins[i] = view.getInt16(binOffset + i * 2, true);
    }
  }

  return {
    numBins,
    centerFreq,
    binBw,
    flags,
    timestamp,
    bins,
  };
}

/**
 * Parse status payload
 * Payload format:
 *   [0]     : System state (uint8)
 *   [1-8]   : Start frequency (uint64, Hz)
 *   [9-16]  : Stop frequency (uint64, Hz)
 *   [17-18] : FFT size (uint16)
 *   [19-22] : Sweep count (uint32)
 *   [23-24] : Battery voltage (uint16, mV)
 *   [25-26] : Reference level (int16, dBm * 10)
 *   [27-28] : Scale (int16, dB/div * 10)
 *   [29]    : PLL locked (uint8, 0/1)
 *   [30]    : ADC overrange (uint8, 0/1)
 *   [31]    : Temperature (int8, °C)
 *   [32-33] : Attenuation (uint16, dB)
 *   [34]    : Window function (uint8)
 *   [35]    : Sweep mode (uint8)
 *   [36]    : Averaging (uint8)
 */
export function parseStatusPayload(payload) {
  if (payload.length < 37) return null;

  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);

  return {
    state: view.getUint8(0),
    startFreq: Number(view.getBigUint64(1, true)),
    stopFreq: Number(view.getBigUint64(9, true)),
    fftSize: view.getUint16(17, true),
    sweepCount: view.getUint32(19, true),
    batteryMv: view.getUint16(23, true),
    refLevel: view.getInt16(25, true) / 10.0,  // dBm
    scale: view.getInt16(27, true) / 10.0,       // dB/div
    pllLocked: view.getUint8(29) !== 0,
    adcOverrange: view.getUint8(30) !== 0,
    temperature: view.getInt8(31),
    attenuation: view.getUint16(32, true),
    window: view.getUint8(34),
    sweepMode: view.getUint8(35),
    averaging: view.getUint8(36),
  };
}

/**
 * Parse peak data payload
 * Payload format:
 *   [0]     : Number of peaks (uint8)
 *   [1+]    : Peak data (10 bytes each):
 *     [0-7]   : Frequency (uint64, Hz)
 *     [8-9]   : Power (int16, dBm * 10)
 */
export function parsePeakPayload(payload) {
  if (payload.length < 1) return null;

  const numPeaks = payload[0];
  const view = new DataView(payload.buffer, payload.byteOffset + 1, payload.byteLength - 1);
  const peaks = [];

  for (let i = 0; i < numPeaks && i * 10 + 10 <= view.byteLength; i++) {
    peaks.push({
      freq: Number(view.getBigUint64(i * 10, true)),
      power: view.getInt16(i * 10 + 8, true) / 10.0,
    });
  }

  return { numPeaks, peaks };
}

/**
 * Build frequency command payload
 * @param {number} startHz - Start frequency in Hz
 * @param {number} stopHz - Stop frequency in Hz
 * @returns {Uint8Array} Payload bytes
 */
export function buildFreqPayload(startHz, stopHz) {
  const payload = new Uint8Array(16);
  const view = new DataView(payload.buffer);
  view.setBigUint64(0, BigInt(startHz), true);
  view.setBigUint64(8, BigInt(stopHz), true);
  return payload;
}

/**
 * Build FFT size command payload
 * @param {number} fftSize - FFT size (256-8192)
 * @returns {Uint8Array} Payload bytes
 */
export function buildFftSizePayload(fftSize) {
  const payload = new Uint8Array(4);
  const view = new DataView(payload.buffer);
  view.setUint32(0, fftSize, true);
  return payload;
}

/**
 * Calculate CRC-16 for packet integrity
 * Polynomial: 0x1021 (CCITT)
 * @param {Uint8Array} data - Data to checksum
 * @returns {number} CRC-16 value
 */
export function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i] << 8;
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
      crc &= 0xFFFF;
    }
  }
  return crc;
}

/**
 * Verify packet integrity using CRC
 * Last 2 bytes of packet are CRC-16
 * @param {Uint8Array} packet - Complete packet including CRC
 * @returns {boolean} True if CRC matches
 */
export function verifyPacketCrc(packet) {
  if (packet.length < HEADER_SIZE + 2) return false;
  const data = packet.slice(0, packet.length - 2);
  const receivedCrc = (packet[packet.length - 2] << 8) | packet[packet.length - 1];
  const calculatedCrc = crc16(data);
  return receivedCrc === calculatedCrc;
}

/**
 * Format frequency for human-readable display
 * @param {number} hz - Frequency in Hz
 * @returns {string} Formatted frequency string
 */
export function formatFrequency(hz) {
  if (hz >= 1e9) return `${(hz / 1e9).toFixed(3)} GHz`;
  if (hz >= 1e6) return `${(hz / 1e6).toFixed(3)} MHz`;
  if (hz >= 1e3) return `${(hz / 1e3).toFixed(1)} kHz`;
  return `${hz} Hz`;
}

/**
 * Format power for human-readable display
 * @param {number} dBm - Power in dBm (may be dBm*10 from firmware)
 * @param {boolean} scaled - Whether the value is already scaled (dBm*10)
 * @returns {string} Formatted power string
 */
export function formatPower(dBm, scaled = false) {
  const value = scaled ? dBm / 10.0 : dBm;
  if (value >= 0) return `+${value.toFixed(1)} dBm`;
  return `${value.toFixed(1)} dBm`;
}

export default {
  VORTEX_SDR_SERVICE_UUID,
  VORTEX_SDR_CMD_CHAR_UUID,
  VORTEX_SDR_DATA_CHAR_UUID,
  VORTEX_SDR_STATUS_CHAR_UUID,
  VORTEX_SDR_CONFIG_CHAR_UUID,
  CMD_GET_STATUS,
  CMD_START_SWEEP,
  CMD_STOP_SWEEP,
  CMD_SET_FREQ,
  CMD_SET_FFT_SIZE,
  CMD_SET_WINDOW,
  CMD_SET_AVERAGING,
  CMD_SET_ATTEN,
  CMD_SET_AGC,
  CMD_GET_PEAKS,
  CMD_SET_MODE,
  CMD_LOG_START,
  CMD_LOG_STOP,
  CMD_LOG_READ,
  CMD_RESET,
  CMD_SET_REF_LEVEL,
  CMD_SET_SCALE,
  buildCommandPacket,
  parseResponse,
  parseSpectrumPayload,
  parseStatusPayload,
  parsePeakPayload,
  buildFreqPayload,
  buildFftSizePayload,
  crc16,
  verifyPacketCrc,
  formatFrequency,
  formatPower,
};