/**
 * SkyPilot — Binary Wire Protocol
 * CRC-16/CCITT, command IDs, frame builder/parser
 *
 * Frame format:
 *   [START 0xAA] [START 0x55] [LEN_H] [LEN_L] [CMD] [SEQ] [PAYLOAD...] [CRC_H] [CRC_L]
 *
 * LEN = length of CMD + SEQ + PAYLOAD (2 + payload_length)
 * CRC = CRC-16/CCITT over [LEN_H, LEN_L, CMD, SEQ, ...PAYLOAD]
 */

// ---- Command IDs ----
export const Commands = {
  // System
  PING:                   0x01,
  PONG:                   0x02,
  DEVICE_INFO_REQUEST:    0x03,
  DEVICE_INFO_REPORT:     0x04,
  REBOOT:                 0x05,

  // Telemetry
  TELEMETRY_REQUEST:      0x10,
  ATTITUDE_REPORT:        0x11,
  MOTOR_REPORT:           0x12,
  BATTERY_REPORT:         0x13,
  GPS_REPORT:             0x14,
  IMU1_REPORT:            0x15,
  IMU2_REPORT:            0x16,
  BARO_REPORT:            0x17,
  LTE_REPORT:             0x18,

  // Control
  ARM:                    0x20,
  DISARM:                 0x21,
  SET_MODE:               0x22,
  SET_ARMED:              0x23,

  // Calibration
  CALIBRATE_IMU:          0x30,
  CALIBRATE_BARO:        0x31,
  CALIBRATE_ESC:          0x32,
  CALIBRATE_RC:           0x33,

  // Configuration
  SET_IMU_RANGE:          0x40,
  SET_IMU_RATE:           0x41,
  SET_DSHOT_MODE:         0x42,
  SET_GPS_BAUD:           0x43,

  // LTE
  LTE_CONNECT:            0x50,
  LTE_DISCONNECT:         0x51,
  LTE_STATUS:             0x52,

  // MAVLink relay
  MAVLINK_FORWARD:        0x60,

  // OTA
  FIRMWARE_UPDATE:        0x70,
  FIRMWARE_CHUNK:         0x71,
  FIRMWARE_VERIFY:        0x72,

  // Error
  ERROR_REPORT:           0xFF,
};

// ---- Constants ----
const FRAME_START_0 = 0xAA;
const FRAME_START_1 = 0x55;
const HEADER_SIZE = 6;   // START0 + START1 + LEN_H + LEN_L + CMD + SEQ
const CRC_SIZE = 2;
const MAX_PAYLOAD_SIZE = 512;
const MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE;

// ---- CRC-16/CCITT ----
const CRC16_TABLE = new Uint16Array(256);

// Initialize CRC-16/CCITT lookup table (polynomial 0x1021)
for (let i = 0; i < 256; i++) {
  let crc = i << 8;
  for (let j = 0; j < 8; j++) {
    if (crc & 0x8000) {
      crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
    } else {
      crc = (crc << 1) & 0xFFFF;
    }
  }
  CRC16_TABLE[i] = crc;
}

function crc16Ccitt(data, offset, length) {
  let crc = 0xFFFF;  // Initial value for CRC-16/CCITT
  for (let i = offset; i < offset + length; i++) {
    crc = ((crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ data[i]) & 0xFF]) & 0xFFFF;
  }
  return crc;
}

// ---- Frame Builder ----
export class FrameBuilder {
  constructor() {
    this.seq = 0;
  }

  /**
   * Build a binary frame from command and payload
   * @param {number} cmd - Command ID (Commands enum)
   * @param {object} payload - Payload object to serialize
   * @returns {Uint8Array} - Complete binary frame
   */
  build(cmd, payload) {
    const payloadBytes = this._serializePayload(cmd, payload);
    const len = 2 + payloadBytes.length; // CMD(1) + SEQ(1) + PAYLOAD

    const frame = new Uint8Array(HEADER_SIZE + payloadBytes.length + CRC_SIZE);

    // Header
    frame[0] = FRAME_START_0;            // 0xAA
    frame[1] = FRAME_START_1;            // 0x55
    frame[2] = (len >> 8) & 0xFF;        // LEN high byte
    frame[3] = len & 0xFF;               // LEN low byte
    frame[4] = cmd & 0xFF;               // CMD
    frame[5] = this.seq & 0xFF;          // SEQ
    this.seq = (this.seq + 1) & 0xFF;

    // Payload
    frame.set(payloadBytes, HEADER_SIZE);

    // CRC over [LEN_H, LEN_L, CMD, SEQ, ...PAYLOAD]
    const crc = crc16Ccitt(frame, 2, 2 + len);
    frame[HEADER_SIZE + payloadBytes.length] = (crc >> 8) & 0xFF;     // CRC high
    frame[HEADER_SIZE + payloadBytes.length + 1] = crc & 0xFF;         // CRC low

    return frame;
  }

  /**
   * Serialize a payload object into bytes based on command type
   */
  _serializePayload(cmd, payload) {
    switch (cmd) {
      case Commands.TELEMETRY_REQUEST: {
        // Fields bitmask: 1=attitude, 2=motors, 4=battery, 8=gps, 16=lte, 32=imu1, 64=imu2, 128=baro
        let fieldsMask = 0;
        if (payload.fields) {
          if (payload.fields.includes('attitude')) fieldsMask |= 1;
          if (payload.fields.includes('motors')) fieldsMask |= 2;
          if (payload.fields.includes('battery')) fieldsMask |= 4;
          if (payload.fields.includes('gps')) fieldsMask |= 8;
          if (payload.fields.includes('lte')) fieldsMask |= 16;
          if (payload.fields.includes('imu1')) fieldsMask |= 32;
          if (payload.fields.includes('imu2')) fieldsMask |= 64;
          if (payload.fields.includes('baro')) fieldsMask |= 128;
        }
        return new Uint8Array([fieldsMask & 0xFF]);
      }

      case Commands.ARM:
      case Commands.DISARM:
      case Commands.CALIBRATE_BARO:
        return new Uint8Array(0);

      case Commands.SET_MODE: {
        const modeMap = {
          'STABILIZE': 0, 'ALTITUDE': 1, 'POSITION': 2,
          'LOITER': 3, 'AUTO': 4, 'RTL': 5,
        };
        const mode = modeMap[payload.mode] || 0;
        return new Uint8Array([mode]);
      }

      case Commands.CALIBRATE_IMU: {
        const imuMap = { '1': 1, '2': 2, 'both': 3 };
        const imu = imuMap[payload.imu] || 3;
        return new Uint8Array([imu]);
      }

      case Commands.CALIBRATE_ESC: {
        const dshotMap = { 'DShot150': 1, 'DShot300': 2, 'DShot600': 3, 'DShot1200': 4 };
        const mode = dshotMap[payload.mode] || 4;
        return new Uint8Array([mode]);
      }

      case Commands.LTE_CONNECT: {
        const apnBytes = this._stringToBytes(payload.apn || '');
        const serverBytes = this._stringToBytes(payload.server || '');
        const portBytes = this._uint16ToBytes(payload.port || 14550);
        const data = new Uint8Array(1 + apnBytes.length + 1 + serverBytes.length + 2);
        let offset = 0;
        data[offset++] = apnBytes.length;
        data.set(apnBytes, offset); offset += apnBytes.length;
        data[offset++] = serverBytes.length;
        data.set(serverBytes, offset); offset += serverBytes.length;
        data.set(portBytes, offset);
        return data;
      }

      case Commands.PING:
      case Commands.DEVICE_INFO_REQUEST:
      case Commands.LTE_DISCONNECT:
      case Commands.LTE_STATUS:
        return new Uint8Array(0);

      default:
        // For unknown commands, try to JSON-serialize the payload
        if (payload && Object.keys(payload).length > 0) {
          const jsonStr = JSON.stringify(payload);
          return this._stringToBytes(jsonStr);
        }
        return new Uint8Array(0);
    }
  }

  _stringToBytes(str) {
    const encoder = new TextEncoder();
    return encoder.encode(str);
  }

  _uint16ToBytes(val) {
    return new Uint8Array([(val >> 8) & 0xFF, val & 0xFF]);
  }
}

// ---- Frame Parser ----
export class FrameParser {
  constructor() {
    this.buffer = new Uint8Array(MAX_FRAME_SIZE * 2);
    this.bufferLen = 0;
  }

  /**
   * Parse incoming data and return decoded messages
   * @param {Uint8Array|ArrayBuffer} data - Raw binary data
   * @returns {object|null} - Decoded message or null if incomplete
   */
  parse(data) {
    const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;

    // Append to buffer
    if (this.bufferLen + bytes.length > this.buffer.length) {
      // Shift buffer
      this.buffer.copyWithin(0, this.bufferLen - MAX_FRAME_SIZE);
      this.bufferLen = MAX_FRAME_SIZE;
    }
    this.buffer.set(bytes, this.bufferLen);
    this.bufferLen += bytes.length;

    // Look for frame start
    let startIdx = -1;
    for (let i = 0; i < this.bufferLen - 1; i++) {
      if (this.buffer[i] === FRAME_START_0 && this.buffer[i + 1] === FRAME_START_1) {
        startIdx = i;
        break;
      }
    }

    if (startIdx === -1) {
      // No start found, discard
      this.bufferLen = 0;
      return null;
    }

    // Need at least header + CRC
    if (this.bufferLen < startIdx + HEADER_SIZE + CRC_SIZE) {
      return null; // Incomplete
    }

    // Parse header
    const len = (this.buffer[startIdx + 2] << 8) | this.buffer[startIdx + 3];
    const cmd = this.buffer[startIdx + 4];
    const seq = this.buffer[startIdx + 5];

    const payloadLen = len - 2; // Subtract CMD + SEQ
    const totalFrameLen = HEADER_SIZE + payloadLen + CRC_SIZE;

    if (this.bufferLen < startIdx + totalFrameLen) {
      return null; // Incomplete
    }

    // Verify CRC
    const crcOffset = startIdx + HEADER_SIZE + payloadLen;
    const expectedCrc = (this.buffer[crcOffset] << 8) | this.buffer[crcOffset + 1];
    const calculatedCrc = crc16Ccitt(this.buffer, startIdx + 2, 2 + len);

    if (expectedCrc !== calculatedCrc) {
      // CRC mismatch, discard frame
      this.buffer.copyWithin(0, startIdx + totalFrameLen);
      this.bufferLen -= startIdx + totalFrameLen;
      return null;
    }

    // Extract payload
    const payloadBytes = this.buffer.slice(
      startIdx + HEADER_SIZE,
      startIdx + HEADER_SIZE + payloadLen
    );

    // Shift buffer
    this.buffer.copyWithin(0, startIdx + totalFrameLen);
    this.bufferLen -= startIdx + totalFrameLen;

    // Decode payload
    const payload = this._deserializePayload(cmd, payloadBytes);

    return { cmd, seq, payload };
  }

  /**
   * Deserialize payload bytes into a structured object based on command type
   */
  _deserializePayload(cmd, bytes) {
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);

    switch (cmd) {
      case Commands.ATTITUDE_REPORT: {
        // Payload: roll(f32), pitch(f32), yaw(f32)
        return {
          roll_deg: view.getFloat32(0, true),
          pitch_deg: view.getFloat32(4, true),
          yaw_deg: view.getFloat32(8, true),
        };
      }

      case Commands.MOTOR_REPORT: {
        // Payload: 12 × uint8 throttle values
        const motors = [];
        for (let i = 0; i < 12; i++) {
          motors.push(bytes[i] || 0);
        }
        return { motors };
      }

      case Commands.BATTERY_REPORT: {
        // Payload: voltage(f32), current(f32), percentage(u8)
        return {
          voltage: view.getFloat32(0, true),
          current: view.getFloat32(4, true),
          percentage: bytes[8],
        };
      }

      case Commands.GPS_REPORT: {
        // Payload: lat(f64), lon(f64), alt(f32), satellites(u8), fix_type(u8), hdop(f32), speed(f32), heading(f32)
        return {
          lat: view.getFloat64(0, true),
          lon: view.getFloat64(8, true),
          alt: view.getFloat32(16, true),
          satellites: bytes[20],
          fix_type: bytes[21],
          hdop: view.getFloat32(22, true),
          speed: view.getFloat32(26, true),
          heading: view.getFloat32(30, true),
        };
      }

      case Commands.IMU1_REPORT:
      case Commands.IMU2_REPORT: {
        // Payload: ax(f32), ay(f32), az(f32), gx(f32), gy(f32), gz(f32), temp(f32)
        return {
          ax: view.getFloat32(0, true),
          ay: view.getFloat32(4, true),
          az: view.getFloat32(8, true),
          gx: view.getFloat32(12, true),
          gy: view.getFloat32(16, true),
          gz: view.getFloat32(20, true),
          temp: view.getFloat32(24, true),
        };
      }

      case Commands.BARO_REPORT: {
        // Payload: pressure(f32), altitude(f32), temp(f32)
        return {
          pressure: view.getFloat32(0, true),
          altitude: view.getFloat32(4, true),
          temp: view.getFloat32(8, true),
        };
      }

      case Commands.LTE_REPORT: {
        // Payload: connected(u8), rssi(i8), rsrp(i8), rsrq(i8), technology(u8), uptime(u32), up_rate(f32), down_rate(f32)
        const operatorLen = bytes[14];
        const operatorStr = new TextDecoder().decode(
          bytes.slice(15, 15 + operatorLen)
        );
        return {
          connected: bytes[0] !== 0,
          rssi: bytes[1] > 127 ? bytes[1] - 256 : bytes[1],
          rsrp: bytes[2] > 127 ? bytes[2] - 256 : bytes[2],
          rsrq: bytes[3] > 127 ? bytes[3] - 256 : bytes[3],
          technology: ['No Service', 'GSM', 'UMTS', 'LTE', 'LTE-A'][bytes[4]] || 'Unknown',
          uptime: view.getUint32(5, true),
          dataRate: {
            up: view.getFloat32(9, true),
            down: view.getFloat32(13, true),
          },
          operator: operatorStr,
        };
      }

      case Commands.DEVICE_INFO_REPORT: {
        // Payload: version(u16), board_name(str), serial(str)
        const version = view.getUint16(0, true);
        const nameLen = bytes[2];
        const name = new TextDecoder().decode(bytes.slice(3, 3 + nameLen));
        const serialLen = bytes[3 + nameLen];
        const serial = new TextDecoder().decode(
          bytes.slice(4 + nameLen, 4 + nameLen + serialLen)
        );
        return {
          version: `${(version >> 8) & 0xFF}.${version & 0xFF}`,
          boardName: name,
          serialNumber: serial,
        };
      }

      case Commands.ERROR_REPORT: {
        // Payload: error_code(u16), message(str)
        const errorCode = view.getUint16(0, true);
        const message = new TextDecoder().decode(bytes.slice(2));
        return {
          errorCode,
          message,
        };
      }

      default:
        // Unknown command — try JSON parse
        try {
          return JSON.parse(new TextDecoder().decode(bytes));
        } catch {
          return { raw: Array.from(bytes) };
        }
    }
  }
}