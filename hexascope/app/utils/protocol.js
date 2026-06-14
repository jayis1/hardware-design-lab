/**
 * protocol.js — HexaScope Binary Wire Protocol
 * Implements a CRC-16-CCITT based binary protocol for USB and Wi-Fi transport
 *
 * Frame format:
 *   [0xAA] [0x55] [CMD:1] [LEN:2LE] [SEQ:2LE] [PAYLOAD:LEN] [CRC16:2LE]
 *
 *   Preamble: 0xAA 0x55 (fixed)
 *   CMD:      Command ID (1 byte)
 *   LEN:      Payload length in bytes (2 bytes, little-endian, max 65535)
 *   SEQ:      Sequence number (2 bytes, little-endian, wraps at 65535)
 *   PAYLOAD:  Variable length data
 *   CRC16:    CRC-16-CCITT of CMD + LEN + SEQ + PAYLOAD (2 bytes, little-endian)
 */

const PREAMBLE_BYTE_0 = 0xAA;
const PREAMBLE_BYTE_1 = 0x55;

// Command IDs
export const COMMANDS = {
  // Acquisition control
  ARM_TRIGGER:          0x01,
  STOP_ACQUISITION:    0x02,
  READ_STATUS:          0x03,
  SET_TRIGGER_THRESH:   0x04,
  SET_DECIMATION:       0x05,
  SET_CHANNEL_ENABLE:   0x06,
  SET_PROTOCOL:         0x07,

  // Register access
  READ_REGISTER:        0x10,
  WRITE_REGISTER:       0x11,

  // Calibration
  SET_CALIBRATION:     0x20,

  // Device info
  GET_INFO:            0x30,

  // Waveform data (hostbound only)
  WAVEFORM_DATA:       0x40,

  // Protocol decode data (hostbound only)
  PROTOCOL_DATA:       0x41,

  // Acknowledgment
  ACK:                 0x80,
  NACK:                0x81,
};

// Trigger types (matching firmware defines)
export const TRIGGER_TYPES = {
  RISING:   0x01,
  FALLING:  0x02,
  BOTH:     0x03,
  PULSE:    0x04,
  TIMEOUT:  0x05,
  RUNT:     0x06,
  WINDOW:   0x07,
  LOGIC:    0x08,
  SERIAL:   0x09,
};

// Protocol decode types
export const PROTOCOL_TYPES = {
  UART: 0x01,
  SPI:  0x02,
  I2C:  0x03,
  CAN:  0x04,
  LIN:  0x05,
};

// Channel mask bits
export const CHANNEL_BITS = {
  CH1: 0x01,
  CH2: 0x02,
  CH3: 0x04,
  CH4: 0x08,
  D1:  0x10,
  D2:  0x20,
};

/**
 * CRC-16-CCITT calculation
 * Polynomial: 0x1021, Initial value: 0xFFFF
 */
function crc16_ccitt(data, initial = 0xFFFF) {
  let crc = initial;
  for (let i = 0; i < data.length; i++) {
    crc ^= (data[i] << 8);
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
 * Build a command frame
 * @param {number} cmd - Command ID
 * @param {Uint8Array} payload - Payload data
 * @param {number} seq - Sequence number
 * @returns {Uint8Array} Complete frame
 */
export function buildFrame(cmd, payload, seq = 0) {
  const len = payload ? payload.length : 0;
  const header = new Uint8Array(6);  // PREAMBLE(2) + CMD(1) + LEN(2) + SEQ(2)
  header[0] = PREAMBLE_BYTE_0;
  header[1] = PREAMBLE_BYTE_1;
  header[2] = cmd;
  header[3] = len & 0xFF;           // LEN low byte
  header[4] = (len >> 8) & 0xFF;    // LEN high byte
  header[5] = seq & 0xFF;            // SEQ low byte (SEQ high byte in next)

  // Reconstruct header with proper SEQ encoding
  const headerWithSeq = new Uint8Array(7);
  headerWithSeq[0] = PREAMBLE_BYTE_0;
  headerWithSeq[1] = PREAMBLE_BYTE_1;
  headerWithSeq[2] = cmd;
  headerWithSeq[3] = len & 0xFF;
  headerWithSeq[4] = (len >> 8) & 0xFF;
  headerWithSeq[5] = seq & 0xFF;
  headerWithSeq[6] = (seq >> 8) & 0xFF;

  // Calculate CRC over CMD + LEN + SEQ + PAYLOAD
  const crcData = new Uint8Array(5 + len);
  crcData[0] = cmd;
  crcData[1] = len & 0xFF;
  crcData[2] = (len >> 8) & 0xFF;
  crcData[3] = seq & 0xFF;
  crcData[4] = (seq >> 8) & 0xFF;
  if (payload) {
    crcData.set(payload, 5);
  }
  const crc = crc16_ccitt(crcData);

  // Assemble complete frame
  const frame = new Uint8Array(7 + len + 2);  // header(7) + payload(len) + crc(2)
  frame[0] = PREAMBLE_BYTE_0;
  frame[1] = PREAMBLE_BYTE_1;
  frame[2] = cmd;
  frame[3] = len & 0xFF;
  frame[4] = (len >> 8) & 0xFF;
  frame[5] = seq & 0xFF;
  frame[6] = (seq >> 8) & 0xFF;
  if (payload) {
    frame.set(payload, 7);
  }
  frame[7 + len] = crc & 0xFF;
  frame[7 + len + 1] = (crc >> 8) & 0xFF;

  return frame;
}

/**
 * Parse received data into frames
 * Uses a state machine to find preamble, validate CRC, and extract payloads
 */
export class FrameParser {
  constructor() {
    this.buffer = new Uint8Array(0);
    this.onFrame = null;  // Callback: (cmd, seq, payload) => void
  }

  /**
   * Feed received bytes into the parser
   * @param {Uint8Array} data - Raw received bytes
   */
  feed(data) {
    // Append to buffer
    const newBuf = new Uint8Array(this.buffer.length + data.length);
    newBuf.set(this.buffer, 0);
    newBuf.set(data, this.buffer.length);
    this.buffer = newBuf;

    // Try to parse frames
    while (this.buffer.length >= 9) {  // Minimum frame: preamble(2) + cmd(1) + len(2) + seq(2) + crc(2)
      // Find preamble
      let preambleIdx = -1;
      for (let i = 0; i < this.buffer.length - 1; i++) {
        if (this.buffer[i] === PREAMBLE_BYTE_0 && this.buffer[i + 1] === PREAMBLE_BYTE_1) {
          preambleIdx = i;
          break;
        }
      }

      if (preambleIdx === -1) {
        // No preamble found, discard all data
        this.buffer = new Uint8Array(0);
        break;
      }

      // Discard bytes before preamble
      if (preambleIdx > 0) {
        this.buffer = this.buffer.slice(preambleIdx);
      }

      // Check if we have enough data for the header
      if (this.buffer.length < 7) break;

      const cmd = this.buffer[2];
      const len = this.buffer[3] | (this.buffer[4] << 8);
      const seq = this.buffer[5] | (this.buffer[6] << 8);
      const totalFrameLen = 7 + len + 2;  // header + payload + crc

      // Check if we have the complete frame
      if (this.buffer.length < totalFrameLen) break;

      // Extract payload
      const payload = this.buffer.slice(7, 7 + len);

      // Calculate CRC
      const crcData = new Uint8Array(5 + len);
      crcData[0] = cmd;
      crcData[1] = len & 0xFF;
      crcData[2] = (len >> 8) & 0xFF;
      crcData[3] = seq & 0xFF;
      crcData[4] = (seq >> 8) & 0xFF;
      crcData.set(payload, 5);
      const expectedCrc = crc16_ccitt(crcData);

      const receivedCrc = this.buffer[7 + len] | (this.buffer[7 + len + 1] << 8);

      if (expectedCrc === receivedCrc) {
        // Valid frame
        if (this.onFrame) {
          this.onFrame(cmd, seq, payload);
        }
      }
      // else: CRC mismatch, discard frame

      // Remove processed frame from buffer
      this.buffer = this.buffer.slice(totalFrameLen);
    }
  }
}

/**
 * HexaScope Protocol Handler
 * Manages connection, sends commands, and receives waveform data
 */
export class HexaScopeProtocol {
  constructor() {
    this.seq = 0;
    this.parser = new FrameParser();
    this.connected = false;
    this.onData = null;         // Callback: (frame) => void
    this.onProtocolData = null; // Callback: (decodedFrame) => void
    this.onConnect = null;      // Callback: () => void
    this.onDisconnect = null;   // Callback: () => void

    this.parser.onFrame = (cmd, seq, payload) => {
      this._handleFrame(cmd, seq, payload);
    };
  }

  /**
   * Start the protocol handler (connect to device)
   */
  start() {
    // In a real implementation, this would use react-native-usb
    // or react-native-ble-plx for BLE/Wi-Fi connection
    this.connected = true;
    if (this.onConnect) this.onConnect();
  }

  /**
   * Stop the protocol handler (disconnect)
   */
  stop() {
    this.connected = false;
    if (this.onDisconnect) this.onDisconnect();
  }

  /**
   * Send a command to the device
   * @param {number} cmd - Command ID
   * @param {number} value - 16-bit value parameter
   * @param {number} index - 32-bit index/address parameter
   */
  sendCommand(cmd, value = 0, index = 0) {
    const payload = new Uint8Array(6);
    payload[0] = value & 0xFF;
    payload[1] = (value >> 8) & 0xFF;
    payload[2] = index & 0xFF;
    payload[3] = (index >> 8) & 0xFF;
    payload[4] = (index >> 16) & 0xFF;
    payload[5] = (index >> 24) & 0xFF;

    const frame = buildFrame(cmd, payload, this.seq++);
    // In real implementation, send via USB or Wi-Fi
    // this.transport.write(frame);
    return frame;
  }

  /**
   * Handle a received frame
   */
  _handleFrame(cmd, seq, payload) {
    switch (cmd) {
      case COMMANDS.WAVEFORM_DATA:
        this._handleWaveformData(payload);
        break;
      case COMMANDS.PROTOCOL_DATA:
        this._handleProtocolData(payload);
        break;
      case COMMANDS.ACK:
        // Command acknowledged
        break;
      case COMMANDS.NACK:
        // Command rejected
        break;
      case COMMANDS.READ_STATUS:
        this._handleStatus(payload);
        break;
      default:
        break;
    }
  }

  /**
   * Parse waveform data frame
   * Format matches usb_waveform_header_t in firmware
   */
  _handleWaveformData(payload) {
    if (payload.length < 16) return;  // Minimum header size

    const magic = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
    if (magic !== 0x48584600) return;  // "HXf\0"

    const channelMask = payload[4];
    const triggerType = payload[5];
    const sampleCount = payload[6] | (payload[7] << 8) | (payload[8] << 16) | (payload[9] << 24);
    const timestampNs = payload[10] | (payload[11] << 8) | (payload[12] << 16) | (payload[13] << 24);
    const sampleRateHz = payload[14] | (payload[15] << 8);
    const flags = payload[16] | (payload[17] << 8);

    // Extract channel data
    const channels = [];
    const dataOffset = 18;
    let offset = dataOffset;

    const numAnalogCh = 4;
    const numDigitalCh = 2;
    const analogBytesPerSample = 1;  // 8-bit ADC

    // Analog channels (interleaved: all 4 analog channels per sample point)
    for (let ch = 0; ch < numAnalogCh; ch++) {
      if (!(channelMask & (1 << ch))) {
        channels.push(null);
        continue;
      }
      const chData = new Uint8Array(sampleCount);
      for (let s = 0; s < sampleCount; s++) {
        if (offset + numAnalogCh * analogBytesPerSample <= payload.length) {
          chData[s] = payload[offset + s * numAnalogCh + ch];
        }
      }
      channels.push(chData);
      offset += sampleCount * numAnalogCh;
    }

    // Digital channels (bit-packed: 2 channels per byte)
    for (let ch = 0; ch < numDigitalCh; ch++) {
      if (!(channelMask & (1 << (ch + numAnalogCh)))) {
        channels.push(null);
        continue;
      }
      const chData = new Uint8Array(sampleCount);
      for (let s = 0; s < sampleCount; s++) {
        const byteIdx = offset + Math.floor(s / 4);
        if (byteIdx < payload.length) {
          const bitOffset = (ch * 2) + ((s % 4) * 2);
          chData[s] = (payload[byteIdx] >> bitOffset) & 0x01 ? 255 : 0;
        }
      }
      channels.push(chData);
    }

    if (this.onData) {
      this.onData({
        magic,
        channelMask,
        triggerType,
        sampleCount,
        timestamp: timestampNs / 1e9,
        sampleRate: sampleRateHz * 1000,
        flags,
        channels,
      });
    }
  }

  /**
   * Parse protocol decode data frame
   */
  _handleProtocolData(payload) {
    if (payload.length < 8) return;

    const protocol = payload[0];
    const direction = payload[1];  // 0=RX, 1=TX
    const timestamp = (payload[2] | (payload[3] << 8) | (payload[4] << 16) | (payload[5] << 24)) / 1e9;
    const error = payload[6];
    const dataLen = payload[7];
    const data = Array.from(payload.slice(8, 8 + dataLen));

    const protocolNames = ['Unknown', 'UART', 'SPI', 'I²C', 'CAN', 'LIN'];

    let info = '';
    switch (protocol) {
    case PROTOCOL_TYPES.UART:
      info = `${dataLen} bytes @ ${payload[8] | (payload[9] << 8)} baud`;
      break;
    case PROTOCOL_TYPES.SPI:
      info = `CS${(payload[8] & 0x03)}, MOSI/MISO`;
      break;
    case PROTOCOL_TYPES.I2C:
      const addr = payload[8] >> 1;
      const rw = payload[8] & 1;
      info = `Addr: 0x${addr.toString(16).toUpperCase()}, ${rw ? 'Read' : 'Write'}`;
      break;
    default:
      break;
    }

    if (this.onProtocolData) {
      this.onProtocolData({
        protocol,
        protocolName: protocolNames[protocol] || 'Unknown',
        direction: direction === 1 ? 'TX' : 'RX',
        timestamp,
        error: !!error,
        data: data.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' '),
        info,
        bytesPerSec: 0,
        type: protocolNames[protocol] || 'Unknown',
      });
    }
  }

  /**
   * Handle status response
   */
  _handleStatus(payload) {
    if (payload.length < 4) return;
    const status = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
    return {
      armed: !!(status & 0x01),
      triggered: !!(status & 0x02),
      done: !!(status & 0x04),
      overflow: !!(status & 0x08),
    };
  }
}

export default HexaScopeProtocol;