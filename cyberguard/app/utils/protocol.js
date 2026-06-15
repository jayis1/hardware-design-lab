/**
 * protocol.js - CyberGuard Binary Wire Protocol
 * BLE/USB communication protocol with CRC-16, command IDs,
 * frame builder/parser
 *
 * Frame format:
 *   [STX 0xAA] [SEQ] [CMD] [LEN_H] [LEN_L] [DATA...] [CRC16_H] [CRC16_L]
 *
 * All multi-byte fields are big-endian (network byte order).
 * CRC-16/CCITT polynomial 0x1021, initial value 0xFFFF,
 * calculated over SEQ through last DATA byte.
 */

const STX = 0xAA;
const HEADER_SIZE = 5;   /* STX + SEQ + CMD + LEN_H + LEN_L */
const CRC_SIZE = 2;      /* CRC-16 */
const MAX_PAYLOAD = 512;  /* Maximum payload bytes */
const MAX_FRAME = HEADER_SIZE + MAX_PAYLOAD + CRC_SIZE;

/* ============================================================
 * Command IDs
 * ============================================================ */
export const COMMANDS = {
  /* System commands */
  GET_INFO:            0x01,
  PING:                0x02,
  RESET:               0x03,
  GET_VERSION:         0x04,
  ENTER_BOOTLOADER:    0x05,
  FACTORY_RESET:       0x06,

  /* Authentication commands */
  AUTHENTICATE:        0x10,
  REGISTER:            0x11,
  GET_ASSERTION:       0x12,
  MAKE_CREDENTIAL:     0x13,
  USER_PRESENCE:       0x14,

  /* Fingerprint commands */
  FP_ENROLL:           0x20,
  FP_VERIFY:           0x21,
  FP_DELETE:            0x22,
  FP_DELETE_ALL:        0x23,
  FP_GET_COUNT:         0x24,
  FP_GET_TEMPLATE_IDS:  0x25,

  /* Credential management */
  CRED_LIST:           0x30,
  CRED_DELETE:          0x31,
  CRED_GET_INFO:       0x32,
  CRED_UPDATE:         0x33,

  /* BLE commands */
  BLE_PAIR:            0x40,
  BLE_UNPAIR:          0x41,
  BLE_BOND_LIST:       0x42,

  /* Settings commands */
  SET_CONFIG:           0x50,
  GET_CONFIG:           0x51,
  SET_LED:             0x52,
  SET_TIMEOUT:         0x53,

  /* NFC commands */
  NFC_FIELD_STATUS:    0x60,
  NFC_READ_NDEF:       0x61,

  /* Firmware update */
  FW_UPDATE_START:     0x70,
  FW_UPDATE_DATA:      0x71,
  FW_UPDATE_VERIFY:    0x72,
  FW_UPDATE_ABORT:     0x73,

  /* Power management */
  POWER_STATUS:        0x80,
  POWER_SLEEP:         0x81,
  POWER_WAKE:          0x82,
};

/* ============================================================
 * Response Status Codes
 * ============================================================ */
export const STATUS = {
  OK:                  0x00,
  ERR_INVALID_CMD:     0x01,
  ERR_INVALID_PARAM:   0x02,
  ERR_INVALID_LEN:     0x03,
  ERR_CRC:             0x04,
  ERR_TIMEOUT:         0x05,
  ERR_BUSY:            0x06,
  ERR_NO_FINGER:       0x10,
  ERR_FP_LOW_QUALITY:  0x11,
  ERR_FP_NO_MATCH:     0x12,
  ERR_FP_SLOT_FULL:    0x13,
  ERR_FP_ENROLL_FAIL:  0x14,
  ERR_CRED_NOT_FOUND:  0x20,
  ERR_CRED_SLOT_FULL:  0x21,
  ERR_AUTH_REQUIRED:   0x30,
  ERR_FP_REQUIRED:     0x31,
  ERR_NOT_PAIRED:      0x40,
  ERR_HW_FAULT:       0xFF,
};

/* ============================================================
 * CRC-16/CCITT Implementation
 * Polynomial: 0x1021, Initial: 0xFFFF
 * ============================================================ */
export function crc16(data) {
  let crc = 0xFFFF;

  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
  }

  return crc;
}

/* ============================================================
 * Frame Builder
 * Builds a complete protocol frame from command and payload
 * ============================================================ */
export class CyberGuardProtocol {
  constructor() {
    this.sequence = 0;
  }

  /**
   * Build a protocol frame
   * @param {number} cmd - Command ID (see COMMANDS)
   * @param {Uint8Array|Buffer} payload - Payload data
   * @param {number} [seq] - Optional sequence number (auto-incremented if omitted)
   * @returns {Uint8Array} Complete frame with STX, header, payload, and CRC
   */
  buildFrame(cmd, payload, seq) {
    if (!payload) {
      payload = new Uint8Array(0);
    }

    if (payload.length > MAX_PAYLOAD) {
      throw new Error(`Payload too large: ${payload.length} > ${MAX_PAYLOAD}`);
    }

    const seqNum = (seq !== undefined) ? seq : (this.sequence++ & 0xFF);
    const len = payload.length;

    // Build frame: [STX] [SEQ] [CMD] [LEN_H] [LEN_L] [DATA...] [CRC_H] [CRC_L]
    const frame = new Uint8Array(HEADER_SIZE + payload.length + CRC_SIZE);

    // Header
    frame[0] = STX;
    frame[1] = seqNum & 0xFF;
    frame[2] = cmd & 0xFF;
    frame[3] = (len >> 8) & 0xFF;
    frame[4] = len & 0xFF;

    // Payload
    for (let i = 0; i < payload.length; i++) {
      frame[HEADER_SIZE + i] = payload[i];
    }

    // CRC over SEQ + CMD + LEN + DATA (excluding STX)
    const crcData = frame.slice(1, HEADER_SIZE + payload.length);
    const crcValue = crc16(crcData);
    frame[HEADER_SIZE + payload.length] = (crcValue >> 8) & 0xFF;
    frame[HEADER_SIZE + payload.length + 1] = crcValue & 0xFF;

    return frame;
  }

  /**
   * Parse a received protocol frame
   * @param {Uint8Array} frame - Raw frame data
   * @returns {{seq: number, cmd: number, status: number, payload: Uint8Array}} Parsed frame
   * @throws {Error} If frame is invalid (bad STX, length, or CRC)
   */
  parseFrame(frame) {
    if (!frame || frame.length < HEADER_SIZE + CRC_SIZE) {
      throw new Error(`Frame too short: ${frame ? frame.length : 0}`);
    }

    if (frame[0] !== STX) {
      throw new Error(`Invalid STX: 0x${frame[0].toString(16)}`);
    }

    const seq = frame[1];
    const cmd = frame[2];
    const len = (frame[3] << 8) | frame[4];

    if (frame.length < HEADER_SIZE + len + CRC_SIZE) {
      throw new Error(`Frame too short for payload: expected ${HEADER_SIZE + len + CRC_SIZE}, got ${frame.length}`);
    }

    // Verify CRC
    const crcData = frame.slice(1, HEADER_SIZE + len);
    const expectedCrc = crc16(crcData);
    const receivedCrc = (frame[HEADER_SIZE + len] << 8) | frame[HEADER_SIZE + len + 1];

    if (expectedCrc !== receivedCrc) {
      throw new Error(`CRC mismatch: expected 0x${expectedCrc.toString(16)}, got 0x${receivedCrc.toString(16)}`);
    }

    // Extract payload
    const payload = frame.slice(HEADER_SIZE, HEADER_SIZE + len);

    // Response frames have status byte as first payload byte
    const status = payload.length > 0 ? payload[0] : STATUS.OK;

    return {
      seq,
      cmd,
      status,
      payload: payload.length > 1 ? payload.slice(1) : new Uint8Array(0),
      rawData: payload,
    };
  }

  /**
   * Reset sequence counter
   */
  resetSequence() {
    this.sequence = 0;
  }
}

/* ============================================================
 * Convenience Functions
 * ============================================================ */

/**
 * Create a PING command frame
 * @param {CyberGuardProtocol} protocol - Protocol instance
 * @param {Uint8Array} [challenge] - Optional 8-byte challenge
 * @returns {Uint8Array} Frame bytes
 */
export function createPingFrame(protocol, challenge) {
  const data = challenge || new Uint8Array([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07]);
  return protocol.buildFrame(COMMANDS.PING, data);
}

/**
 * Create a GET_INFO command frame
 * @param {CyberGuardProtocol} protocol - Protocol instance
 * @returns {Uint8Array} Frame bytes
 */
export function createGetInfoFrame(protocol) {
  return protocol.buildFrame(COMMANDS.GET_INFO, new Uint8Array(0));
}

/**
 * Create an AUTHENTICATE command frame
 * @param {CyberGuardProtocol} protocol - Protocol instance
 * @param {Uint8Array} challenge - 32-byte challenge
 * @param {number} keySlot - Key slot (0-9)
 * @param {boolean} requireFingerprint - Whether fingerprint is required
 * @returns {Uint8Array} Frame bytes
 */
export function createAuthenticateFrame(protocol, challenge, keySlot, requireFingerprint) {
  if (challenge.length !== 32) {
    throw new Error('Challenge must be 32 bytes');
  }

  const payload = new Uint8Array(34);
  payload[0] = keySlot & 0x0F;
  payload[1] = (requireFingerprint ? 0x80 : 0x00);
  payload.set(challenge, 2);

  return protocol.buildFrame(COMMANDS.AUTHENTICATE, payload);
}

/**
 * Create a FINGERPRINT_ENROLL command frame
 * @param {CyberGuardProtocol} protocol - Protocol instance
 * @param {number} slot - Fingerprint slot (0-9)
 * @returns {Uint8Array} Frame bytes
 */
export function createFpEnrollFrame(protocol, slot) {
  const payload = new Uint8Array([slot & 0x0F]);
  return protocol.buildFrame(COMMANDS.FP_ENROLL, payload);
}

/**
 * Parse AUTHENTICATE response
 * @param {Object} parsedFrame - Result from protocol.parseFrame()
 * @returns {{signature: Uint8Array, authTime: number}} Parsed response
 */
export function parseAuthResponse(parsedFrame) {
  if (parsedFrame.status !== STATUS.OK) {
    throw new Error(`Authentication failed: status 0x${parsedFrame.status.toString(16)}`);
  }

  if (parsedFrame.payload.length < 66) {
    throw new Error('Response too short for authentication data');
  }

  // Response format: [64-byte signature] [4-byte auth_time_ms]
  const signature = parsedFrame.payload.slice(0, 64);
  const authTime = (parsedFrame.payload[64] << 24) |
                   (parsedFrame.payload[65] << 16) |
                   (parsedFrame.payload[66] << 8) |
                   parsedFrame.payload[67];

  return { signature, authTime };
}

/**
 * Parse GET_INFO response
 * @param {Object} parsedFrame - Result from protocol.parseFrame()
 * @returns {{version: string, fingerprintCount: number, credentialCount: number, batteryLevel: number}}
 */
export function parseGetInfoResponse(parsedFrame) {
  if (parsedFrame.status !== STATUS.OK) {
    throw new Error(`GET_INFO failed: status 0x${parsedFrame.status.toString(16)}`);
  }

  const p = parsedFrame.payload;
  const version = `${p[0]}.${p[1]}.${p[2]}`;
  const fingerprintCount = p[3];
  const credentialCount = p[4];
  const batteryLevel = p[5];

  return { version, fingerprintCount, credentialCount, batteryLevel };
}

/**
 * Convert Uint8Array to hex string (for debugging)
 * @param {Uint8Array} arr - Byte array
 * @returns {string} Hex string
 */
export function bytesToHex(arr) {
  return Array.from(arr).map(b => b.toString(16).padStart(2, '0')).join(' ');
}

/**
 * Convert hex string to Uint8Array
 * @param {string} hex - Hex string (space-separated or continuous)
 * @returns {Uint8Array} Byte array
 */
export function hexToBytes(hex) {
  const clean = hex.replace(/\s+/g, '');
  const arr = new Uint8Array(clean.length / 2);
  for (let i = 0; i < clean.length; i += 2) {
    arr[i / 2] = parseInt(clean.substring(i, i + 2), 16);
  }
  return arr;
}