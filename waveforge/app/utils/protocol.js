/**
 * protocol.js — WaveForge Binary Wire Protocol
 * Frame format: [0xAA] [LEN] [CMD] [DATA...] [CRC8]
 * Used for BLE/USB communication between companion app and synth board
 */

// CRC-8 lookup table (polynomial 0x07, init 0x00)
const CRC8_TABLE = (() => {
  const table = new Uint8Array(256);
  for (let i = 0; i < 256; i++) {
    let crc = i;
    for (let j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x07;
      } else {
        crc <<= 1;
      }
    }
    table[i] = crc & 0xFF;
  }
  return table;
})();

/**
 * Calculate CRC-8 checksum
 * @param {Buffer|Uint8Array} data - data bytes
 * @returns {number} CRC-8 value
 */
function crc8(data) {
  let crc = 0x00;
  for (let i = 0; i < data.length; i++) {
    crc = CRC8_TABLE[crc ^ data[i]];
  }
  return crc & 0xFF;
}

/**
 * WaveForge Protocol — command IDs and frame builder/parser
 */
export class WaveForgeProtocol {
  // ---- Frame constants ----
  static FRAME_START = 0xAA;
  static FRAME_HEADER_SIZE = 3; // START + LEN + CMD
  static FRAME_CRC_SIZE = 1;
  static MAX_PAYLOAD = 252; // 256 - header - CRC

  // ---- Command IDs (1 byte) ----
  // System commands (0x01–0x0F)
  static CMD_PING           = 0x01;
  static CMD_PONG           = 0x02;
  static CMD_RESET          = 0x03;
  static CMD_VERSION        = 0x04;
  static CMD_STATUS         = 0x05;
  static CMD_FACTORY_RESET  = 0x06;

  // Synthesis commands (0x10–0x2F)
  static CMD_OSC_MODE       = 0x10;
  static CMD_WAVETABLE     = 0x11;
  static CMD_FM_DEPTH      = 0x12;
  static CMD_FM_RATIO      = 0x13;
  static CMD_FILTER_CUTOFF = 0x14;
  static CMD_FILTER_RES    = 0x15;
  static CMD_FILTER_TYPE   = 0x16;
  static CMD_ENV_ATTACK    = 0x17;
  static CMD_ENV_DECAY     = 0x18;
  static CMD_ENV_SUSTAIN   = 0x19;
  static CMD_ENV_RELEASE   = 0x1A;
  static CMD_MASTER_VOL    = 0x1B;
  static CMD_PAN           = 0x1C;

  // Effect commands (0x30–0x3F)
  static CMD_FX_REVERB         = 0x30;
  static CMD_FX_DELAY_TIME     = 0x31;
  static CMD_FX_DELAY_FB       = 0x32;
  static CMD_FX_CHORUS_DEPTH   = 0x33;
  static CMD_FX_CHORUS_RATE    = 0x34;

  // MIDI commands (0x40–0x4F)
  static CMD_MIDI_NOTE_ON     = 0x40;
  static CMD_MIDI_NOTE_OFF    = 0x41;
  static CMD_MIDI_CC          = 0x42;
  static CMD_MIDI_PB          = 0x43;
  static CMD_MIDI_CHANNEL     = 0x44;
  static CMD_MIDI_THRU        = 0x45;

  // Patch commands (0x50–0x5F)
  static CMD_SAVE_PATCH       = 0x50;
  static CMD_LOAD_PATCH       = 0x51;
  static CMD_DELETE_PATCH     = 0x52;
  static CMD_PATCH_LIST       = 0x53;
  static CMD_PATCH_NAME       = 0x54;

  // CV commands (0x60–0x6F)
  static CMD_CV1_MODE         = 0x60;
  static CMD_CV2_MODE         = 0x61;
  static CMD_CV3_MODE         = 0x62;
  static CMD_CV4_MODE         = 0x63;
  static CMD_CV_VALUES        = 0x64;

  // Audio config (0x70–0x7F)
  static CMD_SAMPLE_RATE      = 0x70;
  static CMD_BIT_DEPTH        = 0x71;
  static CMD_BUFFER_SIZE      = 0x72;

  // ---- Response codes ----
  static RSP_OK       = 0x80;
  static RSP_ERROR   = 0x81;
  static RSP_DATA    = 0x82;

  constructor() {
    this.rxBuffer = new Uint8Array(256);
    this.rxIndex = 0;
    this.rxExpected = 0;
    this.rxState = 'IDLE'; // IDLE, GOT_START, GOT_LEN, GOT_CMD, IN_DATA
  }

  /**
   * Build a parameter frame with float value (0.0–1.0 mapped to 0x0000–0xFFFF)
   * @param {number} command - Command ID
   * @param {number} value - Float value 0.0–1.0
   * @returns {Uint8Array} Complete frame with CRC
   */
  buildFrame(command, value) {
    // Convert float 0.0–1.0 to 16-bit integer
    let intVal;
    if (typeof value === 'number' && value >= 0 && value <= 1) {
      intVal = Math.round(value * 65535);
    } else if (typeof value === 'number') {
      intVal = Math.round(value);
    } else {
      intVal = 0;
    }
    intVal = Math.max(0, Math.min(65535, intVal));

    const payload = new Uint8Array(2);
    payload[0] = (intVal >> 8) & 0xFF; // MSB
    payload[1] = intVal & 0xFF;        // LSB

    return this._buildRawFrame(command, payload);
  }

  /**
   * Build a MIDI Note On frame
   * @param {number} channel - MIDI channel (0–15)
   * @param {number} note - MIDI note (0–127)
   * @param {number} velocity - MIDI velocity (0–127)
   * @returns {Uint8Array}
   */
  buildMidiNoteOn(channel, note, velocity) {
    const payload = new Uint8Array(3);
    payload[0] = channel & 0x0F;
    payload[1] = note & 0x7F;
    payload[2] = velocity & 0x7F;
    return this._buildRawFrame(WaveForgeProtocol.CMD_MIDI_NOTE_ON, payload);
  }

  /**
   * Build a MIDI Note Off frame
   * @param {number} channel - MIDI channel
   * @param {number} note - MIDI note
   * @returns {Uint8Array}
   */
  buildMidiNoteOff(channel, note) {
    const payload = new Uint8Array(2);
    payload[0] = channel & 0x0F;
    payload[1] = note & 0x7F;
    return this._buildRawFrame(WaveForgeProtocol.CMD_MIDI_NOTE_OFF, payload);
  }

  /**
   * Build a raw frame with arbitrary payload
   * @param {number} command - Command ID
   * @param {Uint8Array} payload - Data bytes
   * @returns {Uint8Array} Complete frame [START, LEN, CMD, DATA..., CRC]
   */
  _buildRawFrame(command, payload) {
    const totalLen = 3 + payload.length + 1; // START + LEN + CMD + payload + CRC
    const frame = new Uint8Array(totalLen);

    frame[0] = WaveForgeProtocol.FRAME_START;
    frame[1] = payload.length + 1; // LEN = payload + CMD byte
    frame[2] = command;

    for (let i = 0; i < payload.length; i++) {
      frame[3 + i] = payload[i];
    }

    // Calculate CRC over [LEN, CMD, DATA...]
    const crcData = frame.slice(1, totalLen - 1);
    frame[totalLen - 1] = crc8(crcData);

    return frame;
  }

  /**
   * Parse incoming bytes (stream parser with state machine)
   * @param {Uint8Array} data - Received bytes
   * @returns {Array} Array of parsed frames: [{cmd, payload}]
   */
  parse(data) {
    const results = [];

    for (let i = 0; i < data.length; i++) {
      const byte = data[i];

      switch (this.rxState) {
        case 'IDLE':
          if (byte === WaveForgeProtocol.FRAME_START) {
            this.rxBuffer[0] = byte;
            this.rxIndex = 1;
            this.rxState = 'GOT_START';
          }
          break;

        case 'GOT_START':
          this.rxBuffer[1] = byte;
          this.rxExpected = byte + 3; // LEN + CMD + payload + START + CRC
          this.rxIndex = 2;
          this.rxState = 'IN_DATA';
          break;

        case 'IN_DATA':
          this.rxBuffer[this.rxIndex] = byte;
          this.rxIndex++;

          if (this.rxIndex >= this.rxExpected) {
            // Verify CRC
            const frameLen = this.rxExpected;
            const crcData = this.rxBuffer.slice(1, frameLen - 1);
            const expectedCrc = this.rxBuffer[frameLen - 1];
            const calculatedCrc = crc8(crcData);

            if (calculatedCrc === expectedCrc) {
              // Valid frame
              const cmd = this.rxBuffer[2];
              const payloadLen = this.rxBuffer[1] - 1;
              const payload = new Uint8Array(payloadLen);
              for (let j = 0; j < payloadLen; j++) {
                payload[j] = this.rxBuffer[3 + j];
              }
              results.push({ cmd, payload });
            }
            // Reset state regardless of CRC validity
            this.rxState = 'IDLE';
            this.rxIndex = 0;
          }
          break;
      }
    }

    return results;
  }

  /**
   * Extract a float parameter value from a frame payload
   * @param {Uint8Array} payload - 2-byte payload (MSB, LSB)
   * @returns {number} Float value 0.0–1.0
   */
  static payloadToFloat(payload) {
    if (payload.length < 2) return 0;
    const intVal = (payload[0] << 8) | payload[1];
    return intVal / 65535;
  }

  /**
   * Extract an integer parameter value from a frame payload
   * @param {Uint8Array} payload - 2-byte payload (MSB, LSB)
   * @returns {number} Integer value
   */
  static payloadToInt(payload) {
    if (payload.length < 2) return 0;
    return (payload[0] << 8) | payload[1];
  }

  /**
   * Get command name from ID (for debug/logging)
   * @param {number} cmd - Command ID
   * @returns {string} Human-readable command name
   */
  static getCommandName(cmd) {
    const names = {
      0x01: 'PING', 0x02: 'PONG', 0x03: 'RESET', 0x04: 'VERSION',
      0x05: 'STATUS', 0x06: 'FACTORY_RESET',
      0x10: 'OSC_MODE', 0x11: 'WAVETABLE', 0x12: 'FM_DEPTH', 0x13: 'FM_RATIO',
      0x14: 'FILTER_CUTOFF', 0x15: 'FILTER_RES', 0x16: 'FILTER_TYPE',
      0x17: 'ENV_ATTACK', 0x18: 'ENV_DECAY', 0x19: 'ENV_SUSTAIN', 0x1A: 'ENV_RELEASE',
      0x1B: 'MASTER_VOL', 0x1C: 'PAN',
      0x30: 'FX_REVERB', 0x31: 'FX_DELAY_TIME', 0x32: 'FX_DELAY_FB',
      0x33: 'FX_CHORUS_DEPTH', 0x34: 'FX_CHORUS_RATE',
      0x40: 'MIDI_NOTE_ON', 0x41: 'MIDI_NOTE_OFF', 0x42: 'MIDI_CC',
      0x43: 'MIDI_PB', 0x44: 'MIDI_CHANNEL', 0x45: 'MIDI_THRU',
      0x50: 'SAVE_PATCH', 0x51: 'LOAD_PATCH', 0x52: 'DELETE_PATCH',
      0x53: 'PATCH_LIST', 0x54: 'PATCH_NAME',
      0x60: 'CV1_MODE', 0x61: 'CV2_MODE', 0x62: 'CV3_MODE',
      0x63: 'CV4_MODE', 0x64: 'CV_VALUES',
      0x70: 'SAMPLE_RATE', 0x71: 'BIT_DEPTH', 0x72: 'BUFFER_SIZE',
      0x80: 'RSP_OK', 0x81: 'RSP_ERROR', 0x82: 'RSP_DATA',
    };
    return names[cmd] || `CMD_0x${cmd.toString(16).padStart(2, '0')}`;
  }
}