/**
 * NeuroLink Binary Wire Protocol
 * CRC-16/CCITT frame builder/parser for BLE and USB communication
 */

// ============================================================
// Command IDs (matches firmware usb_descriptors.h)
// ============================================================
export const COMMAND_IDS = {
  START_STREAM:      0x01,
  STOP_STREAM:       0x02,
  SET_SAMPLE_RATE:   0x03,
  SET_CHANNEL_GAIN:  0x04,
  SET_FILTER:        0x05,
  ENABLE_CHANNELS:   0x06,
  DISABLE_CHANNELS:  0x07,
  IMPEDANCE_CHECK:   0x08,
  GET_STATUS:        0x09,
  SET_FEATURE_CFG:   0x0A,
  RESET_DEVICE:      0xFF,
};

// ============================================================
// Response Types
// ============================================================
export const FRAME_TYPES = {
  SYNC_WORD:     0xAA55,
  TYPE_SAMPLES:  0x01,
  TYPE_STATUS:   0x02,
  TYPE_IMPEDANCE: 0x03,
  TYPE_FEATURES: 0x04,
  TYPE_ACK:      0x05,
  TYPE_NACK:     0x06,
};

// ============================================================
// CRC-16/CCITT (Polynomial 0x1021, Init 0xFFFF)
// ============================================================
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

// ============================================================
// Frame Builder
// ============================================================

/**
 * Build a command frame to send to the device
 * Format: [0xAA][0x55][CMD][LEN_H][LEN_L][PAYLOAD...][CRC_H][CRC_L]
 */
export function buildCommand(commandId, payload) {
  const headerLen = 5; // sync(2) + cmd(1) + len(2)
  const crcLen = 2;
  const payloadLen = payload ? payload.length : 0;
  const totalLen = headerLen + payloadLen + crcLen;

  const frame = Buffer.alloc(totalLen);
  let offset = 0;

  // Sync word
  frame[offset++] = 0xAA;
  frame[offset++] = 0x55;

  // Command ID
  frame[offset++] = commandId & 0xFF;

  // Payload length (big-endian)
  frame[offset++] = (payloadLen >> 8) & 0xFF;
  frame[offset++] = payloadLen & 0xFF;

  // Payload
  if (payload && payloadLen > 0) {
    for (let i = 0; i < payloadLen; i++) {
      frame[offset++] = payload[i] & 0xFF;
    }
  }

  // CRC-16 over everything except the CRC itself
  const crcVal = crc16(frame.slice(0, offset));
  frame[offset++] = (crcVal >> 8) & 0xFF;
  frame[offset++] = crcVal & 0xFF;

  return frame;
}

/**
 * Build a SET_SAMPLE_RATE command
 * @param {number} rate - Sample rate in Hz (250, 500, 1000, 2000, 4000, 8000, 16000)
 */
export function buildSetSampleRateCommand(rate) {
  const rateMap = {
    250: 0x06,
    500: 0x05,
    1000: 0x04,
    2000: 0x03,
    4000: 0x02,
    8000: 0x01,
    16000: 0x00,
  };
  const rateCode = rateMap[rate] ?? 0x04;
  return buildCommand(COMMAND_IDS.SET_SAMPLE_RATE, [rateCode]);
}

/**
 * Build a SET_CHANNEL_GAIN command
 * @param {number} channel - Channel number (0-31)
 * @param {number} gain - Gain value (1, 2, 4, 6, 8, 12, 24)
 */
export function buildSetGainCommand(channel, gain) {
  const gainMap = { 1: 0x00, 2: 0x10, 4: 0x20, 6: 0x30, 8: 0x40, 12: 0x50, 24: 0x60 };
  const gainCode = gainMap[gain] ?? 0x30;
  return buildCommand(COMMAND_IDS.SET_CHANNEL_GAIN, [channel & 0xFF, gainCode]);
}

/**
 * Build a SET_FILTER command
 * @param {number} filterType - 0=bandpass, 1=notch, 2=lowpass, 3=highpass
 * @param {number} order - Filter order (1-4)
 * @param {number} cutoffLow - Low cutoff in 0.01 Hz units
 * @param {number} cutoffHigh - High cutoff in 0.01 Hz units
 */
export function buildSetFilterCommand(filterType, order, cutoffLow, cutoffHigh) {
  const payload = Buffer.alloc(6);
  payload[0] = filterType & 0xFF;
  payload[1] = order & 0xFF;
  payload.writeUInt16BE(cutoffLow, 2);
  payload.writeUInt16BE(cutoffHigh, 4);
  return buildCommand(COMMAND_IDS.SET_FILTER, Array.from(payload));
}

/**
 * Build an ENABLE_CHANNELS command
 * @param {number[]} channels - Array of channel numbers to enable (0-31)
 */
export function buildEnableChannelsCommand(channels) {
  const bitmask = new Uint8Array(4); // 32 bits = 4 bytes
  for (const ch of channels) {
    if (ch >= 0 && ch < 32) {
      bitmask[Math.floor(ch / 8)] |= (1 << (ch % 8));
    }
  }
  return buildCommand(COMMAND_IDS.ENABLE_CHANNELS, Array.from(bitmask));
}

// ============================================================
// Frame Parser
// ============================================================

/**
 * Parse a received data frame from the device
 * Returns parsed object with type and data, or null if invalid
 */
export function parseFrame(data) {
  if (!data || data.length < 7) {
    return null; // Minimum frame: sync(2) + type(1) + len(2) + crc(2)
  }

  // Check sync word
  if (data[0] !== 0xAA || data[1] !== 0x55) {
    return null;
  }

  const frameType = data[2];
  const payloadLen = (data[3] << 8) | data[4];

  // Verify total length
  const expectedLen = 5 + payloadLen + 2; // header + payload + crc
  if (data.length < expectedLen) {
    return null;
  }

  // Verify CRC
  const crcOffset = 5 + payloadLen;
  const receivedCrc = (data[crcOffset] << 8) | data[crcOffset + 1];
  const calculatedCrc = crc16(data.slice(0, crcOffset));

  if (receivedCrc !== calculatedCrc) {
    return { type: 'crc_error', expected: calculatedCrc, received: receivedCrc };
  }

  // Parse payload based on frame type
  const payload = data.slice(5, 5 + payloadLen);

  switch (frameType) {
    case FRAME_TYPES.TYPE_SAMPLES:
      return parseSampleFrame(payload);

    case FRAME_TYPES.TYPE_STATUS:
      return parseStatusFrame(payload);

    case FRAME_TYPES.TYPE_IMPEDANCE:
      return parseImpedanceFrame(payload);

    case FRAME_TYPES.TYPE_FEATURES:
      return parseFeatureFrame(payload);

    case FRAME_TYPES.TYPE_ACK:
      return { type: 'ack', commandId: payload[0] };

    case FRAME_TYPES.TYPE_NACK:
      return { type: 'nack', commandId: payload[0], errorCode: payload[1] };

    default:
      return { type: 'unknown', frameType, payload };
  }
}

/**
 * Parse biosignal sample data
 * Payload: [frame_id(1)][timestamp_ms(2)][num_channels(1)][ch0_h(2)...ch31_h(2)]
 * Each channel is 16-bit signed (high word of 24-bit, truncated)
 */
function parseSampleFrame(payload) {
  if (payload.length < 4) return null;

  const frameId = payload[0];
  const timestampMs = (payload[1] << 8) | payload[2];
  const numChannels = payload[3];

  const channels = new Float32Array(32);
  let offset = 4;

  for (let i = 0; i < numChannels && i < 32; i++) {
    if (offset + 2 <= payload.length) {
      const raw = (payload[offset] << 8) | payload[offset + 1];
      // Sign-extend 16-bit to float (µV)
      const signed = raw > 32767 ? raw - 65536 : raw;
      // Convert from ADC counts to µV:
      // VREF = 2.5V, Gain = 6, 24-bit → 16-bit truncated ≈ /256
      // µV = (raw / (2^23 - 1)) * (VREF / Gain) * 1e6 * (1/256)
      channels[i] = signed * 0.0119; // Approximate scale factor for µV
      offset += 2;
    }
  }

  return {
    type: 'samples',
    frameId,
    timestamp: timestampMs,
    numChannels,
    channels,
  };
}

/**
 * Parse device status frame
 * Payload: [battery_pct(1)][charging(1)][temp_c(1)][sample_rate(1)][active_channels(4)]
 */
function parseStatusFrame(payload) {
  if (payload.length < 8) return null;

  return {
    type: 'status',
    battery: payload[0],
    charging: payload[1] !== 0,
    temperature: payload[2],
    sampleRate: [250, 500, 1000, 2000, 4000, 8000, 16000][payload[3]] || 250,
    activeChannels: payload[4] | (payload[5] << 8) | (payload[6] << 16) | (payload[7] << 24),
  };
}

/**
 * Parse impedance measurement frame
 * Payload: [num_channels(1)][ch0_imp_h(2)...ch31_imp_h(2)]
 * Each impedance value in kΩ (×10)
 */
function parseImpedanceFrame(payload) {
  if (payload.length < 1) return null;

  const numChannels = payload[0];
  const impedance = new Float32Array(32);
  let offset = 1;

  for (let i = 0; i < numChannels && i < 32; i++) {
    if (offset + 2 <= payload.length) {
      const raw = (payload[offset] << 8) | payload[offset + 1];
      impedance[i] = raw / 10.0; // Convert to kΩ
      offset += 2;
    }
  }

  return {
    type: 'impedance',
    numChannels,
    impedance,
  };
}

/**
 * Parse feature extraction frame
 * Payload: [frame_id(1)][timestamp_ms(2)][feature_mask(1)][num_channels(1)][data...]
 */
function parseFeatureFrame(payload) {
  if (payload.length < 5) return null;

  const frameId = payload[0];
  const timestamp = (payload[1] << 8) | payload[2];
  const featureMask = payload[3];
  const numChannels = payload[4];

  const features = {};
  let offset = 5;

  // RMS feature
  if (featureMask & 0x01) {
    features.rms = new Float32Array(numChannels);
    for (let i = 0; i < numChannels && offset + 2 <= payload.length; i++) {
      features.rms[i] = ((payload[offset] << 8) | payload[offset + 1]) / 100.0;
      offset += 2;
    }
  }

  // MAV feature
  if (featureMask & 0x02) {
    features.mav = new Float32Array(numChannels);
    for (let i = 0; i < numChannels && offset + 2 <= payload.length; i++) {
      features.mav[i] = ((payload[offset] << 8) | payload[offset + 1]) / 100.0;
      offset += 2;
    }
  }

  return {
    type: 'features',
    frameId,
    timestamp,
    featureMask,
    numChannels,
    features,
  };
}

// ============================================================
// Utility Functions
// ============================================================

/**
 * Convert sample rate enum to Hz
 */
export function sampleRateToHz(rateCode) {
  const rates = { 0x00: 16000, 0x01: 8000, 0x02: 4000, 0x03: 2000, 0x04: 1000, 0x05: 500, 0x06: 250 };
  return rates[rateCode] || 250;
}

/**
 * Convert gain code to multiplier
 */
export function gainCodeToMultiplier(gainCode) {
  const gains = { 0x00: 1, 0x10: 2, 0x20: 4, 0x30: 6, 0x40: 8, 0x50: 12, 0x60: 24 };
  return gains[gainCode] || 6;
}