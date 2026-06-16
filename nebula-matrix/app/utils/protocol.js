/**
 * protocol.js — Nebula Matrix Binary Wire Protocol
 *
 * Complete implementation of the Nebula Matrix binary communication protocol
 * over UDP. Provides frame building, parsing, CRC-16 validation, command
 * encoding/decoding, and high-level API methods for all device operations.
 *
 * Protocol Overview:
 *   - Transport: UDP (Ethernet or USB RNDIS)
 *   - Default port: 6454 (shared with Art-Net for convenience)
 *   - Framing: Magic header + command ID + payload length + payload + CRC-16
 *   - Endianness: Big-endian (network byte order)
 *   - CRC: CRC-16/XMODEM (polynomial 0x1021)
 *
 * Frame Format (all fields big-endian):
 *   ┌──────────┬──────────┬──────────┬──────────────┬──────────┐
 *   │ Magic    │ Command  │ Payload  │ Payload      │ CRC-16   │
 *   │ 4 bytes  │ 2 bytes  │ Len 2B   │ (0-1400 B)   │ 2 bytes  │
 *   │ "NEBU"   │          │          │              │          │
 *   └──────────┴──────────┴──────────┴──────────────┴──────────┘
 *
 * Command IDs:
 *   0x0001 — PING (no payload, response has device info)
 *   0x0002 — GET_STATUS (no payload, response has full status)
 *   0x0010 — SET_INPUT_SOURCE (payload: 1 byte source ID)
 *   0x0011 — SET_BRIGHTNESS (payload: 2 bytes, 0-65535)
 *   0x0012 — SET_CONTRAST (payload: 2 bytes, 0-65535)
 *   0x0013 — SET_OUTPUT_ENABLE (payload: 1 byte, 0/1)
 *   0x0014 — FREEZE_FRAME (payload: 1 byte, 0/1)
 *   0x0015 — ENABLE_TEST_PATTERN (payload: 1 byte pattern ID)
 *   0x0016 — SOFT_RESET (no payload)
 *   0x0020 — SET_MATRIX_CONFIG (payload: width 2B + height 2B + scan 1B)
 *   0x0021 — SET_DITHER_CONFIG (payload: mode 1B + strength 1B)
 *   0x0022 — SET_GAMMA_PRESET (payload: preset ID 1B)
 *   0x0023 — SET_GAMMA_LUTS (payload: 3×256×2 = 1536 bytes)
 *   0x0024 — SET_WHITE_POINT (payload: R 1B + G 1B + B 1B)
 *   0x0030 — SET_NETWORK_CONFIG (payload: IP 4B + mask 4B + gw 4B + DHCP 1B + universe 2B + priority 1B)
 *   0x0031 — SET_DEVICE_IDENTITY (payload: name 32B + location 64B)
 *   0x0040 — FIRMWARE_UPDATE_START (payload: type 1B + size 4B + version 4B)
 *   0x0041 — FIRMWARE_UPDATE_DATA (payload: offset 4B + data up to 1400B)
 *   0x0042 — FIRMWARE_UPDATE_COMPLETE (no payload)
 *   0xFFFF — RESPONSE (response flag OR'd with command ID)
 */

import { Buffer } from 'buffer';
import CRC32 from 'crc-32';  /* We'll use CRC-32 for simplicity, or implement CRC-16 */

/* =========================================================================
 * Protocol Constants
 * ========================================================================= */

const PROTOCOL_MAGIC = Buffer.from('NEBU', 'ascii');  /* 0x4E454255 */
const PROTOCOL_VERSION = 0x0100;
const DEFAULT_PORT = 6454;
const MAX_PAYLOAD_SIZE = 1400;
const RESPONSE_TIMEOUT_MS = 2000;
const MAX_RETRIES = 3;

/* Command IDs */
const CMD = {
  PING:                   0x0001,
  GET_STATUS:             0x0002,
  SET_INPUT_SOURCE:       0x0010,
  SET_BRIGHTNESS:         0x0011,
  SET_CONTRAST:           0x0012,
  SET_OUTPUT_ENABLE:      0x0013,
  FREEZE_FRAME:           0x0014,
  ENABLE_TEST_PATTERN:    0x0015,
  SOFT_RESET:             0x0016,
  SET_MATRIX_CONFIG:      0x0020,
  SET_DITHER_CONFIG:      0x0021,
  SET_GAMMA_PRESET:       0x0022,
  SET_GAMMA_LUTS:         0x0023,
  SET_WHITE_POINT:        0x0024,
  SET_NETWORK_CONFIG:     0x0030,
  SET_DEVICE_IDENTITY:    0x0031,
  FIRMWARE_UPDATE_START:  0x0040,
  FIRMWARE_UPDATE_DATA:   0x0041,
  FIRMWARE_UPDATE_COMPLETE: 0x0042,
  RESPONSE_FLAG:          0x8000,
};

/* Input source IDs */
const INPUT_SOURCE = {
  NONE:          0x00,
  ETHERNET:      0x01,
  HDMI:          0x02,
  SD_CARD:       0x03,
  TEST_PATTERN:  0x04,
  SPI_DIRECT:    0x05,
};

/* Test pattern IDs */
const TEST_PATTERN = {
  COLOR_BARS: 0,
  GRADIENT:   1,
  GRID:       2,
  WHITE:      3,
};

/* Gamma presets */
const GAMMA_PRESET = {
  LINEAR: 0,
  SRGB:   1,
  VIDEO:  2,
  DCI_P3: 3,
  CUSTOM: 4,
};

/* Dither modes */
const DITHER_MODE = {
  NONE:             0,
  FLOYD_STEINBERG:  1,
  ATKINSON:         2,
  BLUE_NOISE:       3,
  TEMPORAL:         4,
  HYBRID:           5,
};

/* Firmware update types */
const FW_TYPE = {
  FPGA: 0,
  MCU:  1,
};

/* =========================================================================
 * CRC-16/XMODEM Implementation
 * =========================================================================
 *
 * Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 * Initial value: 0x0000
 * No XOR out, reflected input/output: false
 */

function crc16Xmodem(data) {
  let crc = 0x0000;
  const polynomial = 0x1021;

  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ polynomial;
      } else {
        crc <<= 1;
      }
      crc &= 0xFFFF;  /* Keep 16-bit */
    }
  }
  return crc;
}

/* =========================================================================
 * Frame Builder
 * ========================================================================= */

/**
 * Build a protocol frame for transmission.
 *
 * @param {number} commandId — Command ID (without response flag)
 * @param {Buffer|null} payload — Payload data (null for no payload)
 * @returns {Buffer} Complete frame ready for UDP transmission
 */
function buildFrame(commandId, payload = null) {
  const payloadLen = payload ? payload.length : 0;

  if (payloadLen > MAX_PAYLOAD_SIZE) {
    throw new Error(`Payload too large: ${payloadLen} > ${MAX_PAYLOAD_SIZE}`);
  }

  /* Frame layout:
   *   [0-3]:   Magic "NEBU" (4 bytes)
   *   [4-5]:   Command ID (2 bytes, big-endian)
   *   [6-7]:   Payload length (2 bytes, big-endian)
   *   [8-...]: Payload (variable)
   *   [...]:   CRC-16 (2 bytes, big-endian)
   */
  const headerSize = 8;
  const crcSize = 2;
  const frameSize = headerSize + payloadLen + crcSize;

  const frame = Buffer.alloc(frameSize);

  /* Magic */
  PROTOCOL_MAGIC.copy(frame, 0);

  /* Command ID (big-endian) */
  frame.writeUInt16BE(commandId, 4);

  /* Payload length (big-endian) */
  frame.writeUInt16BE(payloadLen, 6);

  /* Payload */
  if (payload && payloadLen > 0) {
    payload.copy(frame, 8);
  }

  /* CRC-16 over everything except CRC field itself */
  const crcData = frame.slice(0, frameSize - crcSize);
  const crc = crc16Xmodem(crcData);
  frame.writeUInt16BE(crc, frameSize - crcSize);

  return frame;
}

/* =========================================================================
 * Frame Parser
 * ========================================================================= */

/**
 * Parse a received protocol frame.
 *
 * @param {Buffer} data — Raw UDP packet data
 * @returns {Object} Parsed frame: { commandId, payload, valid }
 */
function parseFrame(data) {
  if (!data || data.length < 10) {
    return { commandId: 0, payload: null, valid: false, error: 'Frame too short' };
  }

  /* Check magic */
  const magic = data.slice(0, 4);
  if (!magic.equals(PROTOCOL_MAGIC)) {
    return { commandId: 0, payload: null, valid: false, error: 'Invalid magic' };
  }

  /* Read header */
  const commandId = data.readUInt16BE(4);
  const payloadLen = data.readUInt16BE(6);

  /* Validate length */
  const expectedSize = 8 + payloadLen + 2;
  if (data.length < expectedSize) {
    return { commandId: 0, payload: null, valid: false, error: 'Frame truncated' };
  }

  /* Extract payload */
  let payload = null;
  if (payloadLen > 0) {
    payload = data.slice(8, 8 + payloadLen);
  }

  /* Verify CRC */
  const crcData = data.slice(0, expectedSize - 2);
  const receivedCrc = data.readUInt16BE(expectedSize - 2);
  const computedCrc = crc16Xmodem(crcData);

  if (receivedCrc !== computedCrc) {
    return {
      commandId,
      payload,
      valid: false,
      error: `CRC mismatch: received 0x${receivedCrc.toString(16)}, computed 0x${computedCrc.toString(16)}`,
    };
  }

  return { commandId, payload, valid: true, error: null };
}

/* =========================================================================
 * Response Parser Helpers
 * ========================================================================= */

/**
 * Parse PING response payload.
 * Format: version(2B) + fpgaId(4B) + mcuVersion(2B) + deviceName(32B) + matrixWidth(2B) + matrixHeight(2B)
 */
function parsePingResponse(payload) {
  if (!payload || payload.length < 44) return null;

  let offset = 0;
  const version = payload.readUInt16BE(offset); offset += 2;
  const fpgaId = payload.readUInt32BE(offset); offset += 4;
  const mcuVersion = payload.readUInt16BE(offset); offset += 2;
  const deviceName = payload.slice(offset, offset + 32).toString('ascii').replace(/\0/g, ''); offset += 32;
  const matrixWidth = payload.readUInt16BE(offset); offset += 2;
  const matrixHeight = payload.readUInt16BE(offset); offset += 2;

  return {
    version: `${(version >> 8) & 0xFF}.${version & 0xFF}.0`,
    fpgaId,
    mcuVersion: `${(mcuVersion >> 8) & 0xFF}.${mcuVersion & 0xFF}.0`,
    deviceName,
    matrixWidth,
    matrixHeight,
  };
}

/**
 * Parse GET_STATUS response payload.
 * Format: status(4B) + inputSource(1B) + brightness(2B) + contrast(2B) +
 *         refreshRate(2B) + framesOutput(4B) + framesDropped(4B) +
 *         tempFpga(2B signed, ×10) + tempDdrA(2B) + tempDdrB(2B) + tempAmbient(2B) +
 *         voltage12v(2B, mV) + current12v(2B, mA) +
 *         ethLinkUp(1B) + ethSpeed(2B) + ethPacketsRx(4B) + ethPacketsDropped(4B) +
 *         spiPixelsRx(4B) + spiFramesRx(4B) + spiErrors(4B) + spiOverflows(4B) +
 *         fanSpeed(1B) + uptime(4B) + systemErrors(4B)
 */
function parseStatusResponse(payload) {
  if (!payload || payload.length < 60) return null;

  let offset = 0;
  const status = payload.readUInt32BE(offset); offset += 4;
  const inputSource = payload.readUInt8(offset); offset += 1;
  const brightness = payload.readUInt16BE(offset); offset += 2;
  const contrast = payload.readUInt16BE(offset); offset += 2;
  const refreshRate = payload.readUInt16BE(offset); offset += 2;
  const framesOutput = payload.readUInt32BE(offset); offset += 4;
  const framesDropped = payload.readUInt32BE(offset); offset += 4;

  /* Temperatures: signed 16-bit, value × 10 */
  const tempFpga = payload.readInt16BE(offset) / 10.0; offset += 2;
  const tempDdrA = payload.readInt16BE(offset) / 10.0; offset += 2;
  const tempDdrB = payload.readInt16BE(offset) / 10.0; offset += 2;
  const tempAmbient = payload.readInt16BE(offset) / 10.0; offset += 2;

  /* Power: mV and mA */
  const voltage12v = payload.readUInt16BE(offset) / 1000.0; offset += 2;
  const current12v = payload.readUInt16BE(offset) / 1000.0; offset += 2;
  const powerWatts = voltage12v * current12v;

  /* Ethernet */
  const ethLinkUp = payload.readUInt8(offset) !== 0; offset += 1;
  const ethSpeed = payload.readUInt16BE(offset); offset += 2;
  const ethPacketsRx = payload.readUInt32BE(offset); offset += 4;
  const ethPacketsDropped = payload.readUInt32BE(offset); offset += 4;

  /* SPI */
  const spiPixelsRx = payload.readUInt32BE(offset); offset += 4;
  const spiFramesRx = payload.readUInt32BE(offset); offset += 4;
  const spiErrors = payload.readUInt32BE(offset); offset += 4;
  const spiOverflows = payload.readUInt32BE(offset); offset += 4;

  /* System */
  const fanSpeed = payload.readUInt8(offset); offset += 1;
  const uptime = payload.readUInt32BE(offset); offset += 4;
  const systemErrors = payload.readUInt32BE(offset); offset += 4;

  /* Derived values */
  const outputEnabled = !!(status & 0x01);
  const spiPixelRate = refreshRate > 0 ? (spiPixelsRx / Math.max(uptime, 1) / 1000000).toFixed(1) : '0.0';

  return {
    status,
    inputSource,
    brightness,
    contrast,
    refreshRate,
    framesOutput,
    framesDropped,
    tempFpga,
    tempDdrA,
    tempDdrB,
    tempAmbient,
    voltage12v,
    current12v,
    powerWatts,
    ethLinkUp,
    ethSpeed: ethSpeed === 1000 ? '1 Gbps' : ethSpeed === 100 ? '100 Mbps' : ethSpeed === 10 ? '10 Mbps' : `${ethSpeed} Mbps`,
    ethPacketsRx,
    ethPacketsDropped,
    spiPixelsRx,
    spiFramesRx,
    spiErrors,
    spiOverflows,
    spiPixelRate,
    fanSpeed,
    uptime,
    systemErrors,
    outputEnabled,
  };
}

/* =========================================================================
 * NebulaProtocol Class
 * =========================================================================
 *
 * High-level API for communicating with a Nebula Matrix device.
 * Handles connection lifecycle, command/response matching, retries,
 * and timeouts.
 */

class NebulaProtocol {
  constructor() {
    this.socket = null;
    this.host = null;
    this.port = null;
    this.connected = false;
    this.pendingRequests = new Map();  /* sequenceNum → { resolve, reject, timer } */
    this.sequenceNum = 0;
    this.responseBuffer = Buffer.alloc(0);
  }

  /* =====================================================================
   * Connection Management
   * ===================================================================== */

  /**
   * Connect to a Nebula Matrix device.
   *
   * @param {string} host — IP address or hostname
   * @param {number} port — UDP port (default 6454)
   * @returns {Promise<void>}
   */
  async connect(host, port = DEFAULT_PORT) {
    if (this.connected) {
      await this.disconnect();
    }

    this.host = host;
    this.port = port;

    /* Create UDP socket using react-native-udp */
    const dgram = require('react-native-udp');
    this.socket = dgram.createSocket({ type: 'udp4' });

    return new Promise((resolve, reject) => {
      this.socket.bind(0, () => {
        this.connected = true;
        resolve();
      });

      this.socket.on('message', (msg, rinfo) => {
        this._handleMessage(msg);
      });

      this.socket.on('error', (err) => {
        console.error('NebulaProtocol socket error:', err);
        this.connected = false;
      });

      /* Timeout for bind */
      setTimeout(() => {
        if (!this.connected) {
          reject(new Error('Socket bind timeout'));
        }
      }, 5000);
    });
  }

  /**
   * Disconnect from the device.
   */
  async disconnect() {
    this.connected = false;

    /* Reject all pending requests */
    for (const [seq, pending] of this.pendingRequests) {
      clearTimeout(pending.timer);
      pending.reject(new Error('Disconnected'));
    }
    this.pendingRequests.clear();

    if (this.socket) {
      try {
        this.socket.close();
      } catch (e) {
        /* Ignore close errors */
      }
      this.socket = null;
    }

    this.host = null;
    this.port = null;
  }

  /* =====================================================================
   * Message Handling
   * ===================================================================== */

  _handleMessage(msg) {
    const frame = parseFrame(msg);
    if (!frame.valid) {
      console.warn('Invalid frame received:', frame.error);
      return;
    }

    /* Check if it's a response (RESPONSE_FLAG set) */
    if (frame.commandId & CMD.RESPONSE_FLAG) {
      const requestCmd = frame.commandId & ~CMD.RESPONSE_FLAG;

      /* Find matching pending request */
      for (const [seq, pending] of this.pendingRequests) {
        if (pending.commandId === requestCmd) {
          clearTimeout(pending.timer);
          this.pendingRequests.delete(seq);
          pending.resolve(frame);
          return;
        }
      }

      console.warn('Unsolicited response for command:', requestCmd.toString(16));
    }
  }

  /* =====================================================================
   * Send and Wait for Response
   * ===================================================================== */

  /**
   * Send a command and wait for the response.
   *
   * @param {number} commandId — Command ID
   * @param {Buffer|null} payload — Command payload
   * @param {number} timeoutMs — Response timeout
   * @returns {Promise<Object>} Parsed response frame
   */
  async _sendCommand(commandId, payload = null, timeoutMs = RESPONSE_TIMEOUT_MS) {
    if (!this.connected || !this.socket) {
      throw new Error('Not connected');
    }

    const frame = buildFrame(commandId, payload);
    const seq = ++this.sequenceNum;

    return new Promise((resolve, reject) => {
      /* Register pending request */
      const timer = setTimeout(() => {
        this.pendingRequests.delete(seq);
        reject(new Error(`Command 0x${commandId.toString(16)} timed out after ${timeoutMs}ms`));
      }, timeoutMs);

      this.pendingRequests.set(seq, {
        commandId,
        resolve,
        reject,
        timer,
      });

      /* Send frame */
      this.socket.send(frame, 0, frame.length, this.port, this.host, (err) => {
        if (err) {
          clearTimeout(timer);
          this.pendingRequests.delete(seq);
          reject(new Error(`Send failed: ${err.message}`));
        }
      });
    });
  }

  /**
   * Send a command with retries.
   *
   * @param {number} commandId
   * @param {Buffer|null} payload
   * @param {number} maxRetries
   * @returns {Promise<Object>}
   */
  async _sendCommandWithRetry(commandId, payload = null, maxRetries = MAX_RETRIES) {
    let lastError;

    for (let attempt = 0; attempt < maxRetries; attempt++) {
      try {
        return await this._sendCommand(commandId, payload);
      } catch (error) {
        lastError = error;
        if (attempt < maxRetries - 1) {
          /* Wait before retry */
          await new Promise(r => setTimeout(r, 200 * (attempt + 1)));
        }
      }
    }

    throw lastError;
  }

  /* =====================================================================
   * High-Level API Methods
   * ===================================================================== */

  /**
   * Ping the device and get identification info.
   * @returns {Promise<Object>} Device info
   */
  async pingDevice() {
    const response = await this._sendCommandWithRetry(CMD.PING);
    return parsePingResponse(response.payload);
  }

  /**
   * Get full device status.
   * @returns {Promise<Object>} Status object
   */
  async getFullStatus() {
    const response = await this._sendCommandWithRetry(CMD.GET_STATUS);
    return parseStatusResponse(response.payload);
  }

  /**
   * Set the active input source.
   * @param {number} source — One of INPUT_SOURCE values
   */
  async setInputSource(source) {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(source, 0);
    await this._sendCommandWithRetry(CMD.SET_INPUT_SOURCE, payload);
  }

  /**
   * Set global brightness.
   * @param {number} value — 0-65535
   */
  async setBrightness(value) {
    const payload = Buffer.alloc(2);
    payload.writeUInt16BE(Math.max(0, Math.min(65535, value)), 0);
    await this._sendCommandWithRetry(CMD.SET_BRIGHTNESS, payload);
  }

  /**
   * Set global contrast.
   * @param {number} value — 0-65535
   */
  async setContrast(value) {
    const payload = Buffer.alloc(2);
    payload.writeUInt16BE(Math.max(0, Math.min(65535, value)), 0);
    await this._sendCommandWithRetry(CMD.SET_CONTRAST, payload);
  }

  /**
   * Enable or disable HUB75E output.
   * @param {boolean} enable
   */
  async setOutputEnable(enable) {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(enable ? 1 : 0, 0);
    await this._sendCommandWithRetry(CMD.SET_OUTPUT_ENABLE, payload);
  }

  /**
   * Freeze or unfreeze the current frame.
   * @param {boolean} freeze
   */
  async freezeFrame(freeze) {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(freeze ? 1 : 0, 0);
    await this._sendCommandWithRetry(CMD.FREEZE_FRAME, payload);
  }

  /**
   * Enable a test pattern.
   * @param {number} pattern — 0=color bars, 1=gradient, 2=grid, 3=white
   */
  async enableTestPattern(pattern) {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(pattern & 0x03, 0);
    await this._sendCommandWithRetry(CMD.ENABLE_TEST_PATTERN, payload);
  }

  /**
   * Soft reset the FPGA pipeline.
   */
  async softReset() {
    await this._sendCommandWithRetry(CMD.SOFT_RESET);
  }

  /**
   * Set matrix dimensions and scan type.
   * @param {number} width — 1-256
   * @param {number} height — 1-256
   * @param {number} scan — 4, 8, 16, or 32
   */
  async setMatrixConfig(width, height, scan) {
    const payload = Buffer.alloc(5);
    payload.writeUInt16BE(width, 0);
    payload.writeUInt16BE(height, 2);
    payload.writeUInt8(scan, 4);
    await this._sendCommandWithRetry(CMD.SET_MATRIX_CONFIG, payload);
  }

  /**
   * Set dithering mode and strength.
   * @param {number} mode — 0-5
   * @param {number} strength — 0-255
   */
  async setDitherConfig(mode, strength) {
    const payload = Buffer.alloc(2);
    payload.writeUInt8(mode, 0);
    payload.writeUInt8(strength, 1);
    await this._sendCommandWithRetry(CMD.SET_DITHER_CONFIG, payload);
  }

  /**
   * Set gamma correction preset.
   * @param {number} preset — 0=linear, 1=sRGB, 2=video, 3=DCI-P3, 4=custom
   */
  async setGammaPreset(preset) {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(preset, 0);
    await this._sendCommandWithRetry(CMD.SET_GAMMA_PRESET, payload);
  }

  /**
   * Set custom gamma LUTs for all three channels.
   * @param {number} gammaR — Red gamma value (e.g., 2.2)
   * @param {number} gammaG — Green gamma value
   * @param {number} gammaB — Blue gamma value
   */
  async setGammaLUTs(gammaR, gammaG, gammaB) {
    /* Generate 256-entry LUT for each channel based on gamma value */
    const generateLut = (gamma) => {
      const lut = Buffer.alloc(512);  /* 256 × 2 bytes */
      for (let i = 0; i < 256; i++) {
        const normalized = i / 255.0;
        const corrected = Math.pow(normalized, 1.0 / gamma);
        const value = Math.round(corrected * 65535);
        lut.writeUInt16BE(Math.max(0, Math.min(65535, value)), i * 2);
      }
      return lut;
    };

    const lutR = generateLut(gammaR);
    const lutG = generateLut(gammaG);
    const lutB = generateLut(gammaB);

    const payload = Buffer.concat([lutR, lutG, lutB]);  /* 1536 bytes */
    await this._sendCommandWithRetry(CMD.SET_GAMMA_LUTS, payload);
  }

  /**
   * Set white point calibration.
   * @param {number} r — 0-255
   * @param {number} g — 0-255
   * @param {number} b — 0-255
   */
  async setWhitePoint(r, g, b) {
    const payload = Buffer.alloc(3);
    payload.writeUInt8(r, 0);
    payload.writeUInt8(g, 1);
    payload.writeUInt8(b, 2);
    await this._sendCommandWithRetry(CMD.SET_WHITE_POINT, payload);
  }

  /**
   * Set network configuration.
   * @param {string} ip — IPv4 address
   * @param {string} mask — Netmask
   * @param {string} gw — Gateway
   * @param {boolean} dhcp — Enable DHCP
   * @param {number} artnetUniverse — Starting Art-Net universe
   * @param {number} sacnPriority — sACN priority (0-200)
   */
  async setNetworkConfig(ip, mask, gw, dhcp, artnetUniverse, sacnPriority) {
    const payload = Buffer.alloc(16);

    /* Parse IP addresses */
    const ipParts = ip.split('.').map(Number);
    const maskParts = mask.split('.').map(Number);
    const gwParts = gw.split('.').map(Number);

    for (let i = 0; i < 4; i++) {
      payload.writeUInt8(ipParts[i] || 0, i);
      payload.writeUInt8(maskParts[i] || 0, 4 + i);
      payload.writeUInt8(gwParts[i] || 0, 8 + i);
    }

    payload.writeUInt8(dhcp ? 1 : 0, 12);
    payload.writeUInt16BE(artnetUniverse, 13);
    payload.writeUInt8(sacnPriority, 15);

    await this._sendCommandWithRetry(CMD.SET_NETWORK_CONFIG, payload);
  }

  /**
   * Set device identity.
   * @param {string} name — Device name (max 32 chars)
   * @param {string} location — Location string (max 64 chars)
   */
  async setDeviceIdentity(name, location) {
    const payload = Buffer.alloc(96);

    /* Write name (32 bytes, null-padded) */
    const nameBuf = Buffer.from(name.slice(0, 32), 'ascii');
    nameBuf.copy(payload, 0);
    /* Remaining bytes already zero from Buffer.alloc */

    /* Write location (64 bytes, null-padded) */
    const locBuf = Buffer.from(location.slice(0, 64), 'ascii');
    locBuf.copy(payload, 32);

    await this._sendCommandWithRetry(CMD.SET_DEVICE_IDENTITY, payload);
  }

  /**
   * Start a firmware update.
   * @param {number} type — FW_TYPE.FPGA or FW_TYPE.MCU
   * @param {number} size — Total firmware size in bytes
   * @param {string} version — Version string (e.g., "1.2.0")
   */
  async firmwareUpdateStart(type, size, version) {
    const payload = Buffer.alloc(9);
    payload.writeUInt8(type, 0);
    payload.writeUInt32BE(size, 1);

    /* Parse version: major.minor.patch */
    const parts = version.split('.').map(Number);
    payload.writeUInt8(parts[0] || 0, 5);
    payload.writeUInt8(parts[1] || 0, 6);
    payload.writeUInt8(parts[2] || 0, 7);
    payload.writeUInt8(0, 8);  /* Reserved */

    await this._sendCommandWithRetry(CMD.FIRMWARE_UPDATE_START, payload);
  }

  /**
   * Send a chunk of firmware data.
   * @param {number} offset — Byte offset in firmware image
   * @param {Buffer} data — Firmware data chunk (max 1400 bytes)
   */
  async firmwareUpdateData(offset, data) {
    if (data.length > MAX_PAYLOAD_SIZE - 4) {
      throw new Error(`Data chunk too large: ${data.length} > ${MAX_PAYLOAD_SIZE - 4}`);
    }

    const payload = Buffer.alloc(4 + data.length);
    payload.writeUInt32BE(offset, 0);
    data.copy(payload, 4);

    await this._sendCommandWithRetry(CMD.FIRMWARE_UPDATE_DATA, payload);
  }

  /**
   * Complete firmware update (triggers device reboot).
   */
  async firmwareUpdateComplete() {
    await this._sendCommandWithRetry(CMD.FIRMWARE_UPDATE_COMPLETE);
  }
}

/* =========================================================================
 * Exports
 * ========================================================================= */

export default NebulaProtocol;
export {
  CMD,
  INPUT_SOURCE,
  TEST_PATTERN,
  GAMMA_PRESET,
  DITHER_MODE,
  FW_TYPE,
  PROTOCOL_MAGIC,
  PROTOCOL_VERSION,
  DEFAULT_PORT,
  buildFrame,
  parseFrame,
  crc16Xmodem,
  parsePingResponse,
  parseStatusResponse,
};
