/**
 * @file    protocol.js
 * @brief   TideBand BLE protocol definitions and packet parsing.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

// ---- Service / Characteristic UUIDs ----
export const TIDEBAND_SERVICE_UUID = '0000a5b0-0000-1000-8000-00805f9b34fb';
export const TIDEBAND_TX_CHAR_UUID  = '0000a5b1-0000-1000-8000-00805f9b34fb';
export const TIDEBAND_RX_CHAR_UUID  = '0000a5b2-0000-1000-8000-00805f9b34fb';

// ---- Protocol constants ----
export const SYNC_BYTE = 0xA5;
export const MAX_PAYLOAD = 247;

// ---- Opcodes ----
export const OP = {
  PROFILE_DATA:   0x01,
  DIVE_START:     0x02,
  DIVE_END:       0x03,
  STATUS_REQ:     0x04,
  STATUS_RSP:     0x05,
  CAL_SET:        0x06,
  CAL_RSP:        0x07,
  OTA_BEGIN:      0x10,
  OTA_CHUNK:      0x11,
  OTA_END:        0x12,
  OTA_ACK:        0x13,
  ERASE_DIVES:    0x20,
  EXPORT_BEGIN:   0x21,
  EXPORT_CHUNK:   0x22,
  EXPORT_END:     0x23,
  SET_RATE:       0x30,
  SET_THRESHOLD:  0x31,
  GET_INFO:       0x32,
  INFO_RSP:       0x33,
};

// ---- CRC8 (XOR of all bytes) ----
export function crc8(data) {
  let crc = 0;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
  }
  return crc & 0xFF;
}

// ---- Build a BLE packet ----
export function buildPacket(opcode, payload = []) {
  const pkt = new Uint8Array(3 + payload.length + 1);
  pkt[0] = SYNC_BYTE;
  pkt[1] = opcode;
  pkt[2] = payload.length;
  for (let i = 0; i < payload.length; i++) {
    pkt[3 + i] = payload[i];
  }
  pkt[3 + payload.length] = crc8(pkt.slice(0, 3 + payload.length));
  return Buffer.from(pkt).toString('base64');
}

// ---- Parse a received BLE packet ----
export function parsePacket(base64Data) {
  const raw = Buffer.from(base64Data, 'base64');
  if (raw.length < 4) return null;
  if (raw[0] !== SYNC_BYTE) return null;

  const opcode = raw[1];
  const len = raw[2];
  const payload = raw.slice(3, 3 + len);
  const crc = raw[3 + len];

  // Verify CRC
  const expected = crc8(raw.slice(0, 3 + len));
  if (expected !== crc) return null;

  return { opcode, payload, length: len };
}

// ---- Parse status response payload ----
export function parseStatusPayload(payload) {
  if (payload.length < 18) return null;

  const view = new DataView(payload.buffer);

  return {
    batteryPct:    payload[0],
    diveActive:    payload[1] !== 0,
    diveCount:     view.getUint16(2, true),
    currentDepth:  view.getFloat32(4, true),
    currentTemp:   view.getFloat32(8, true),
    currentSpeed:  view.getFloat32(12, true),
    currentHeading: view.getFloat32(16, true),
    quality:       payload[20] || 0,
  };
}

// ---- Parse profile data payload ----
export function parseProfilePayload(payload) {
  if (payload.length < 25) return null;

  const view = new DataView(payload.buffer);

  return {
    timestamp:  view.getUint32(0, true),
    depth:      view.getFloat32(4, true),
    temp:       view.getFloat32(8, true),
    vx:         view.getFloat32(12, true),
    vy:         view.getFloat32(16, true),
    vz:         view.getFloat32(20, true),
    quality:    payload[24] & 0x03,
    valid:      (payload[24] >> 2) & 0x01,
  };
}

// ---- Convert heading (rad) to compass direction string ----
export function headingToDirection(headingDeg) {
  const dirs = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
  const idx = Math.round(headingDeg / 45) % 8;
  return dirs[idx];
}

// ---- Format speed for display ----
export function formatSpeed(speedMs, units = 'metric') {
  if (units === 'imperial') {
    const kn = speedMs * 1.943844;  // m/s to knots
    return kn.toFixed(1) + ' kn';
  }
  return speedMs.toFixed(2) + ' m/s';
}

// ---- Format depth for display ----
export function formatDepth(depthM, units = 'metric') {
  if (units === 'imperial') {
    const ft = depthM * 3.28084;
    return ft.toFixed(1) + ' ft';
  }
  return depthM.toFixed(1) + ' m';
}

// ---- Export profile data to CSV ----
export function profileToCSV(records) {
  let csv = 'timestamp,depth_m,temp_c,vn_ms,ve_ms,vu_ms,speed_ms,heading_deg,quality\n';
  for (const r of records) {
    const speed = Math.sqrt(r.vx * r.vx + r.vy * r.vy + r.vz * r.vz);
    const heading = Math.atan2(r.vy, r.vx) * 180 / Math.PI;
    csv += `${r.timestamp},${r.depth.toFixed(2)},${r.temp.toFixed(1)},` +
           `${r.vx.toFixed(3)},${r.vy.toFixed(3)},${r.vz.toFixed(3)},` +
           `${speed.toFixed(3)},${heading.toFixed(1)},${r.quality}\n`;
  }
  return csv;
}