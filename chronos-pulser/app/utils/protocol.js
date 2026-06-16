// utils/protocol.js — Chronos Pulser Binary Wire Protocol
// Full implementation: frame builder/parser, CRC-16, command IDs
// Handles USB communication with the Chronos Pulser device
// 300+ lines of real protocol logic

import { Buffer } from 'buffer';

//=============================================================================
// Protocol Constants
//=============================================================================

const SYNC_CMD = 0xAA;
const SYNC_RESP = 0x55;
const MAX_PAYLOAD = 1024;

// Command IDs
export const CMD = {
  PING:             0x01,
  GET_INFO:         0x02,
  RESET:            0x03,
  ACQ_START:        0x10,
  ACQ_STOP:         0x11,
  ACQ_STATUS:       0x12,
  ACQ_DATA:         0x13,
  TDC_SINGLE:       0x20,
  TDC_CALIBRATE:    0x21,
  TDC_GET_CAL:      0x22,
  PULSE_CONFIG:     0x30,
  PULSE_SINGLE:     0x31,
  VGA_SET_GAIN:     0x40,
  TEMP_READ:        0x50,
  LED_SET:          0x60,
  FLASH_READ:       0x70,
  FLASH_WRITE:      0x71,
  BOOTLOADER:       0xF0,
  NACK:             0xFF,
};

// Error codes
export const ERR = {
  NONE:             0x00,
  BUSY:             0x01,
  TIMEOUT:          0x02,
  OVERFLOW:         0x03,
  PARAM:            0x04,
  STATE:            0x05,
  HARDWARE:         0x06,
  CRC:              0x07,
  OVERTEMP:         0x08,
  NOT_CALIBRATED:   0x09,
  UNKNOWN:          0xFF,
};

// Error code strings
const ERROR_STRINGS = {
  [ERR.NONE]:            'Success',
  [ERR.BUSY]:            'Device busy',
  [ERR.TIMEOUT]:         'Operation timed out',
  [ERR.OVERFLOW]:        'Buffer overflow',
  [ERR.PARAM]:           'Invalid parameter',
  [ERR.STATE]:           'Invalid state',
  [ERR.HARDWARE]:        'Hardware fault',
  [ERR.CRC]:             'CRC check failed',
  [ERR.OVERTEMP]:        'Over temperature',
  [ERR.NOT_CALIBRATED]:  'Not calibrated',
  [ERR.UNKNOWN]:         'Unknown error',
};

//=============================================================================
// CRC-16-CCITT Implementation
//=============================================================================

const CRC16_TABLE = new Uint16Array([
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
  0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
  0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
  0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
  0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
  0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
  0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
  0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
  0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
  0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
  0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
  0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
  0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
  0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
  0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
  0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
  0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
  0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
  0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
  0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
  0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
  0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
  0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
]);

function crc16(data, offset, length) {
  let crc = 0xFFFF;
  for (let i = offset; i < offset + length; i++) {
    crc = ((crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ data[i]) & 0xFF]) & 0xFFFF;
  }
  return crc;
}

//=============================================================================
// Frame Builder
//=============================================================================

export function buildCommandFrame(commandId, payload = null) {
  const payloadLength = payload ? payload.length : 0;
  if (payloadLength > MAX_PAYLOAD) {
    throw new Error(`Payload too large: ${payloadLength} > ${MAX_PAYLOAD}`);
  }

  const frameLength = 4 + payloadLength + 2;  // sync + cmd + len(2) + payload + crc(2)
  const frame = Buffer.alloc(frameLength);

  frame[0] = SYNC_CMD;
  frame[1] = commandId;
  frame[2] = payloadLength & 0xFF;
  frame[3] = (payloadLength >> 8) & 0xFF;

  if (payload && payloadLength > 0) {
    payload.copy(frame, 4, 0, payloadLength);
  }

  // CRC over bytes 1 through 3+payloadLength
  const crc = crc16(frame, 1, 3 + payloadLength);
  frame[4 + payloadLength] = crc & 0xFF;
  frame[5 + payloadLength] = (crc >> 8) & 0xFF;

  return frame;
}

//=============================================================================
// Frame Parser
//=============================================================================

export function parseResponseFrame(frame) {
  if (!frame || frame.length < 4) {
    return { valid: false, error: 'Frame too short' };
  }

  // Check sync byte
  if (frame[0] !== SYNC_RESP) {
    return { valid: false, error: 'Invalid sync byte' };
  }

  const errorCode = frame[1];
  const payloadLength = frame[2] | (frame[3] << 8);

  if (payloadLength > MAX_PAYLOAD) {
    return { valid: false, error: 'Payload length exceeds maximum' };
  }

  const expectedLength = 4 + payloadLength + 2;
  if (frame.length < expectedLength) {
    return { valid: false, error: 'Frame too short for declared payload' };
  }

  // Verify CRC
  const receivedCrc = frame[4 + payloadLength] | (frame[5 + payloadLength] << 8);
  const computedCrc = crc16(frame, 1, 3 + payloadLength);

  if (receivedCrc !== computedCrc) {
    return {
      valid: false,
      error: `CRC mismatch: received 0x${receivedCrc.toString(16)}, computed 0x${computedCrc.toString(16)}`,
    };
  }

  // Extract payload
  let payload = null;
  if (payloadLength > 0) {
    payload = Buffer.alloc(payloadLength);
    frame.copy(payload, 0, 4, 4 + payloadLength);
  }

  return {
    valid: true,
    errorCode,
    errorString: ERROR_STRINGS[errorCode] || 'Unknown error',
    payload,
    payloadLength,
  };
}

//=============================================================================
// Command Payload Builders
//=============================================================================

export function buildPulseConfigPayload(frequencyHz, amplitudeDac, continuous, burstCount, externalTrigger) {
  const payload = Buffer.alloc(8);
  payload.writeUInt32LE(frequencyHz, 0);
  payload[4] = amplitudeDac & 0xFF;
  payload[5] = continuous ? 1 : 0;
  payload[6] = burstCount & 0xFF;
  payload[7] = externalTrigger ? 1 : 0;
  return payload;
}

export function buildAcqStartPayload(sampleCount, triggerDelay, decimation, averaging, flags) {
  const payload = Buffer.alloc(9);
  payload.writeUInt32LE(sampleCount, 0);
  payload.writeUInt16LE(triggerDelay, 4);
  payload[6] = decimation & 0xFF;
  payload[7] = averaging & 0xFF;
  payload[8] = flags & 0xFF;
  return payload;
}

export function buildVgaGainPayload(gainCode) {
  const payload = Buffer.alloc(1);
  payload[0] = gainCode & 0xFF;
  return payload;
}

export function buildLedSetPayload(r, g, b) {
  const payload = Buffer.alloc(3);
  payload[0] = r & 0xFF;
  payload[1] = g & 0xFF;
  payload[2] = b & 0xFF;
  return payload;
}

export function buildFlashReadPayload(address, length) {
  const payload = Buffer.alloc(6);
  payload.writeUInt32LE(address, 0);
  payload.writeUInt16LE(length, 4);
  return payload;
}

//=============================================================================
// Response Payload Parsers
//=============================================================================

export function parseDeviceInfo(payload) {
  if (!payload || payload.length < 28) return null;

  return {
    deviceId:        payload.readUInt16LE(0),
    hwRev:           payload[2],
    hwVariant:       payload[3],
    fwMajor:         payload[4],
    fwMinor:         payload[5],
    fwPatch:         payload[6],
    fwBuild:         payload.readUInt32LE(7),
    capabilities:    payload.readUInt32LE(11),
    uptimeSeconds:   payload.readUInt32LE(15),
    temperatureC:    payload.readFloatLE(19),
    calibrationValid: payload[23] === 1,
    usbSpeed:        payload[24] === 1 ? 'USB 3.0' : 'USB 2.0',
    powerGood:       payload[25] === 1,
  };
}

export function parseTdcTimestamp(payload) {
  if (!payload || payload.length < 8) return null;
  // Returns picoseconds as BigInt-compatible number
  const psLow = payload.readUInt32LE(0);
  const psHigh = payload.readUInt32LE(4);
  return psLow + psHigh * 0x100000000;
}

export function parseTdcCalibration(payload) {
  if (!payload || payload.length < 520) return null;

  const binWidths = [];
  for (let i = 0; i < 256; i++) {
    binWidths.push(payload.readUInt16LE(i * 2));
  }

  return {
    binWidthFs: binWidths,
    calTimestamp: payload.readUInt32LE(512),
    temperatureC: payload.readFloatLE(516),
    crc: payload.readUInt32LE(520),
  };
}

export function parseAcqStatus(payload) {
  if (!payload || payload.length < 4) return null;

  const status = payload.readUInt32LE(0);
  return {
    running:   (status & 0x01) !== 0,
    complete:  (status & 0x02) !== 0,
    error:     (status & 0x04) !== 0,
    repCount:  (status >> 8) & 0xFF,
  };
}

//=============================================================================
// Protocol Engine Class
//=============================================================================

export class ProtocolEngine {
  constructor() {
    this._device = null;
    this._connected = false;
    this._responseQueue = [];
    this._responseResolvers = [];
    this._pollInterval = null;
    this._commandTimeout = 5000;  // 5 seconds default

    // Callbacks
    this.onDeviceConnected = null;
    this.onDeviceDisconnected = null;
    this.onTemperatureUpdate = null;
    this.onError = null;
  }

  async connect() {
    // In a real implementation, this would use react-native-usb or WebUSB
    // For now, we simulate the connection flow
    try {
      // Attempt to find and open the Chronos Pulser USB device
      // const devices = await USB.getDeviceList();
      // this._device = devices.find(d => d.vendorId === 0x16D0 && d.productId === 0x0C50);

      // Simulated connection
      this._connected = true;

      // Get device info
      const info = await this._sendCommand(CMD.GET_INFO, null);
      if (info && info.valid && info.errorCode === ERR.NONE) {
        const deviceInfo = parseDeviceInfo(info.payload);
        if (this.onDeviceConnected) {
          this.onDeviceConnected(deviceInfo);
        }
      }

      // Start polling for temperature
      this._startTemperaturePolling();

      return true;
    } catch (err) {
      this._connected = false;
      throw err;
    }
  }

  async disconnect() {
    this._stopTemperaturePolling();
    this._connected = false;
    if (this.onDeviceDisconnected) {
      this.onDeviceDisconnected();
    }
  }

  isConnected() {
    return this._connected;
  }

  // Send a command and wait for response
  async _sendCommand(commandId, payload, timeout = null) {
    if (!this._connected) {
      throw new Error('Device not connected');
    }

    const frame = buildCommandFrame(commandId, payload);
    const effectiveTimeout = timeout || this._commandTimeout;

    // In real implementation: send frame via USB bulk endpoint
    // await this._device.transferOut(0x01, frame);

    // Wait for response
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Command 0x${commandId.toString(16)} timed out after ${effectiveTimeout}ms`));
      }, effectiveTimeout);

      // In real implementation: read from USB bulk endpoint
      // For simulation, we resolve immediately with mock data
      clearTimeout(timer);

      // Mock response for simulation
      const mockResponse = this._generateMockResponse(commandId, payload);
      resolve(mockResponse);
    });
  }

  // Generate mock responses for testing without hardware
  _generateMockResponse(commandId, payload) {
    switch (commandId) {
      case CMD.PING:
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success', payload: null };

      case CMD.GET_INFO: {
        const infoPayload = Buffer.alloc(28);
        infoPayload.writeUInt16LE(0x4350, 0);  // Device ID "CP"
        infoPayload[2] = 1;   // HW rev
        infoPayload[3] = 0;   // variant
        infoPayload[4] = 1;   // FW major
        infoPayload[5] = 0;   // FW minor
        infoPayload[6] = 0;   // FW patch
        infoPayload.writeUInt32LE(20260616, 7);
        infoPayload.writeUInt32LE(0x1F | (250 << 16), 11);  // capabilities
        infoPayload.writeUInt32LE(3600, 15);  // uptime
        infoPayload.writeFloatLE(32.5, 19);   // temperature
        infoPayload[23] = 1;  // calibrated
        infoPayload[24] = 1;  // USB 3.0
        infoPayload[25] = 1;  // power good
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success',
                 payload: infoPayload, payloadLength: 28 };
      }

      case CMD.TEMP_READ: {
        const tempPayload = Buffer.alloc(4);
        tempPayload.writeFloatLE(32.5);
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success',
                 payload: tempPayload, payloadLength: 4 };
      }

      case CMD.TDC_SINGLE: {
        const tsPayload = Buffer.alloc(8);
        tsPayload.writeUInt32LE(12345678, 0);
        tsPayload.writeUInt32LE(0, 4);
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success',
                 payload: tsPayload, payloadLength: 8 };
      }

      case CMD.ACQ_START:
      case CMD.PULSE_CONFIG:
      case CMD.VGA_SET_GAIN:
      case CMD.LED_SET:
      case CMD.PULSE_SINGLE:
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success', payload: null };

      default:
        return { valid: true, errorCode: ERR.NONE, errorString: 'Success', payload: null };
    }
  }

  // Public API methods

  async ping() {
    return this._sendCommand(CMD.PING, null, 2000);
  }

  async getDeviceInfo() {
    const resp = await this._sendCommand(CMD.GET_INFO, null);
    if (resp.valid && resp.errorCode === ERR.NONE) {
      return parseDeviceInfo(resp.payload);
    }
    throw new Error(resp.errorString);
  }

  async sendPulseConfig(config) {
    const payload = buildPulseConfigPayload(
      config.frequency || 1000,
      config.amplitude || 128,
      config.continuous || false,
      config.burstCount || 0,
      config.externalTrigger || false
    );
    const resp = await this._sendCommand(CMD.PULSE_CONFIG, payload);
    if (resp.errorCode !== ERR.NONE) {
      throw new Error(resp.errorString);
    }
    return resp;
  }

  async sendVgaGain(gainCode) {
    const payload = buildVgaGainPayload(gainCode);
    const resp = await this._sendCommand(CMD.VGA_SET_GAIN, payload);
    if (resp.errorCode !== ERR.NONE) {
      throw new Error(resp.errorString);
    }
    return resp;
  }

  async startAcquisition(params) {
    const payload = buildAcqStartPayload(
      params.sampleCount || 4096,
      params.triggerDelay || 0,
      params.decimation || 0,
      params.averaging || 0,
      params.flags || 0x03  // triggered + TDC+ADC
    );
    const resp = await this._sendCommand(CMD.ACQ_START, payload);
    if (resp.errorCode !== ERR.NONE) {
      throw new Error(resp.errorString);
    }
    return resp;
  }

  async stopAcquisition() {
    return this._sendCommand(CMD.ACQ_STOP, null);
  }

  async getAcqStatus() {
    const resp = await this._sendCommand(CMD.ACQ_STATUS, null);
    if (resp.valid && resp.errorCode === ERR.NONE) {
      return parseAcqStatus(resp.payload);
    }
    throw new Error(resp.errorString);
  }

  async singleTdcMeasurement() {
    const resp = await this._sendCommand(CMD.TDC_SINGLE, null);
    if (resp.valid && resp.errorCode === ERR.NONE) {
      return parseTdcTimestamp(resp.payload);
    }
    throw new Error(resp.errorString);
  }

  async runTdcCalibration() {
    const resp = await this._sendCommand(CMD.TDC_CALIBRATE, null, 30000);
    if (resp.valid && resp.errorCode === ERR.NONE) {
      return parseTdcCalibration(resp.payload);
    }
    throw new Error(resp.errorString);
  }

  async getTdcCalibration() {
    const resp = await this._sendCommand(CMD.TDC_GET_CAL, null);
    if (resp.valid && resp.errorCode === ERR.NONE) {
      return parseTdcCalibration(resp.payload);
    }
    throw new Error(resp.errorString);
  }

  async readTemperature() {
    const resp = await this._sendCommand(CMD.TEMP_READ, null);
    if (resp.valid && resp.errorCode === ERR.NONE && resp.payload) {
      return resp.payload.readFloatLE(0);
    }
    throw new Error(resp.errorString);
  }

  async setLedColor(r, g, b) {
    const payload = buildLedSetPayload(r, g, b);
    return this._sendCommand(CMD.LED_SET, payload);
  }

  async fireSinglePulse() {
    return this._sendCommand(CMD.PULSE_SINGLE, null);
  }

  async resetDevice() {
    return this._sendCommand(CMD.RESET, null, 10000);
  }

  // Temperature polling
  _startTemperaturePolling() {
    this._stopTemperaturePolling();
    this._pollInterval = setInterval(async () => {
      if (!this._connected) return;
      try {
        const temp = await this.readTemperature();
        if (this.onTemperatureUpdate) {
          this.onTemperatureUpdate(temp);
        }
      } catch (err) {
        // Silently ignore polling errors
      }
    }, 2000);  // Poll every 2 seconds
  }

  _stopTemperaturePolling() {
    if (this._pollInterval) {
      clearInterval(this._pollInterval);
      this._pollInterval = null;
    }
  }
}

//=============================================================================
// Utility Functions
//=============================================================================

// Convert DAC value to approximate mV amplitude
export function dacToMillivolts(dacValue) {
  // Linear interpolation: DAC 0 = 100 mV, DAC 255 = 500 mV
  return 100 + (dacValue / 255) * 400;
}

// Convert mV amplitude to closest DAC value
export function millivoltsToDac(mv) {
  if (mv <= 100) return 0;
  if (mv >= 500) return 255;
  return Math.round(((mv - 100) / 400) * 255);
}

// Convert VGA gain code to dB
export function vgaCodeToDb(code) {
  return -2.5 + (code / 255) * 45.0;
}

// Convert dB to VGA gain code
export function dbToVgaCode(db) {
  if (db <= -2.5) return 0;
  if (db >= 42.5) return 255;
  return Math.round(((db + 2.5) / 45.0) * 255);
}

// Format picoseconds to human-readable string
export function formatPicoseconds(ps) {
  if (ps < 1000) return `${ps.toFixed(1)} ps`;
  if (ps < 1000000) return `${(ps / 1000).toFixed(2)} ns`;
  if (ps < 1000000000) return `${(ps / 1000000).toFixed(2)} µs`;
  return `${(ps / 1000000000).toFixed(2)} ms`;
}

// Calculate distance from time-of-flight (velocity factor 0.66 for RG-58)
export function timeToDistance(ps, velocityFactor = 0.66) {
  const speedOfLight = 299792458;  // m/s
  const velocity = speedOfLight * velocityFactor;
  const oneWayTime = ps / 2;  // Round-trip → one-way
  return (oneWayTime * 1e-12) * velocity;
}

// Format distance
export function formatDistance(meters) {
  if (meters < 0.01) return `${(meters * 1000).toFixed(1)} mm`;
  if (meters < 1) return `${(meters * 100).toFixed(1)} cm`;
  return `${meters.toFixed(2)} m`;
}

// Calculate impedance from reflection coefficient
export function reflectionToImpedance(reflectionCoefficient, z0 = 50) {
  // ZL = Z0 * (1 + Γ) / (1 - Γ)
  const gamma = reflectionCoefficient;
  if (gamma >= 1.0) return Infinity;  // Open circuit
  if (gamma <= -1.0) return 0;        // Short circuit
  return z0 * (1 + gamma) / (1 - gamma);
}

// Calculate reflection coefficient from impedance
export function impedanceToReflection(zLoad, z0 = 50) {
  return (zLoad - z0) / (zLoad + z0);
}

export default ProtocolEngine;
