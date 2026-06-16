/**
 * Terramesh — Protocol Utilities
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Wire protocol parser for Terramesh mesh packets and sensor data records.
 * Handles binary packet parsing, CRC verification, and JSON serialization
 * for the cloud API.
 */

const MESH_HEADER_SIZE = 12; // 8 preamble + 2 dest + 2 src + 2 seq + 1 hop + 1 type
const CRC_SIZE = 4;
const SENSOR_RECORD_SIZE = 32;

const MSG_TYPE = {
  DATA: 0x01,
  ACK: 0x02,
  CMD: 0x03,
  BEACON: 0x04,
  REGISTER: 0x05,
  RREQ: 0x06,
  RREP: 0x07,
};

const CLASSIFICATION = {
  NORMAL: 0,
  WARNING: 1,
  CRITICAL: 2,
};

/**
 * CRC32 calculation (same polynomial as hardware CRC peripheral)
 */
const crc32Table = (() => {
  const table = new Uint32Array(256);
  for (let i = 0; i < 256; i++) {
    let crc = i;
    for (let j = 0; j < 8; j++) {
      crc = (crc & 1) ? (0xEDB88320 ^ (crc >>> 1)) : (crc >>> 1);
    }
    table[i] = crc;
  }
  return table;
})();

const crc32 = (data) => {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < data.length; i++) {
    crc = crc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >>> 8);
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
};

/**
 * Parse a raw mesh packet from the LoRa radio
 */
const parseMeshPacket = (buffer) => {
  if (!buffer || buffer.length < MESH_HEADER_SIZE + CRC_SIZE) {
    return null;
  }

  const view = new DataView(buffer);

  const packet = {
    preamble: new Uint8Array(buffer.slice(0, 8)),
    dest_addr: view.getUint16(8, false),
    src_addr: view.getUint16(10, false),
    seq_num: view.getUint16(12, false),
    hop_count: view.getUint8(14),
    msg_type: view.getUint8(15),
    payload: null,
    rssi: 0,
    snr: 0,
  };

  // Extract payload
  const payloadLen = buffer.length - MESH_HEADER_SIZE - CRC_SIZE;
  if (payloadLen > 0) {
    packet.payload = new Uint8Array(buffer.slice(MESH_HEADER_SIZE, MESH_HEADER_SIZE + payloadLen));
  }

  // Verify CRC
  const crcOffset = buffer.length - CRC_SIZE;
  const expectedCrc = view.getUint32(crcOffset, false);
  const calculatedCrc = crc32(new Uint8Array(buffer.slice(0, crcOffset)));

  if (calculatedCrc !== expectedCrc) {
    console.warn(`[Protocol] CRC mismatch: calc=0x${calculatedCrc.toString(16)}, expected=0x${expectedCrc.toString(16)}`);
    return null;
  }

  return packet;
};

/**
 * Parse a 32-byte sensor data record from payload
 */
const parseSensorData = (payload) => {
  if (!payload || payload.length < SENSOR_RECORD_SIZE) {
    return null;
  }

  const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);

  return {
    timestamp: view.getUint32(0, true),
    pore_press_shallow: view.getUint16(4, true),
    pore_press_deep: view.getUint16(6, true),
    moisture: view.getUint16(8, true),
    tilt_x: view.getInt16(10, true),
    tilt_y: view.getInt16(12, true),
    accel_x: view.getInt16(14, true),
    accel_y: view.getInt16(16, true),
    accel_z: view.getInt16(18, true),
    temperature: view.getInt16(20, true),
    pressure: view.getUint16(22, true),
    classification: view.getUint8(24),
    battery_mv: view.getUint8(25),
  };
};

/**
 * Build a mesh packet for transmission
 */
const buildMeshPacket = (destAddr, srcAddr, msgType, payload) => {
  const payloadLen = payload ? payload.length : 0;
  const totalLen = MESH_HEADER_SIZE + payloadLen + CRC_SIZE;
  const buffer = new ArrayBuffer(totalLen);
  const view = new DataView(buffer);

  // Preamble (sync word)
  const preamble = new Uint8Array([0x2B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
  for (let i = 0; i < 8; i++) view.setUint8(i, preamble[i]);

  view.setUint16(8, destAddr, false);
  view.setUint16(10, srcAddr, false);
  view.setUint16(12, 0, false); // seq_num (set by caller)
  view.setUint8(14, 8); // hop_count
  view.setUint8(15, msgType);

  if (payload) {
    for (let i = 0; i < payloadLen; i++) {
      view.setUint8(MESH_HEADER_SIZE + i, payload[i]);
    }
  }

  // Append CRC32
  const crc = crc32(new Uint8Array(buffer.slice(0, MESH_HEADER_SIZE + payloadLen)));
  view.setUint32(MESH_HEADER_SIZE + payloadLen, crc, false);

  return new Uint8Array(buffer);
};

/**
 * Build a sensor data record (32 bytes) from values
 */
const buildSensorRecord = (values) => {
  const buffer = new ArrayBuffer(SENSOR_RECORD_SIZE);
  const view = new DataView(buffer);

  view.setUint32(0, Math.floor(Date.now() / 1000), true);
  view.setUint16(4, Math.round(values.porePressShallow * 100), true);
  view.setUint16(6, Math.round(values.porePressDeep * 100), true);
  view.setUint16(8, Math.round(values.moisture * 100), true);
  view.setInt16(10, Math.round(values.tiltX * 1000), true);
  view.setInt16(12, Math.round(values.tiltY * 1000), true);
  view.setInt16(14, Math.round(values.accelX), true);
  view.setInt16(16, Math.round(values.accelY), true);
  view.setInt16(18, Math.round(values.accelZ), true);
  view.setInt16(20, Math.round(values.temperature * 100), true);
  view.setUint16(22, Math.round(values.pressure), true);
  view.setUint8(24, values.classification || 0);
  view.setUint8(25, Math.round(values.batteryMv / 20));

  return new Uint8Array(buffer);
};

/**
 * Format sensor data for cloud API JSON
 */
const formatTelemetryJson = (nodeId, sensorData) => {
  return {
    node_id: nodeId,
    timestamp: sensorData.timestamp,
    sensors: {
      pore_pressure_shallow: sensorData.pore_press_shallow * 0.01,
      pore_pressure_deep: sensorData.pore_press_deep * 0.01,
      moisture: sensorData.moisture * 0.01,
      tilt_x: sensorData.tilt_x * 0.001,
      tilt_y: sensorData.tilt_y * 0.001,
      accel_x: sensorData.accel_x,
      accel_y: sensorData.accel_y,
      accel_z: sensorData.accel_z,
      temperature: sensorData.temperature * 0.01,
      pressure: sensorData.pressure,
    },
    classification: sensorData.classification,
    battery_mv: sensorData.battery_mv * 20,
  };
};

/**
 * Parse classification code to human-readable string
 */
const classificationToString = (code) => {
  switch (code) {
    case CLASSIFICATION.NORMAL:
      return 'Normal';
    case CLASSIFICATION.WARNING:
      return 'Warning';
    case CLASSIFICATION.CRITICAL:
      return 'Critical';
    default:
      return 'Unknown';
  }
};

/**
 * Get color for classification
 */
const classificationColor = (code) => {
  switch (code) {
    case CLASSIFICATION.NORMAL:
      return '#00d4aa';
    case CLASSIFICATION.WARNING:
      return '#ffa500';
    case CLASSIFICATION.CRITICAL:
      return '#ff4444';
    default:
      return '#6c6c80';
  }
};

export const protocol = {
  MSG_TYPE,
  CLASSIFICATION,
  MESH_HEADER_SIZE,
  SENSOR_RECORD_SIZE,
  parseMeshPacket,
  parseSensorData,
  buildMeshPacket,
  buildSensorRecord,
  formatTelemetryJson,
  classificationToString,
  classificationColor,
  crc32,
};
