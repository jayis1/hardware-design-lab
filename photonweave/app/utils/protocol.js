/**
 * protocol.js — PhotonWeave Binary Wire Protocol
 *
 * Implements the complete binary communication protocol for the
 * PhotonWeave CGH Engine. Handles frame building, CRC32 verification,
 * command dispatch, and response parsing for USB and TCP transports.
 *
 * Protocol Structure:
 *   Request:  [SYNC 4B] [CMD_ID 1B] [FLAGS 1B] [LENGTH 2B] [PAYLOAD NB] [CRC32 4B]
 *   Response: [SYNC 4B] [RESP_CODE 1B] [FLAGS 1B] [LENGTH 2B] [PAYLOAD NB] [CRC32 4B]
 *
 * All multi-byte fields are little-endian.
 * Maximum payload: 1018 bytes (1024 - 6 header bytes).
 */

import { Buffer } from 'buffer';
import CRC32 from 'crc-32';

//=============================================================================
// Protocol Constants
//=============================================================================

// Frame markers
const PROTOCOL_SYNC = Buffer.from([0x50, 0x57, 0x4E, 0x56]); // "PWNV"
const PROTOCOL_MAX_PAYLOAD = 1018;
const PROTOCOL_HEADER_SIZE = 8;  // SYNC(4) + CMD(1) + FLAGS(1) + LEN(2)
const PROTOCOL_FOOTER_SIZE = 4;  // CRC32
const PROTOCOL_FRAME_MAX = PROTOCOL_HEADER_SIZE + PROTOCOL_MAX_PAYLOAD + PROTOCOL_FOOTER_SIZE;

// Command IDs
export const CMD = {
  GET_STATUS:           0x01,
  GET_TEMPERATURE:      0x02,
  GET_PERFORMANCE:      0x03,
  GET_DEVICE_INFO:      0x04,
  SET_CGH_PARAMS:       0x10,
  START_CGH:            0x11,
  ABORT_CGH:            0x12,
  GET_HOLOGRAM_INFO:    0x13,
  SUBMIT_DEPTH_MAP:     0x14,
  SET_HDMI_TIMING:      0x20,
  ENABLE_HDMI:          0x21,
  DISABLE_HDMI:         0x22,
  GET_QSFP_STATUS:      0x30,
  ENABLE_QSFP:          0x31,
  FPGA_RESET:           0xF0,
  FW_UPDATE_START:      0xF1,
  FW_UPDATE_DATA:       0xF2,
  FW_UPDATE_END:        0xF3,
  READ_REGISTER:        0xFA,
  WRITE_REGISTER:       0xFB,
  ECHO:                 0xFF,
};

// Response codes
export const RESP = {
  OK:                   0x00,
  ERROR:                0x01,
  BUSY:                 0x02,
  INVALID_CMD:          0x03,
  INVALID_PARAM:        0x04,
  TIMEOUT:              0x05,
  NOT_READY:            0x06,
  CRC_MISMATCH:         0x07,
};

// Response code names for display
const RESP_NAMES = {
  [RESP.OK]:            'OK',
  [RESP.ERROR]:         'Error',
  [RESP.BUSY]:          'Busy',
  [RESP.INVALID_CMD]:   'Invalid Command',
  [RESP.INVALID_PARAM]: 'Invalid Parameter',
  [RESP.TIMEOUT]:       'Timeout',
  [RESP.NOT_READY]:     'Not Ready',
  [RESP.CRC_MISMATCH]:  'CRC Mismatch',
};

// CGH parameter defaults
export const CGH_DEFAULTS = {
  wavelength_nm_x1000: 532000,    // 532 nm green
  depth_min_um: 0,
  depth_max_um: 100000,           // 100 mm
  depth_planes: 256,
  input_width: 2048,
  input_height: 2048,
  output_width: 3840,
  output_height: 2160,
  fft_size: 4096,
  propagation_model: 1,          // Angular Spectrum Method
  phase_quantize_bits: 8,
  color_channel: 1,              // Green
  pixel_pitch_um_x100: 360,      // 3.6 µm
  fill_factor_percent: 92,
};

// HDMI 4K60 timing defaults
export const HDMI_4K60_TIMING = {
  h_active: 3840,
  h_front_porch: 176,
  h_sync: 88,
  h_back_porch: 296,
  v_active: 2160,
  v_front_porch: 8,
  v_sync: 10,
  v_back_porch: 72,
};

//=============================================================================
// Frame Builder
//=============================================================================

/**
 * Build a protocol request frame.
 * @param {number} cmdId - Command ID from CMD enum
 * @param {number} flags - Command flags (0 for most commands)
 * @param {Buffer|null} payload - Payload data (null for no payload)
 * @returns {Buffer} Complete frame ready for transmission
 */
function buildFrame(cmdId, flags = 0, payload = null) {
  const payloadLen = payload ? payload.length : 0;

  if (payloadLen > PROTOCOL_MAX_PAYLOAD) {
    throw new Error(`Payload too large: ${payloadLen} > ${PROTOCOL_MAX_PAYLOAD}`);
  }

  // Calculate total frame size
  const frameSize = PROTOCOL_HEADER_SIZE + payloadLen + PROTOCOL_FOOTER_SIZE;
  const frame = Buffer.alloc(frameSize);

  // Write SYNC marker
  PROTOCOL_SYNC.copy(frame, 0);

  // Write command ID
  frame.writeUInt8(cmdId, 4);

  // Write flags
  frame.writeUInt8(flags, 5);

  // Write payload length (little-endian 16-bit)
  frame.writeUInt16LE(payloadLen, 6);

  // Write payload
  if (payload && payloadLen > 0) {
    payload.copy(frame, PROTOCOL_HEADER_SIZE);
  }

  // Compute and write CRC32 over [CMD_ID .. end of payload]
  const crcData = frame.subarray(4, PROTOCOL_HEADER_SIZE + payloadLen);
  const crc = CRC32.buf(crcData) >>> 0; // Unsigned 32-bit
  frame.writeUInt32LE(crc, PROTOCOL_HEADER_SIZE + payloadLen);

  return frame;
}

//=============================================================================
// Frame Parser
//=============================================================================

/**
 * Parse a protocol response frame.
 * @param {Buffer} frame - Raw response frame
 * @returns {Object} Parsed response { respCode, flags, payload, crcValid }
 * @throws {Error} On invalid frame format
 */
function parseFrame(frame) {
  if (frame.length < PROTOCOL_HEADER_SIZE + PROTOCOL_FOOTER_SIZE) {
    throw new Error(`Frame too short: ${frame.length} bytes`);
  }

  // Verify SYNC marker
  if (frame[0] !== 0x50 || frame[1] !== 0x57 ||
      frame[2] !== 0x4E || frame[3] !== 0x56) {
    throw new Error('Invalid SYNC marker');
  }

  // Read response code
  const respCode = frame.readUInt8(4);

  // Read flags
  const flags = frame.readUInt8(5);

  // Read payload length
  const payloadLen = frame.readUInt16LE(6);

  // Validate frame size
  const expectedSize = PROTOCOL_HEADER_SIZE + payloadLen + PROTOCOL_FOOTER_SIZE;
  if (frame.length < expectedSize) {
    throw new Error(`Frame size mismatch: ${frame.length} < ${expectedSize}`);
  }

  // Extract payload
  let payload = null;
  if (payloadLen > 0) {
    payload = Buffer.from(frame.subarray(PROTOCOL_HEADER_SIZE,
                                          PROTOCOL_HEADER_SIZE + payloadLen));
  }

  // Verify CRC32
  const crcData = frame.subarray(4, PROTOCOL_HEADER_SIZE + payloadLen);
  const expectedCrc = CRC32.buf(crcData) >>> 0;
  const receivedCrc = frame.readUInt32LE(PROTOCOL_HEADER_SIZE + payloadLen);
  const crcValid = (expectedCrc === receivedCrc);

  return {
    respCode,
    flags,
    payload,
    crcValid,
    respName: RESP_NAMES[respCode] || `Unknown (0x${respCode.toString(16)})`,
  };
}

//=============================================================================
// Payload Parsers — Status Structures
//=============================================================================

/**
 * Parse GET_STATUS response payload.
 * @param {Buffer} payload - 56-byte status payload
 * @returns {Object} Device status object
 */
function parseStatusPayload(payload) {
  if (!payload || payload.length < 56) {
    throw new Error('Invalid status payload length');
  }

  return {
    systemState: payload.readUInt32LE(0),
    fpgaStatus: payload.readUInt32LE(4),
    cghStatus: payload.readUInt32LE(8),
    uptimeMs: payload.readUInt32LE(12),
    temperatures: [
      payload.readInt32LE(16),   // FPGA zone 1
      payload.readInt32LE(20),   // FPGA zone 2
      payload.readInt32LE(24),   // DDR4
      payload.readInt32LE(28),   // PMIC
    ],
    powerRails: [
      payload.readUInt32LE(32),  // VCCINT (mV)
      payload.readUInt32LE(36),  // VCCAUX
      payload.readUInt32LE(40),  // VDD_DDR
      payload.readUInt32LE(44),  // MGTAVCC
      payload.readUInt32LE(48),  // MGTAVTT
      payload.readUInt32LE(52),  // VCCO_3V3
    ],
    fpsX1000: payload.readUInt32LE(56),
    frameCount: payload.readUInt32LE(60),
  };
}

/**
 * Parse GET_DEVICE_INFO response payload.
 * @param {Buffer} payload - Device info payload
 * @returns {Object} Device information
 */
function parseDeviceInfoPayload(payload) {
  if (!payload || payload.length < 32) {
    throw new Error('Invalid device info payload');
  }

  const versionRaw = payload.readUInt32LE(0);
  const serialRaw = payload.readUInt32LE(4);

  return {
    hardwareVersion: {
      major: (versionRaw >> 16) & 0xFFFF,
      minor: versionRaw & 0xFFFF,
    },
    serialNumber: serialRaw.toString(16).toUpperCase().padStart(8, '0'),
    firmwareVersion: `${payload.readUInt16LE(8)}.${payload.readUInt16LE(10)}.${payload.readUInt16LE(12)}`,
    macAddress: [
      payload[14].toString(16).padStart(2, '0'),
      payload[15].toString(16).padStart(2, '0'),
      payload[16].toString(16).padStart(2, '0'),
      payload[17].toString(16).padStart(2, '0'),
      payload[18].toString(16).padStart(2, '0'),
      payload[19].toString(16).padStart(2, '0'),
    ].join(':').toUpperCase(),
    boardRevision: `${(payload[20] >> 4) & 0x0F}.${payload[20] & 0x0F}`,
    productName: payload.subarray(21, 53).toString('utf8').replace(/\0/g, ''),
    manufacturer: payload.subarray(53, 85).toString('utf8').replace(/\0/g, ''),
  };
}

/**
 * Parse GET_PERFORMANCE response payload.
 * @param {Buffer} payload - Performance payload
 * @returns {Object} Performance metrics
 */
function parsePerformancePayload(payload) {
  if (!payload || payload.length < 28) {
    throw new Error('Invalid performance payload');
  }

  return {
    fps: payload.readUInt32LE(0) / 1000.0,
    frameTimeUs: payload.readUInt32LE(4),
    frameCount: payload.readUInt32LE(8),
    cghBusyPercent: payload.readUInt32LE(12) / 100.0,
    ddr4ReadBw: payload.readUInt32LE(16) / 100.0,    // MB/s
    ddr4WriteBw: payload.readUInt32LE(20) / 100.0,   // MB/s
    pcieRxBw: payload.readUInt32LE(24) / 100.0,      // MB/s
    activeFftEngines: payload.readUInt8(28),
  };
}

//=============================================================================
// Payload Builders — Command Payloads
//=============================================================================

/**
 * Build SET_CGH_PARAMS command payload.
 * @param {Object} params - CGH parameter object
 * @returns {Buffer} 56-byte parameter payload
 */
function buildCghParamsPayload(params) {
  const buf = Buffer.alloc(56);

  buf.writeUInt32LE(params.wavelength_nm_x1000 || CGH_DEFAULTS.wavelength_nm_x1000, 0);
  buf.writeUInt32LE(params.depth_min_um || CGH_DEFAULTS.depth_min_um, 4);
  buf.writeUInt32LE(params.depth_max_um || CGH_DEFAULTS.depth_max_um, 8);
  buf.writeUInt32LE(params.depth_planes || CGH_DEFAULTS.depth_planes, 12);
  buf.writeUInt32LE(params.input_width || CGH_DEFAULTS.input_width, 16);
  buf.writeUInt32LE(params.input_height || CGH_DEFAULTS.input_height, 20);
  buf.writeUInt32LE(params.output_width || CGH_DEFAULTS.output_width, 24);
  buf.writeUInt32LE(params.output_height || CGH_DEFAULTS.output_height, 28);
  buf.writeUInt32LE(params.fft_size || CGH_DEFAULTS.fft_size, 32);
  buf.writeUInt32LE(params.propagation_model ?? CGH_DEFAULTS.propagation_model, 36);
  buf.writeUInt32LE(params.phase_quantize_bits || CGH_DEFAULTS.phase_quantize_bits, 40);
  buf.writeUInt32LE(params.color_channel ?? CGH_DEFAULTS.color_channel, 44);
  buf.writeUInt32LE(params.pixel_pitch_um_x100 || CGH_DEFAULTS.pixel_pitch_um_x100, 48);
  buf.writeUInt32LE(params.fill_factor_percent ?? CGH_DEFAULTS.fill_factor_percent, 52);

  return buf;
}

/**
 * Build SET_HDMI_TIMING command payload.
 * @param {Object} timing - HDMI timing parameters
 * @returns {Buffer} 32-byte timing payload
 */
function buildHdmiTimingPayload(timing) {
  const buf = Buffer.alloc(32);

  buf.writeUInt32LE(timing.h_active, 0);
  buf.writeUInt32LE(timing.h_front_porch, 4);
  buf.writeUInt32LE(timing.h_sync, 8);
  buf.writeUInt32LE(timing.h_back_porch, 12);
  buf.writeUInt32LE(timing.v_active, 16);
  buf.writeUInt32LE(timing.v_front_porch, 20);
  buf.writeUInt32LE(timing.v_sync, 24);
  buf.writeUInt32LE(timing.v_back_porch, 28);

  return buf;
}

//=============================================================================
// PhotonWeaveProtocol Class
//=============================================================================

export class PhotonWeaveProtocol {
  constructor() {
    this._transport = null;
    this._connected = false;
    this._deviceInfo = null;
    this._commandQueue = [];
    this._pendingCommand = null;
    this._receiveBuffer = Buffer.alloc(0);
    this._timeoutMs = 5000;
    this._responseCallbacks = new Map();
    this._seqNumber = 0;
  }

  /**
   * Connect to the PhotonWeave device.
   * @param {Object} params - Connection parameters
   * @param {string} params.type - 'usb' or 'tcp'
   * @param {string} [params.host] - TCP host (for TCP)
   * @param {number} [params.port] - TCP port (for TCP, default 9999)
   */
  async connect(params) {
    if (this._connected) {
      await this.disconnect();
    }

    if (params.type === 'usb') {
      // USB connection via FT601
      const UsbSerialport = require('react-native-usb-serialport');
      // Note: In production, use actual USB serial port library
      this._transport = {
        type: 'usb',
        write: async (data) => {
          // USB bulk transfer
          return UsbSerialport.write(data.toString('base64'));
        },
        read: async (timeout) => {
          return UsbSerialport.read(timeout);
        },
        close: async () => {
          return UsbSerialport.close();
        },
      };
    } else if (params.type === 'tcp') {
      // TCP connection
      const TcpSocket = require('react-native-tcp-socket');
      const host = params.host || '192.168.1.100';
      const port = params.port || 9999;

      this._transport = await new Promise((resolve, reject) => {
        const client = TcpSocket.createConnection({ host, port }, () => {
          resolve({
            type: 'tcp',
            socket: client,
            write: async (data) => {
              return new Promise((wResolve, wReject) => {
                client.write(data, (err) => {
                  if (err) wReject(err);
                  else wResolve();
                });
              });
            },
            read: async (timeout) => {
              return new Promise((rResolve) => {
                client.once('data', (data) => {
                  rResolve(Buffer.from(data));
                });
                setTimeout(() => rResolve(null), timeout);
              });
            },
            close: async () => {
              client.destroy();
            },
          });
        });
        client.on('error', reject);
      });
    } else {
      throw new Error(`Unknown transport type: ${params.type}`);
    }

    this._connected = true;

    // Verify connection with echo command
    const echoResult = await this.sendCommand(CMD.ECHO, Buffer.from([0x42]));
    if (echoResult.respCode !== RESP.OK) {
      await this._transport.close();
      this._connected = false;
      throw new Error('Device echo test failed');
    }
  }

  /**
   * Disconnect from the device.
   */
  async disconnect() {
    if (this._transport) {
      await this._transport.close();
      this._transport = null;
    }
    this._connected = false;
    this._deviceInfo = null;
    this._receiveBuffer = Buffer.alloc(0);
    this._responseCallbacks.clear();
  }

  /**
   * Check if connected.
   * @returns {boolean}
   */
  isConnected() {
    return this._connected;
  }

  /**
   * Send a command and wait for response.
   * @param {number} cmdId - Command ID
   * @param {Buffer|null} payload - Command payload
   * @param {number} [timeoutMs] - Response timeout
   * @returns {Object} Parsed response
   */
  async sendCommand(cmdId, payload = null, timeoutMs = null) {
    if (!this._connected) {
      throw new Error('Not connected');
    }

    const timeout = timeoutMs || this._timeoutMs;
    const frame = buildFrame(cmdId, 0, payload);

    // Send frame
    await this._transport.write(frame);

    // Read response
    const startTime = Date.now();
    let responseBuffer = Buffer.alloc(0);

    while ((Date.now() - startTime) < timeout) {
      const chunk = await this._transport.read(timeout - (Date.now() - startTime));
      if (chunk) {
        responseBuffer = Buffer.concat([responseBuffer, chunk]);

        // Try to parse a complete frame
        if (responseBuffer.length >= PROTOCOL_HEADER_SIZE + PROTOCOL_FOOTER_SIZE) {
          // Check for SYNC
          const syncIdx = responseBuffer.indexOf(PROTOCOL_SYNC);
          if (syncIdx >= 0) {
            // Read payload length from header
            if (responseBuffer.length >= syncIdx + PROTOCOL_HEADER_SIZE) {
              const payloadLen = responseBuffer.readUInt16LE(syncIdx + 6);
              const totalFrameSize = PROTOCOL_HEADER_SIZE + payloadLen + PROTOCOL_FOOTER_SIZE;

              if (responseBuffer.length >= syncIdx + totalFrameSize) {
                const frame = responseBuffer.subarray(syncIdx, syncIdx + totalFrameSize);
                const parsed = parseFrame(frame);

                if (!parsed.crcValid) {
                  throw new Error('CRC verification failed');
                }

                return parsed;
              }
            }
          }
        }
      }
    }

    throw new Error(`Command 0x${cmdId.toString(16)} timed out after ${timeout}ms`);
  }

  //===========================================================================
  // High-Level API Methods
  //===========================================================================

  /**
   * Get full device status.
   * @returns {Object} Device status
   */
  async getStatus() {
    const resp = await this.sendCommand(CMD.GET_STATUS);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`GET_STATUS failed: ${resp.respName}`);
    }
    return parseStatusPayload(resp.payload);
  }

  /**
   * Get device information (version, serial, MAC, etc.).
   * @returns {Object} Device info
   */
  async getDeviceInfo() {
    const resp = await this.sendCommand(CMD.GET_DEVICE_INFO);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`GET_DEVICE_INFO failed: ${resp.respName}`);
    }
    const info = parseDeviceInfoPayload(resp.payload);
    this._deviceInfo = info;
    return info;
  }

  /**
   * Get performance metrics.
   * @returns {Object} Performance data
   */
  async getPerformance() {
    const resp = await this.sendCommand(CMD.GET_PERFORMANCE);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`GET_PERFORMANCE failed: ${resp.respName}`);
    }
    return parsePerformancePayload(resp.payload);
  }

  /**
   * Get temperature readings.
   * @returns {number[]} Array of 4 temperatures in millidegrees C
   */
  async getTemperatures() {
    const resp = await this.sendCommand(CMD.GET_TEMPERATURE);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`GET_TEMPERATURE failed: ${resp.respName}`);
    }
    return [
      resp.payload.readInt32LE(0),
      resp.payload.readInt32LE(4),
      resp.payload.readInt32LE(8),
      resp.payload.readInt32LE(12),
    ];
  }

  /**
   * Configure CGH parameters.
   * @param {Object} params - CGH parameters
   * @param {number} [paramSet=0] - Parameter set index (0-15)
   */
  async setCghParams(params, paramSet = 0) {
    const payload = buildCghParamsPayload(params);
    // Prepend param set index
    const fullPayload = Buffer.alloc(payload.length + 1);
    fullPayload.writeUInt8(paramSet, 0);
    payload.copy(fullPayload, 1);

    const resp = await this.sendCommand(CMD.SET_CGH_PARAMS, fullPayload);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`SET_CGH_PARAMS failed: ${resp.respName}`);
    }
  }

  /**
   * Start CGH pipeline processing.
   */
  async startCgh() {
    const resp = await this.sendCommand(CMD.START_CGH);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`START_CGH failed: ${resp.respName}`);
    }
  }

  /**
   * Abort current CGH frame processing.
   */
  async abortCgh() {
    const resp = await this.sendCommand(CMD.ABORT_CGH);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`ABORT_CGH failed: ${resp.respName}`);
    }
  }

  /**
   * Enable HDMI output.
   * @param {Object} [timing] - Optional custom timing parameters
   */
  async enableHdmi(timing = null) {
    if (timing) {
      const timingPayload = buildHdmiTimingPayload(timing);
      await this.sendCommand(CMD.SET_HDMI_TIMING, timingPayload);
    }
    const resp = await this.sendCommand(CMD.ENABLE_HDMI);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`ENABLE_HDMI failed: ${resp.respName}`);
    }
  }

  /**
   * Disable HDMI output.
   */
  async disableHdmi() {
    const resp = await this.sendCommand(CMD.DISABLE_HDMI);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`DISABLE_HDMI failed: ${resp.respName}`);
    }
  }

  /**
   * Get QSFP+ module status.
   * @returns {Object} QSFP+ status
   */
  async getQsfpStatus() {
    const resp = await this.sendCommand(CMD.GET_QSFP_STATUS);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`GET_QSFP_STATUS failed: ${resp.respName}`);
    }
    return {
      modulePresent: resp.payload.readUInt8(0) !== 0,
      linkUp: resp.payload.readUInt8(1) !== 0,
      rxActive: resp.payload.readUInt8(2) !== 0,
      rxFrameCount: resp.payload.readUInt32LE(4),
      rxErrorCount: resp.payload.readUInt32LE(8),
      linkSpeedMbps: resp.payload.readUInt32LE(12),
    };
  }

  /**
   * Refresh device status (called on app foreground).
   */
  async refreshStatus() {
    try {
      await this.getStatus();
    } catch (error) {
      console.warn('Status refresh failed:', error.message);
    }
  }

  /**
   * Read a raw FPGA register.
   * @param {number} offset - Register offset from BAR0
   * @returns {number} 32-bit register value
   */
  async readRegister(offset) {
    const payload = Buffer.alloc(4);
    payload.writeUInt32LE(offset, 0);
    const resp = await this.sendCommand(CMD.READ_REGISTER, payload);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`READ_REGISTER failed: ${resp.respName}`);
    }
    return resp.payload.readUInt32LE(0);
  }

  /**
   * Write a raw FPGA register.
   * @param {number} offset - Register offset from BAR0
   * @param {number} value - 32-bit value to write
   */
  async writeRegister(offset, value) {
    const payload = Buffer.alloc(8);
    payload.writeUInt32LE(offset, 0);
    payload.writeUInt32LE(value, 4);
    const resp = await this.sendCommand(CMD.WRITE_REGISTER, payload);
    if (resp.respCode !== RESP.OK) {
      throw new Error(`WRITE_REGISTER failed: ${resp.respName}`);
    }
  }
}

//=============================================================================
// Utility Exports
//=============================================================================

export { buildFrame, parseFrame, parseStatusPayload, parseDeviceInfoPayload,
         parsePerformancePayload, buildCghParamsPayload, buildHdmiTimingPayload,
         PROTOCOL_SYNC, PROTOCOL_MAX_PAYLOAD, RESP_NAMES };
