/**
 * protocol.js — Binary wire protocol for Chronos-RTK companion app
 * Frame format: [STX][LEN][CMD][DATA...][CRC16][ETX]
 * CRC-16/CCITT over CMD + DATA bytes
 */

/* ========================================================================== */
/* Protocol constants                                                           */
/* ========================================================================== */
const STX = 0xAA;   // Start of frame
const ETX = 0x55;   // End of frame
const MAX_FRAME_SIZE = 1024;

/* ========================================================================== */
/* Command IDs (app → device)                                                  */
/* ========================================================================== */
export const COMMAND_IDS = {
  HANDSHAKE:      0x01,
  GET_CONFIG:     0x02,
  SET_LORA_FREQ:  0x10,
  SET_LORA_SF:    0x11,
  SET_RTK_MODE:   0x20,
  SET_NMEA_RATE:  0x21,
  SET_OUTPUT:     0x22,
  SET_LOGGING:    0x30,
  ERASE_FLASH:   0x31,
  DOWNLOAD_LOG:   0x32,
  CLEAR_LOG:     0x33,
  GET_POSITION:   0x40,
  GET_SATELLITES: 0x41,
  REBOOT:        0xF0,
  BOOTLOADER:    0xF1,
  CUSTOM:        0xFF,
};

/* ========================================================================== */
/* Response IDs (device → app)                                                 */
/* ========================================================================== */
export const RESPONSE_IDS = {
  ACK:            0x01,
  NACK:           0x02,
  CONFIG:         0x03,
  POSITION:       0x40,
  SATELLITES:     0x41,
  LOG_DATA:       0x50,
  NMEA_SENTENCE:  0x60,
  RTCM_MESSAGE:   0x61,
  STATUS:         0x70,
};

/* ========================================================================== */
/* CRC-16/CCITT implementation                                                  */
/* ========================================================================== */
const CRC16_POLY = 0x1021;  // x^16 + x^12 + x^5 + 1

/**
 * Calculate CRC-16/CCITT over a byte array
 * @param {Uint8Array} data  Input bytes
 * @param {number} start     Start index (default 0)
 * @param {number} length    Number of bytes (default data.length)
 * @returns {number} 16-bit CRC
 */
export function crc16Ccitt(data, start = 0, length = data.length) {
  let crc = 0xFFFF;
  for (let i = start; i < start + length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ CRC16_POLY) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }
  return crc;
}

/* ========================================================================== */
/* Frame builder                                                               */
/* ========================================================================== */
export class ChronosProtocol {
  constructor() {
    this.rxBuffer = new Uint8Array(MAX_FRAME_SIZE);
    this.rxIndex = 0;
    this.rxState = 'IDLE';  // IDLE, STX_RECEIVED, DATA, CRC_HIGH, CRC_LOW, ETX
    this.rxCrcHigh = 0;
  }

  /**
   * Build a command frame to send to the device
   * Frame format: [0xAA][LEN_H][LEN_L][CMD][DATA...][CRC_H][CRC_L][0x55]
   * @param {number} cmdId  Command ID from COMMAND_IDS
   * @param {number[]} data Payload data bytes
   * @returns {Uint8Array} Complete frame ready to send
   */
  buildCommand(cmdId, data = []) {
    const payloadLen = 1 + data.length;  // CMD + DATA
    const totalLen = 1 + 2 + payloadLen + 2 + 1;  // STX + LEN + PAYLOAD + CRC + ETX
    const frame = new Uint8Array(totalLen);

    let idx = 0;
    frame[idx++] = STX;
    frame[idx++] = (payloadLen >> 8) & 0xFF;  // LEN high
    frame[idx++] = payloadLen & 0xFF;           // LEN low
    frame[idx++] = cmdId & 0xFF;

    for (let i = 0; i < data.length; i++) {
      frame[idx++] = data[i] & 0xFF;
    }

    // Calculate CRC over CMD + DATA
    const crcData = frame.slice(3, 3 + payloadLen);
    const crc = crc16Ccitt(crcData);
    frame[idx++] = (crc >> 8) & 0xFF;  // CRC high
    frame[idx++] = crc & 0xFF;          // CRC low
    frame[idx++] = ETX;

    return frame;
  }

  /**
   * Parse incoming bytes and extract complete frames
   * @param {Uint8Array} bytes  Raw bytes from serial port
   * @returns {Array<{cmdId: number, data: Uint8Array}>} Parsed frames
   */
  parse(bytes) {
    const frames = [];

    for (let i = 0; i < bytes.length; i++) {
      const byte = bytes[i];

      switch (this.rxState) {
        case 'IDLE':
          if (byte === STX) {
            this.rxIndex = 0;
            this.rxBuffer[this.rxIndex++] = byte;
            this.rxState = 'STX_RECEIVED';
          }
          break;

        case 'STX_RECEIVED':
          this.rxCrcHigh = byte;  // LEN high byte (also store for CRC)
          this.rxBuffer[this.rxIndex++] = byte;
          this.rxState = 'LEN_LOW';
          break;

        case 'LEN_LOW': {
          const payloadLen = (this.rxCrcHigh << 8) | byte;
          if (payloadLen > MAX_FRAME_SIZE - 6) {
            this.rxState = 'IDLE';  // Invalid length
            break;
          }
          this.rxBuffer[this.rxIndex++] = byte;
          this.rxState = 'DATA';
          this.rxRemaining = payloadLen;
          break;
        }

        case 'DATA':
          this.rxBuffer[this.rxIndex++] = byte;
          this.rxRemaining--;
          if (this.rxRemaining === 0) {
            this.rxState = 'CRC_HIGH';
          }
          break;

        case 'CRC_HIGH':
          this.rxCrcHigh = byte;
          this.rxBuffer[this.rxIndex++] = byte;
          this.rxState = 'CRC_LOW';
          break;

        case 'CRC_LOW': {
          const expectedCrc = (this.rxCrcHigh << 8) | byte;
          this.rxBuffer[this.rxIndex++] = byte;

          // Extract payload for CRC check
          const payloadLen = (this.rxBuffer[1] << 8) | this.rxBuffer[2];
          const payload = this.rxBuffer.slice(3, 3 + payloadLen);
          const computedCrc = crc16Ccitt(payload);

          if (computedCrc === expectedCrc) {
            this.rxState = 'ETX';
          } else {
            // CRC mismatch - discard frame
            this.rxState = 'IDLE';
          }
          break;
        }

        case 'ETX':
          if (byte === ETX) {
            // Complete valid frame received
            const payloadLen = (this.rxBuffer[1] << 8) | this.rxBuffer[2];
            const cmdId = this.rxBuffer[3];
            const data = this.rxBuffer.slice(4, 4 + payloadLen - 1);
            frames.push({
              cmdId,
              data: new Uint8Array(data),
            });
          }
          this.rxState = 'IDLE';
          break;

        default:
          this.rxState = 'IDLE';
          break;
      }
    }

    return frames;
  }

  /**
   * Reset the parser state
   */
  reset() {
    this.rxState = 'IDLE';
    this.rxIndex = 0;
  }
}

/* ========================================================================== */
/* Convenience functions for building specific commands                         */
/* ========================================================================== */

/**
 * Build a handshake command
 * @returns {Uint8Array}
 */
export function buildHandshake() {
  const proto = new ChronosProtocol();
  return proto.buildCommand(COMMAND_IDS.HANDSHAKE, [
    0x01,  // Protocol version high
    0x00,  // Protocol version low
  ]);
}

/**
 * Build a set LoRa frequency command
 * @param {number} freqHz  Frequency in Hz (868000000 or 915000000)
 * @returns {Uint8Array}
 */
export function buildSetLoraFreq(freqHz) {
  const proto = new ChronosProtocol();
  return proto.buildCommand(COMMAND_IDS.SET_LORA_FREQ, [
    (freqHz >> 24) & 0xFF,
    (freqHz >> 16) & 0xFF,
    (freqHz >> 8) & 0xFF,
    freqHz & 0xFF,
  ]);
}

/**
 * Build a set RTK mode command
 * @param {string} mode  'rover', 'base', or 'standalone'
 * @returns {Uint8Array}
 */
export function buildSetRtkMode(mode) {
  const proto = new ChronosProtocol();
  const modeMap = { rover: 0, base: 1, standalone: 2 };
  return proto.buildCommand(COMMAND_IDS.SET_RTK_MODE, [modeMap[mode] ?? 0]);
}

/**
 * Parse a position response from the device
 * @param {Uint8Array} data  Response payload
 * @returns {Object} Position data
 */
export function parsePositionResponse(data) {
  if (data.length < 32) return null;
  return {
    latitude: data.getFloat64(0, false),   // Big-endian double
    longitude: data.getFloat64(8, false),
    altitude: data.getFloat64(16, false),
    hAccuracy: data.getFloat32(24, false),  // Big-endian float
    vAccuracy: data.getFloat32(28, false),
  };
}

/**
 * Parse a satellites response from the device
 * @param {Uint8Array} data  Response payload
 * @returns {Object} Satellite data
 */
export function parseSatellitesResponse(data) {
  if (data.length < 8) return null;
  return {
    inView: (data[0] << 8) | data[1],
    used: data[2],
    gps: data[3],
    glo: data[4],
    gal: data[5],
    bds: data[6],
    pdop: data.getFloat32(7, false),
  };
}

/**
 * Parse a config response from the device
 * @param {Uint8Array} data  Response payload
 * @returns {Object} Config data
 */
export function parseConfigResponse(data) {
  if (data.length < 16) return null;
  return {
    firmwareMajor: data[0],
    firmwareMinor: data[1],
    firmwarePatch: data[2],
    batteryPercent: data[3],
    flashUsedPercent: data[4],
    loraFreqHz: (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8],
    loraSf: data[9],
    rtkMode: data[10],
    nmeaRate: data[11],
  };
}