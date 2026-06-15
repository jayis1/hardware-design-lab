// protocol.js — Aether-Link Binary Wire Protocol
// Defines the binary communication protocol between the React Native
// companion app and the Aether-Link management driver (aether_mgmt.ko).
//
// The app communicates with the device via a TCP socket to the host
// machine running the management driver, which exposes a JSON-RPC-like
// interface over a binary-framed TCP connection on port 4420.
//
// Frame format:
//   [SYNC 2B] [LENGTH 4B] [CMD_ID 2B] [FLAGS 2B] [SEQ_NUM 4B]
//   [PAYLOAD N bytes] [CRC32 4B]
//
// SYNC: 0xA37A ("Aether-Link" magic)
// LENGTH: Total frame length including header and CRC (big-endian)
// CMD_ID: Command identifier (see command table below)
// FLAGS: Bit 0 = response expected, Bit 1 = compressed, Bits 2-15 reserved
// SEQ_NUM: Monotonically increasing sequence number
// PAYLOAD: Command-specific data
// CRC32: IEEE 802.3 CRC-32 of all bytes from SYNC through end of PAYLOAD

import { Buffer } from 'buffer';

// ============================================================================
// Protocol Constants
// ============================================================================

export const PROTOCOL_VERSION = 0x0100;  // Version 1.0
export const DEFAULT_PORT = 4420;
export const SYNC_MAGIC = 0xA37A;
export const FRAME_HEADER_SIZE = 14;  // SYNC(2) + LENGTH(4) + CMD_ID(2) + FLAGS(2) + SEQ_NUM(4)
export const FRAME_CRC_SIZE = 4;
export const FRAME_OVERHEAD = FRAME_HEADER_SIZE + FRAME_CRC_SIZE;  // 18 bytes
export const MAX_PAYLOAD_SIZE = 65536;  // 64KB max payload
export const MAX_FRAME_SIZE = FRAME_OVERHEAD + MAX_PAYLOAD_SIZE;

// ============================================================================
// Command IDs
// ============================================================================

export const CMD = {
  // System commands (0x0000-0x00FF)
  PING:               0x0001,  // Heartbeat/keepalive
  GET_VERSION:        0x0002,  // Get firmware version
  GET_STATUS:         0x0003,  // Get device status bitmap
  GET_FEATURES:       0x0004,  // Get feature bitmap
  RESET_DEVICE:       0x0005,  // Soft reset device
  FW_UPDATE_START:    0x0006,  // Begin firmware update
  FW_UPDATE_DATA:     0x0007,  // Send firmware update chunk
  FW_UPDATE_COMMIT:   0x0008,  // Commit firmware update
  FW_UPDATE_ABORT:    0x0009,  // Abort firmware update

  // Telemetry commands (0x0100-0x01FF)
  GET_TELEMETRY:      0x0101,  // Get all telemetry at once
  GET_TEMP_FPGA:      0x0102,  // Get FPGA temperature
  GET_TEMP_QSFP0:     0x0103,  // Get QSFP0 temperature
  GET_TEMP_QSFP1:     0x0104,  // Get QSFP1 temperature
  GET_POWER:          0x0105,  // Get power consumption
  GET_FAN_SPEED:      0x0106,  // Get fan speeds
  SET_FAN_PWM:        0x0107,  // Set fan PWM duty cycle
  GET_STATS:          0x0108,  // Get all statistics counters
  GET_ERROR_LOG:      0x0109,  // Get error log entries

  // Network commands (0x0200-0x02FF)
  GET_PORT_STATUS:    0x0201,  // Get port link status
  GET_PORT_CONFIG:    0x0202,  // Get port IP/MAC configuration
  SET_PORT_CONFIG:    0x0203,  // Set port IP/netmask/gateway
  GET_CONN_LIST:      0x0204,  // Get list of active connections
  ADD_CONNECTION:     0x0205,  // Add NVMe-oF connection
  DELETE_CONNECTION:  0x0206,  // Delete NVMe-oF connection
  GET_CONN_STATS:     0x0207,  // Get per-connection statistics

  // NVMe commands (0x0300-0x03FF)
  GET_NVME_NAMESPACES: 0x0301, // Get NVMe namespace list
  GET_NVME_QUEUES:    0x0302,  // Get NVMe queue configuration
  GET_NVME_IOPS:      0x0303,  // Get NVMe IOPS counter

  // Event notifications (0x8000-0x80FF) — device → app
  EVENT_TEMP_WARNING: 0x8001,  // Temperature warning threshold crossed
  EVENT_TEMP_CRITICAL:0x8002,  // Temperature critical threshold crossed
  EVENT_LINK_CHANGE:  0x8003,  // Network link state changed
  EVENT_CONN_CHANGE:  0x8004,  // Connection state changed
  EVENT_POWER_WARNING:0x8005,  // Power warning threshold crossed
  EVENT_ERROR:        0x8006,  // Error condition occurred
  EVENT_FW_UPDATE_PROGRESS: 0x8007, // Firmware update progress
};

// ============================================================================
// Flag Definitions
// ============================================================================

export const FLAGS = {
  RESPONSE_EXPECTED:  0x0001,  // Request expects a response
  COMPRESSED:         0x0002,  // Payload is zlib-compressed
  ENCRYPTED:          0x0004,  // Payload is AES-128-GCM encrypted
  PRIORITY_HIGH:      0x0008,  // High-priority command
  STREAM_START:       0x0010,  // First frame of multi-frame stream
  STREAM_CONTINUE:    0x0020,  // Continuation frame
  STREAM_END:         0x0040,  // Final frame of multi-frame stream
};

// ============================================================================
// Response Status Codes
// ============================================================================

export const STATUS = {
  OK:                 0x0000,  // Success
  ERR_UNKNOWN_CMD:    0x0001,  // Unknown command ID
  ERR_INVALID_PARAMS: 0x0002,  // Invalid parameters
  ERR_DEVICE_BUSY:    0x0003,  // Device busy, retry later
  ERR_NOT_READY:      0x0004,  // Device not ready
  ERR_TIMEOUT:        0x0005,  // Operation timed out
  ERR_NO_MEMORY:      0x0006,  // Out of memory
  ERR_NOT_SUPPORTED:  0x0007,  // Feature not supported
  ERR_HW_FAULT:       0x0008,  // Hardware fault
  ERR_CRC_MISMATCH:   0x0009,  // Frame CRC mismatch
  ERR_AUTH_FAILED:    0x000A,  // Authentication failed
  ERR_CONN_REFUSED:   0x000B,  // Connection refused
  ERR_INTERNAL:       0xFFFF,  // Internal error
};

// ============================================================================
// CRC-32 Implementation (IEEE 802.3 polynomial)
// ============================================================================

// Pre-computed CRC-32 lookup table (polynomial 0xEDB88320, reflected)
const CRC32_TABLE = new Uint32Array(256);

function buildCRC32Table() {
  for (let i = 0; i < 256; i++) {
    let crc = i;
    for (let j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = 0xEDB88320 ^ (crc >>> 1);
      } else {
        crc = crc >>> 1;
      }
    }
    CRC32_TABLE[i] = crc;
  }
}

// Initialize table at module load
buildCRC32Table();

/**
 * Compute CRC-32 (IEEE 802.3) of a buffer.
 * @param {Buffer} data - Input data buffer
 * @param {number} initialCrc - Initial CRC value (default 0xFFFFFFFF)
 * @returns {number} 32-bit CRC value
 */
export function crc32(data, initialCrc = 0xFFFFFFFF) {
  let crc = initialCrc;
  for (let i = 0; i < data.length; i++) {
    crc = CRC32_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >>> 8);
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;  // Ensure unsigned 32-bit
}

// ============================================================================
// Frame Builder
// ============================================================================

let globalSeqNum = 0;

/**
 * Build a protocol frame for sending to the device.
 * @param {number} cmdId - Command ID from CMD table
 * @param {Buffer|null} payload - Command payload (null for no payload)
 * @param {number} flags - Frame flags (default: RESPONSE_EXPECTED)
 * @returns {Buffer} Complete frame ready for transmission
 */
export function buildFrame(cmdId, payload = null, flags = FLAGS.RESPONSE_EXPECTED) {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = FRAME_OVERHEAD + payloadLen;

  if (totalLen > MAX_FRAME_SIZE) {
    throw new Error(`Frame too large: ${totalLen} bytes (max ${MAX_FRAME_SIZE})`);
  }

  const seqNum = globalSeqNum++;
  const frame = Buffer.alloc(totalLen);
  let offset = 0;

  // SYNC magic (2 bytes, big-endian)
  frame.writeUInt16BE(SYNC_MAGIC, offset);
  offset += 2;

  // LENGTH (4 bytes, big-endian)
  frame.writeUInt32BE(totalLen, offset);
  offset += 4;

  // CMD_ID (2 bytes, big-endian)
  frame.writeUInt16BE(cmdId, offset);
  offset += 2;

  // FLAGS (2 bytes, big-endian)
  frame.writeUInt16BE(flags, offset);
  offset += 2;

  // SEQ_NUM (4 bytes, big-endian)
  frame.writeUInt32BE(seqNum, offset);
  offset += 4;

  // PAYLOAD (variable)
  if (payload && payloadLen > 0) {
    payload.copy(frame, offset);
    offset += payloadLen;
  }

  // CRC32 (4 bytes, little-endian — covers SYNC through PAYLOAD)
  const crcData = frame.subarray(0, offset);
  const crc = crc32(crcData);
  frame.writeUInt32LE(crc, offset);

  return frame;
}

// ============================================================================
// Frame Parser
// ============================================================================

/**
 * Parse a raw buffer into a protocol frame.
 * @param {Buffer} rawData - Raw data received from socket
 * @returns {Object|null} Parsed frame object, or null if incomplete/invalid
 */
export function parseFrame(rawData) {
  if (rawData.length < FRAME_HEADER_SIZE) {
    return null;  // Incomplete header
  }

  let offset = 0;

  // Check SYNC magic
  const sync = rawData.readUInt16BE(offset);
  if (sync !== SYNC_MAGIC) {
    // Try to re-sync: scan for next SYNC magic
    for (let i = 1; i < rawData.length - 1; i++) {
      if (rawData.readUInt16BE(i) === SYNC_MAGIC) {
        return parseFrame(rawData.subarray(i));
      }
    }
    return null;  // No valid SYNC found
  }
  offset += 2;

  // Read LENGTH
  const totalLen = rawData.readUInt32BE(offset);
  offset += 4;

  if (totalLen < FRAME_OVERHEAD || totalLen > MAX_FRAME_SIZE) {
    return null;  // Invalid length
  }

  if (rawData.length < totalLen) {
    return null;  // Incomplete frame
  }

  // Read CMD_ID
  const cmdId = rawData.readUInt16BE(offset);
  offset += 2;

  // Read FLAGS
  const flags = rawData.readUInt16BE(offset);
  offset += 2;

  // Read SEQ_NUM
  const seqNum = rawData.readUInt32BE(offset);
  offset += 4;

  // Extract PAYLOAD
  const payloadLen = totalLen - FRAME_OVERHEAD;
  let payload = null;
  if (payloadLen > 0) {
    payload = rawData.subarray(offset, offset + payloadLen);
    offset += payloadLen;
  }

  // Verify CRC32
  const receivedCrc = rawData.readUInt32LE(offset);
  const crcData = rawData.subarray(0, offset);
  const computedCrc = crc32(crcData);

  if (receivedCrc !== computedCrc) {
    return {
      valid: false,
      error: 'CRC_MISMATCH',
      cmdId,
      seqNum,
      receivedCrc,
      computedCrc,
    };
  }

  return {
    valid: true,
    cmdId,
    flags,
    seqNum,
    payload,
    totalLen,
  };
}

// ============================================================================
// Response Frame Builder
// ============================================================================

/**
 * Build a response frame for a command.
 * @param {number} cmdId - Original command ID (response uses same ID)
 * @param {number} statusCode - Status code from STATUS table
 * @param {Buffer|null} data - Response data payload
 * @param {number} reqSeqNum - Sequence number from the request
 * @returns {Buffer} Response frame
 */
export function buildResponse(cmdId, statusCode, data = null, reqSeqNum = 0) {
  // Response payload: [STATUS 2B] [RESERVED 2B] [DATA N bytes]
  const dataLen = data ? data.length : 0;
  const payloadLen = 4 + dataLen;
  const payload = Buffer.alloc(payloadLen);

  payload.writeUInt16BE(statusCode, 0);
  payload.writeUInt16BE(0, 2);  // Reserved
  if (data && dataLen > 0) {
    data.copy(payload, 4);
  }

  return buildFrame(cmdId, payload, FLAGS.RESPONSE_EXPECTED);
}

// ============================================================================
// Telemetry Data Structures
// ============================================================================

/**
 * Parse telemetry response payload into structured data.
 * @param {Buffer} payload - Raw telemetry response payload
 * @returns {Object} Structured telemetry data
 */
export function parseTelemetry(payload) {
  if (!payload || payload.length < 40) {
    return null;
  }

  let offset = 0;

  // Status code (2 bytes)
  const status = payload.readUInt16BE(offset);
  offset += 4;  // Skip status + reserved

  const telemetry = {
    status,
    timestamp: payload.readUInt32BE(offset),
    fpgaTemp: payload.readUInt16BE(offset + 4) * 0.01,    // Celsius
    qsfp0Temp: payload.readUInt16BE(offset + 6) * 0.01,   // Celsius
    qsfp1Temp: payload.readUInt16BE(offset + 8) * 0.01,   // Celsius
    powerMW: payload.readUInt32BE(offset + 10),            // Milliwatts
    voltageMV: payload.readUInt16BE(offset + 14),         // Millivolts
    currentMA: payload.readUInt16BE(offset + 16),         // Milliamps
    fan0RPM: payload.readUInt16BE(offset + 18),
    fan1RPM: payload.readUInt16BE(offset + 20),
    fanPWM: payload.readUInt8(offset + 22),
    connCount: payload.readUInt16BE(offset + 23),
    port0Status: payload.readUInt32BE(offset + 25),
    port0Speed: payload.readUInt32BE(offset + 29),
    port1Status: payload.readUInt32BE(offset + 33),
    port1Speed: payload.readUInt32BE(offset + 37),
  };

  return telemetry;
}

/**
 * Parse statistics response payload.
 * @param {Buffer} payload - Raw statistics response payload
 * @returns {Object} Structured statistics data
 */
export function parseStats(payload) {
  if (!payload || payload.length < 60) {
    return null;
  }

  let offset = 4;  // Skip status + reserved

  return {
    txPackets: payload.readBigUInt64BE(offset),
    rxPackets: payload.readBigUInt64BE(offset + 8),
    txBytes: payload.readBigUInt64BE(offset + 16),
    rxBytes: payload.readBigUInt64BE(offset + 24),
    txDrops: payload.readBigUInt64BE(offset + 32),
    rxDrops: payload.readBigUInt64BE(offset + 40),
    tcpRetransmissions: payload.readBigUInt64BE(offset + 48),
    nvmeIOPS: payload.readBigUInt64BE(offset + 56),
  };
}

// ============================================================================
// Connection Management Payloads
// ============================================================================

/**
 * Build an "add connection" payload.
 * @param {string} targetIP - Target IPv4 address (e.g., "192.168.1.100")
 * @param {number} targetPort - Target TCP port (default 4420 for NVMe-oF)
 * @param {number} portId - Local QSFP28 port (0 or 1)
 * @returns {Buffer} Connection request payload
 */
export function buildAddConnectionPayload(targetIP, targetPort = 4420, portId = 0) {
  const payload = Buffer.alloc(32);
  let offset = 0;

  // Target IP (4 bytes)
  const ipParts = targetIP.split('.').map(Number);
  payload.writeUInt8(ipParts[0], offset++);
  payload.writeUInt8(ipParts[1], offset++);
  payload.writeUInt8(ipParts[2], offset++);
  payload.writeUInt8(ipParts[3], offset++);

  // Target port (2 bytes)
  payload.writeUInt16BE(targetPort, offset);
  offset += 2;

  // Local port (2 bytes) — auto-assigned, set to 0
  payload.writeUInt16BE(0, offset);
  offset += 2;

  // Port ID (1 byte)
  payload.writeUInt8(portId, offset);
  offset += 1;

  // Reserved (23 bytes)
  // All zero

  return payload;
}

/**
 * Parse connection list response.
 * @param {Buffer} payload - Raw connection list response
 * @returns {Array} Array of connection objects
 */
export function parseConnectionList(payload) {
  if (!payload || payload.length < 4) {
    return [];
  }

  let offset = 4;  // Skip status + reserved
  const count = payload.readUInt16BE(offset);
  offset += 2;

  const connections = [];
  for (let i = 0; i < count && offset + 20 <= payload.length; i++) {
    const conn = {
      connId: payload.readUInt8(offset),
      state: payload.readUInt8(offset + 1),
      portId: payload.readUInt8(offset + 2),
      srcIP: `${payload.readUInt8(offset + 4)}.${payload.readUInt8(offset + 5)}.${payload.readUInt8(offset + 6)}.${payload.readUInt8(offset + 7)}`,
      dstIP: `${payload.readUInt8(offset + 8)}.${payload.readUInt8(offset + 9)}.${payload.readUInt8(offset + 10)}.${payload.readUInt8(offset + 11)}`,
      srcPort: payload.readUInt16BE(offset + 12),
      dstPort: payload.readUInt16BE(offset + 14),
      txBytes: payload.readUInt32BE(offset + 16),
      rxBytes: payload.readUInt32BE(offset + 20),
    };
    connections.push(conn);
    offset += 24;
  }

  return connections;
}

// ============================================================================
// Event Notification Parsing
// ============================================================================

/**
 * Parse an event notification from the device.
 * @param {Object} frame - Parsed frame object
 * @returns {Object|null} Event data or null if not an event
 */
export function parseEvent(frame) {
  if (!frame || !frame.valid) return null;

  const cmdId = frame.cmdId;
  if (cmdId < 0x8000) return null;  // Not an event

  const payload = frame.payload;
  if (!payload) return null;

  const event = {
    type: cmdId,
    timestamp: payload.readUInt32BE(0),
  };

  switch (cmdId) {
    case CMD.EVENT_TEMP_WARNING:
    case CMD.EVENT_TEMP_CRITICAL:
      event.fpgaTemp = payload.readUInt16BE(4) * 0.01;
      event.qsfp0Temp = payload.readUInt16BE(6) * 0.01;
      break;

    case CMD.EVENT_LINK_CHANGE:
      event.portId = payload.readUInt8(4);
      event.linkUp = payload.readUInt8(5) === 1;
      event.speed = payload.readUInt32BE(6);
      break;

    case CMD.EVENT_CONN_CHANGE:
      event.connId = payload.readUInt8(4);
      event.newState = payload.readUInt8(5);
      break;

    case CMD.EVENT_POWER_WARNING:
      event.powerMW = payload.readUInt32BE(4);
      break;

    case CMD.EVENT_ERROR:
      event.errorCode = payload.readUInt8(4);
      event.severity = payload.readUInt8(5);
      event.source = payload.readUInt8(6);
      break;

    case CMD.EVENT_FW_UPDATE_PROGRESS:
      event.percentComplete = payload.readUInt8(4);
      event.bytesWritten = payload.readUInt32BE(5);
      break;

    default:
      break;
  }

  return event;
}

// ============================================================================
// Frame Accumulator (for TCP stream reassembly)
// ============================================================================

/**
 * Accumulates TCP data and extracts complete frames.
 * Handles partial reads and multiple frames in a single TCP segment.
 */
export class FrameAccumulator {
  constructor() {
    this._buffer = Buffer.alloc(0);
  }

  /**
   * Add received data to the accumulator.
   * @param {Buffer} data - New data from TCP socket
   * @returns {Array} Array of complete parsed frames
   */
  addData(data) {
    this._buffer = Buffer.concat([this._buffer, data]);
    const frames = [];

    while (this._buffer.length >= FRAME_HEADER_SIZE) {
      // Check SYNC
      const sync = this._buffer.readUInt16BE(0);
      if (sync !== SYNC_MAGIC) {
        // Discard bytes until next SYNC
        const syncIdx = this._buffer.subarray(1).indexOf(
          Buffer.from([(SYNC_MAGIC >> 8) & 0xFF, SYNC_MAGIC & 0xFF])
        );
        if (syncIdx === -1) {
          this._buffer = Buffer.alloc(0);  // No SYNC found, discard all
          break;
        }
        this._buffer = this._buffer.subarray(syncIdx + 1);
        continue;
      }

      // Read total length
      const totalLen = this._buffer.readUInt32BE(2);
      if (totalLen < FRAME_OVERHEAD || totalLen > MAX_FRAME_SIZE) {
        // Invalid length, discard first 2 bytes and try again
        this._buffer = this._buffer.subarray(2);
        continue;
      }

      if (this._buffer.length < totalLen) {
        break;  // Incomplete frame, wait for more data
      }

      // Extract complete frame
      const frameData = this._buffer.subarray(0, totalLen);
      this._buffer = this._buffer.subarray(totalLen);

      const frame = parseFrame(frameData);
      if (frame) {
        frames.push(frame);
      }
    }

    return frames;
  }

  /**
   * Clear the accumulator.
   */
  reset() {
    this._buffer = Buffer.alloc(0);
  }

  /**
   * Get the number of buffered bytes.
   * @returns {number}
   */
  get bufferedBytes() {
    return this._buffer.length;
  }
}

// ============================================================================
// Command Name Lookup (for debugging and display)
// ============================================================================

const CMD_NAMES = {};
for (const [name, value] of Object.entries(CMD)) {
  CMD_NAMES[value] = name;
}

/**
 * Get human-readable command name.
 * @param {number} cmdId
 * @returns {string}
 */
export function getCommandName(cmdId) {
  return CMD_NAMES[cmdId] || `UNKNOWN_0x${cmdId.toString(16).toUpperCase().padStart(4, '0')}`;
}

/**
 * Get human-readable status name.
 * @param {number} statusCode
 * @returns {string}
 */
export function getStatusName(statusCode) {
  for (const [name, value] of Object.entries(STATUS)) {
    if (value === statusCode) return name;
  }
  return `UNKNOWN_0x${statusCode.toString(16).toUpperCase().padStart(4, '0')}`;
}

// ============================================================================
// Connection State Names
// ============================================================================

export const TCP_STATES = {
  0: 'CLOSED',
  1: 'LISTEN',
  2: 'SYN_SENT',
  3: 'SYN_RCVD',
  4: 'ESTABLISHED',
  5: 'FIN_WAIT1',
  6: 'FIN_WAIT2',
  7: 'CLOSE_WAIT',
  8: 'CLOSING',
  9: 'LAST_ACK',
  10: 'TIME_WAIT',
};

export function getTCPStateName(state) {
  return TCP_STATES[state] || `UNKNOWN(${state})`;
}
