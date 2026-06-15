/**
 * utils/protocol.js — Binary wire protocol for PicoRadar
 *
 * Defines the binary protocol between the companion app and the
 * PicoRadar device over BLE or USB CDC. Includes:
 *  - Frame structure with sync bytes, length, command ID, payload, CRC-16
 *  - Command IDs for all operations
 *  - Frame builder (TX) and parser (RX)
 *  - CRC-16/CCITT calculation
 *
 * Frame format:
 *  [0x50 0x52] [LEN_U16] [CMD_U8] [PAYLOAD...] [CRC_U16]
 *  Sync="PR"   Length    Command  Data bytes   CRC-16
 */

/* ---- Protocol Constants ---- */
export const FRAME_SYNC_0 = 0x50; // 'P'
export const FRAME_SYNC_1 = 0x52; // 'R'
export const FRAME_HEADER_SIZE = 5;  // sync(2) + len(2) + cmd(1)
export const FRAME_CRC_SIZE = 2;
export const FRAME_OVERHEAD = FRAME_HEADER_SIZE + FRAME_CRC_SIZE; // 7 bytes
export const MAX_PAYLOAD_SIZE = 512;
export const MAX_FRAME_SIZE = FRAME_OVERHEAD + MAX_PAYLOAD_SIZE;

/* ---- Command IDs ---- */
export const ProtocolCommands = {
  // Radar control
  CMD_RADAR_START:      0x01,
  CMD_RADAR_STOP:       0x02,
  CMD_SET_PROFILE:      0x03,
  CMD_GET_PROFILE:      0x04,

  // Point cloud data (device → app)
  CMD_POINT_CLOUD:      0x10,
  CMD_POINT_CLOUD_COMP: 0x11,  // Compressed

  // IMU data (device → app)
  CMD_IMU_DATA:         0x20,
  CMD_SET_IMU_ODR:      0x21,

  // Display
  CMD_SET_OLED:         0x30,
  CMD_GET_OLED:         0x31,

  // WiFi
  CMD_SET_WIFI:         0x40,
  CMD_GET_WIFI_STATUS:  0x41,

  // Ethernet / MQTT
  CMD_SET_MQTT:         0x50,
  CMD_GET_ETH_STATUS:   0x51,

  // Session / Logging
  CMD_LOG_START:        0x60,
  CMD_LOG_STOP:         0x61,
  CMD_EXPORT_CSV:       0x62,
  CMD_SESSION_STATS:    0x63,

  // System
  CMD_REBOOT:           0xF0,
  CMD_FW_VERSION:      0xF1,
  CMD_OTA_UPDATE:      0xF2,
  CMD_PING:            0xFF,
};

/* ---- Point Cloud Frame Payload Format ----
 * CMD_POINT_CLOUD payload:
 *   [frameNum_U32] [numPoints_U16] [ts_U32] [points...]
 *   Each point: [range_F32] [angle_F32] [velocity_F32] [intensity_F32] = 16 bytes
 *   Total: 10 + numPoints * 16 bytes
 */

/* ---- CRC-16/CCITT ---- */
const CRC16_TABLE = (() => {
  const table = new Uint16Array(256);
  for (let i = 0; i < 256; i++) {
    let crc = i << 8;
    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
      } else {
        crc = (crc << 1) & 0xFFFF;
      }
    }
    table[i] = crc;
  }
  return table;
})();

export function crc16(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc = ((crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ data[i]) & 0xFF]) & 0xFFFF;
  }
  return crc;
}

/* ---- Frame Builder ---- */
export function buildFrame(cmd, payload = null) {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = FRAME_OVERHEAD + payloadLen;

  if (payloadLen > MAX_PAYLOAD_SIZE) {
    throw new Error(`Payload too large: ${payloadLen} > ${MAX_PAYLOAD_SIZE}`);
  }

  const frame = new Uint8Array(totalLen);
  const lenField = FRAME_CRC_SIZE + payloadLen + 1; // CRC + payload + cmd byte

  // Header
  frame[0] = FRAME_SYNC_0;
  frame[1] = FRAME_SYNC_1;
  frame[2] = (lenField >> 8) & 0xFF;  // Length high byte
  frame[3] = lenField & 0xFF;          // Length low byte
  frame[4] = cmd;

  // Payload
  if (payload && payloadLen > 0) {
    frame.set(payload, FRAME_HEADER_SIZE);
  }

  // CRC over [header + payload] (everything except CRC itself)
  const crc = crc16(frame.slice(0, totalLen - FRAME_CRC_SIZE));
  frame[totalLen - 2] = (crc >> 8) & 0xFF;
  frame[totalLen - 1] = crc & 0xFF;

  return frame;
}

/* ---- Frame Parser ---- */
export class FrameParser {
  constructor() {
    this.buffer = new Uint8Array(MAX_FRAME_SIZE * 2);
    this.bufLen = 0;
    this.callbacks = {};
  }

  on(command, callback) {
    if (!this.callbacks[command]) this.callbacks[command] = [];
    this.callbacks[command].push(callback);
  }

  off(command, callback) {
    if (!this.callbacks[command]) return;
    this.callbacks[command] = this.callbacks[command].filter((cb) => cb !== callback);
  }

  /** Feed raw bytes from BLE/USB into the parser */
  feed(data) {
    // Append to buffer
    this.buffer.set(data, this.bufLen);
    this.bufLen += data.length;

    // Try to parse complete frames
    while (this.bufLen >= FRAME_OVERHEAD) {
      // Look for sync bytes
      let syncPos = -1;
      for (let i = 0; i <= this.bufLen - 2; i++) {
        if (this.buffer[i] === FRAME_SYNC_0 && this.buffer[i + 1] === FRAME_SYNC_1) {
          syncPos = i;
          break;
        }
      }

      if (syncPos === -1) {
        // No sync found — discard all but last byte (could be start of sync)
        this.bufLen = 0;
        return;
      }

      // Discard bytes before sync
      if (syncPos > 0) {
        this.buffer.copyWithin(0, syncPos, this.bufLen);
        this.bufLen -= syncPos;
      }

      if (this.bufLen < FRAME_HEADER_SIZE) return; // Need more header bytes

      // Parse length field
      const lenField = (this.buffer[2] << 8) | this.buffer[3];
      const totalFrameLen = FRAME_HEADER_SIZE - 1 + lenField; // sync(2) + len(2) + lenField(cmd+payload+crc)

      if (totalFrameLen > MAX_FRAME_SIZE) {
        // Invalid frame — discard sync and retry
        this.buffer.copyWithin(0, 2, this.bufLen);
        this.bufLen -= 2;
        continue;
      }

      if (this.bufLen < totalFrameLen) return; // Need more data

      // Extract frame
      const frame = this.buffer.slice(0, totalFrameLen);
      const cmd = frame[4];
      const payload = frame.slice(FRAME_HEADER_SIZE, totalFrameLen - FRAME_CRC_SIZE);

      // Verify CRC
      const receivedCrc = (frame[totalFrameLen - 2] << 8) | frame[totalFrameLen - 1];
      const calculatedCrc = crc16(frame.slice(0, totalFrameLen - FRAME_CRC_SIZE));

      if (receivedCrc === calculatedCrc) {
        // Valid frame — dispatch to callbacks
        const cbs = this.callbacks[cmd] || [];
        cbs.forEach((cb) => cb(payload));
      }
      // else: CRC mismatch — discard

      // Remove processed frame from buffer
      this.buffer.copyWithin(0, totalFrameLen, this.bufLen);
      this.bufLen -= totalFrameLen;
    }
  }
}

/* ---- High-level Protocol Manager ---- */
export class ProtocolManager {
  constructor() {
    this.parser = new FrameParser();
    this.connected = false;
    this.bleDevice = null;
    this.bleTxChar = null;
    this.pointCloudCallbacks = [];
    this.imuDataCallbacks = [];
    this.sessionStatsCallbacks = [];
    this.connectionCallbacks = [];

    // Register parser callbacks
    this.parser.on(ProtocolCommands.CMD_POINT_CLOUD, (payload) => {
      const frame = this.parsePointCloud(payload);
      this.pointCloudCallbacks.forEach((cb) => cb(frame));
    });

    this.parser.on(ProtocolCommands.CMD_IMU_DATA, (payload) => {
      const data = this.parseImuData(payload);
      this.imuDataCallbacks.forEach((cb) => cb(data));
    });

    this.parser.on(ProtocolCommands.CMD_SESSION_STATS, (payload) => {
      const stats = this.parseSessionStats(payload);
      this.sessionStatsCallbacks.forEach((cb) => cb(stats));
    });

    this.parser.on(ProtocolCommands.CMD_PING, () => {
      // Pong received — connection alive
    });
  }

  setBleDevice(device, txChar) {
    this.bleDevice = device;
    this.bleTxChar = txChar;
  }

  setConnected(isConnected) {
    this.connected = isConnected;
    this.connectionCallbacks.forEach((cb) => cb(isConnected));
  }

  disconnect() {
    if (this.bleDevice) {
      this.bleDevice.cancelConnection();
      this.bleDevice = null;
      this.bleTxChar = null;
    }
    this.connected = false;
    this.connectionCallbacks.forEach((cb) => cb(false));
  }

  feedData(rawBytes) {
    this.parser.feed(rawBytes);
  }

  sendCommand(cmd, params = null) {
    let payload = null;
    if (params) {
      payload = this.encodePayload(cmd, params);
    }
    const frame = buildFrame(cmd, payload);
    // In real implementation, write to BLE characteristic or USB endpoint
    if (this.bleTxChar && this.bleDevice) {
      // Convert to base64 for BLE characteristic write
      const base64 = this.arrayBufferToBase64(frame);
      this.bleTxChar.writeWithResponse(base64);
    }
  }

  onPointCloud(cb) { this.pointCloudCallbacks.push(cb); }
  offPointCloud(cb) { this.pointCloudCallbacks = this.pointCloudCallbacks.filter((c) => c !== cb); }
  onImuData(cb) { this.imuDataCallbacks.push(cb); }
  offImuData(cb) { this.imuDataCallbacks = this.imuDataCallbacks.filter((c) => c !== cb); }
  onSessionStats(cb) { this.sessionStatsCallbacks.push(cb); }
  offSessionStats(cb) { this.sessionStatsCallbacks = this.sessionStatsCallbacks.filter((c) => c !== cb); }
  onConnectionChange(cb) { this.connectionCallbacks.push(cb); }

  /* ---- Payload Parsers ---- */

  parsePointCloud(payload) {
    if (payload.length < 10) return { points: [] };
    const view = new DataView(payload.buffer, payload.byteOffset, payload.length);
    const frameNum = view.getUint32(0, true);
    const numPoints = view.getUint16(4, true);
    const timestamp = view.getUint32(6, true);

    const points = [];
    for (let i = 0; i < numPoints && (10 + i * 16) <= payload.length - 16; i++) {
      const offset = 10 + i * 16;
      points.push({
        range: view.getFloat32(offset, true),
        angle: view.getFloat32(offset + 4, true),
        velocity: view.getFloat32(offset + 8, true),
        intensity: view.getFloat32(offset + 12, true),
      });
    }

    return { frameNum, timestamp, points };
  }

  parseImuData(payload) {
    if (payload.length < 28) return {};
    const view = new DataView(payload.buffer, payload.byteOffset, payload.length);
    return {
      accelX: view.getFloat32(0, true),
      accelY: view.getFloat32(4, true),
      accelZ: view.getFloat32(8, true),
      gyroX: view.getFloat32(12, true),
      gyroY: view.getFloat32(16, true),
      gyroZ: view.getFloat32(20, true),
      temperature: view.getFloat32(24, true),
    };
  }

  parseSessionStats(payload) {
    if (payload.length < 24) return {};
    const view = new DataView(payload.buffer, payload.byteOffset, payload.length);
    return {
      totalPoints: view.getUint32(0, true),
      totalFrames: view.getUint32(4, true),
      avgPointsPerFrame: view.getFloat32(8, true),
      maxRange: view.getFloat32(12, true),
      maxVelocity: view.getFloat32(16, true),
      duration: view.getFloat32(20, true),
    };
  }

  /* ---- Payload Encoders ---- */

  encodePayload(cmd, params) {
    switch (cmd) {
      case ProtocolCommands.CMD_SET_PROFILE: {
        const buf = new Uint8Array(10);
        const view = new DataView(buf.buffer);
        view.setUint8(0, params.profileId);
        view.setUint32(1, params.bandwidth, true);
        view.setUint16(5, params.chirps, true);
        view.setUint16(7, params.framePeriodMs, true);
        return buf;
      }
      case ProtocolCommands.CMD_SET_OLED: {
        const buf = new Uint8Array(2);
        buf[0] = params.enabled ? 1 : 0;
        buf[1] = params.contrast;
        return buf;
      }
      case ProtocolCommands.CMD_SET_WIFI: {
        const ssidBytes = new TextEncoder().encode(params.ssid);
        const passBytes = new TextEncoder().encode(params.password);
        const buf = new Uint8Array(1 + 1 + ssidBytes.length + 1 + passBytes.length);
        let offset = 0;
        buf[offset++] = params.enabled ? 1 : 0;
        buf[offset++] = ssidBytes.length;
        buf.set(ssidBytes, offset); offset += ssidBytes.length;
        buf[offset++] = passBytes.length;
        buf.set(passBytes, offset);
        return buf;
      }
      case ProtocolCommands.CMD_SET_IMU_ODR: {
        const buf = new Uint8Array(1);
        buf[0] = params.odr;
        return buf;
      }
      default:
        return null;
    }
  }

  arrayBufferToBase64(buffer) {
    let binary = '';
    const bytes = new Uint8Array(buffer);
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }
}