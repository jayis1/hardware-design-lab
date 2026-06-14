// ============================================================================
// utils/protocol.js — RideCore Binary Wire Protocol with CRC
// ============================================================================
// Frame format:
//   [0xAA] [CMD_ID] [LENGTH] [PAYLOAD...] [CRC16_LO] [CRC16_HI]
//
// CRC16-CCITT over CMD_ID + LENGTH + PAYLOAD
// ============================================================================

// Command IDs (must match firmware ble_uart.h)
export const CMD_IDS = {
  CMD_PING:          0x01,
  CMD_GET_STATUS:    0x02,
  CMD_SET_THROTTLE:  0x10,
  CMD_SET_BRAKE:     0x11,
  CMD_GET_CONFIG:    0x20,
  CMD_SET_CONFIG:    0x21,
  CMD_SAVE_CONFIG:   0x22,
  CMD_FAULT_CLEAR:   0x30,
  CMD_FW_UPDATE:     0x40,

  RSP_ACK:           0x80,
  RSP_NACK:          0x81,
  RSP_STATUS:        0x82,
  RSP_CONFIG:        0x83,
  RSP_FAULT:         0x84,
};

const FRAME_HEADER = 0xAA;
const MAX_PAYLOAD = 128;

/**
 * CRC16-CCITT (poly 0x1021, init 0xFFFF)
 * @param {Buffer|Uint8Array} data
 * @param {number} len
 * @returns {number} CRC16 value
 */
export function crc16(data, len) {
  let crc = 0xFFFF;
  for (let i = 0; i < len; i++) {
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

/**
 * Build a RideCore protocol frame
 * @param {number} cmdId - Command ID from CMD_IDS
 * @param {Buffer} payload - Payload bytes (max 128)
 * @returns {Buffer} Complete frame ready for transmission
 */
export function buildFrame(cmdId, payload) {
  if (!payload) payload = Buffer.alloc(0);
  if (payload.length > MAX_PAYLOAD) {
    throw new Error(`Payload too large: ${payload.length} > ${MAX_PAYLOAD}`);
  }

  // Build CRC data: cmdId + length + payload
  const crcData = Buffer.alloc(2 + payload.length);
  crcData[0] = cmdId;
  crcData[1] = payload.length;
  payload.copy(crcData, 2);

  const crcVal = crc16(crcData, crcData.length);

  // Build complete frame: header + cmdId + length + payload + crc16
  const frame = Buffer.alloc(4 + payload.length);
  frame[0] = FRAME_HEADER;
  frame[1] = cmdId;
  frame[2] = payload.length;
  payload.copy(frame, 3);
  frame[3 + payload.length] = crcVal & 0xFF;
  frame[4 + payload.length] = (crcVal >> 8) & 0xFF;

  return frame;
}

/**
 * Parse a RideCore protocol frame from raw bytes
 * @param {Buffer} data - Raw bytes received (may contain multiple frames)
 * @returns {Object|null} Parsed frame: { cmdId, length, payload, crc16, valid }
 */
export function parseFrame(data) {
  if (!data || data.length < 5) return null;

  // Find header
  let offset = 0;
  while (offset < data.length && data[offset] !== FRAME_HEADER) {
    offset++;
  }
  if (offset >= data.length) return null;

  const cmdId = data[offset + 1];
  const length = data[offset + 2];

  if (length > MAX_PAYLOAD) return null;
  if (offset + 3 + length + 2 > data.length) return null;  // Incomplete frame

  const payload = Buffer.from(data.slice(offset + 3, offset + 3 + length));
  const receivedCrc = data[offset + 3 + length] | (data[offset + 4 + length] << 8);

  // Verify CRC
  const crcData = Buffer.alloc(2 + length);
  crcData[0] = cmdId;
  crcData[1] = length;
  payload.copy(crcData, 2);
  const calcCrc = crc16(crcData, crcData.length);

  return {
    cmdId,
    length,
    payload,
    crc16: receivedCrc,
    valid: calcCrc === receivedCrc,
    consumed: offset + 5 + length,  // Total bytes consumed
  };
}

/**
 * Frame parser state machine for streaming data
 * Usage: const parser = createFrameParser();
 * parser.onFrame = (frame) => { ... };
 * parser.feed(chunk);
 */
export function createFrameParser() {
  let state = 'HEADER';
  let cmdId = 0;
  let length = 0;
  let payload = [];
  let crcLo = 0;

  const parser = {
    onFrame: null,

    feed(data) {
      for (let i = 0; i < data.length; i++) {
        const byte = data[i];

        switch (state) {
          case 'HEADER':
            if (byte === FRAME_HEADER) {
              state = 'CMD_ID';
            }
            break;

          case 'CMD_ID':
            cmdId = byte;
            state = 'LENGTH';
            break;

          case 'LENGTH':
            length = byte;
            if (length > MAX_PAYLOAD) {
              state = 'HEADER';  // Invalid, reset
            } else if (length === 0) {
              state = 'CRC_LO';
            } else {
              payload = [];
              state = 'PAYLOAD';
            }
            break;

          case 'PAYLOAD':
            payload.push(byte);
            if (payload.length >= length) {
              state = 'CRC_LO';
            }
            break;

          case 'CRC_LO':
            crcLo = byte;
            state = 'CRC_HI';
            break;

          case 'CRC_HI': {
            const receivedCrc = crcLo | (byte << 8);
            const payloadBuf = Buffer.from(payload);
            const crcData = Buffer.alloc(2 + length);
            crcData[0] = cmdId;
            crcData[1] = length;
            payloadBuf.copy(crcData, 2);
            const calcCrc = crc16(crcData, crcData.length);

            if (parser.onFrame) {
              parser.onFrame({
                cmdId,
                length,
                payload: payloadBuf,
                crc16: receivedCrc,
                valid: calcCrc === receivedCrc,
              });
            }
            state = 'HEADER';
            break;
          }
        }
      }
    },

    reset() {
      state = 'HEADER';
      cmdId = 0;
      length = 0;
      payload = [];
      crcLo = 0;
    },
  };

  return parser;
}