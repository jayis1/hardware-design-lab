/* FlangePrint packet codec.
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
'use strict';
function crc16(bytes) {
  let crc = 0xffff;
  for (const value of bytes) {
    crc ^= value << 8;
    for (let bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000)
        ? ((crc << 1) ^ 0x1021) & 0xffff
        : (crc << 1) & 0xffff;
    }
  }
  return crc;
}
function decodeStatus(input) {
  const bytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (bytes.length < 44 || bytes[0] !== 70 || bytes[1] !== 80)
    throw new Error('Not a FlangePrint status packet');
  const supplied = bytes[bytes.length - 2] | (bytes[bytes.length - 1] << 8);
  if (crc16(bytes.slice(0, -2)) !== supplied)
    throw new Error('Status CRC mismatch');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const gaps = [];
  for (let i = 0; i < 8; i++) gaps.push(view.getInt16(16 + i * 2, true) / 1000);
  return {
    version: bytes[2], state: bytes[3], sequence: view.getUint32(4, true),
    timestampMs: view.getUint32(8, true), batteryMv: view.getUint16(12, true),
    validMask: bytes[14], suggestedSector: bytes[15], gaps,
    meanGap: view.getInt16(32, true) / 1000,
    tilt: view.getInt16(34, true) / 1000,
    direction: view.getInt16(36, true) / 10,
    residual: view.getInt16(38, true) / 1000,
    acousticRisk: bytes[40], hottestSector: bytes[41]
  };
}
function command(code, payload = []) {
  const body = Uint8Array.from([code, ...payload]);
  const crc = crc16(body);
  return Uint8Array.from([...body, crc & 255, crc >> 8]);
}
function simulatedStatus(step = 0) {
  const gaps = Array.from({length: 8}, (_, i) => Math.max(0.82,
    2.4 - step * 0.022 + 0.55 * Math.exp(-step / 35) * Math.cos(i * Math.PI / 4)
    - 0.22 * Math.exp(-step / 40) * Math.sin(i * Math.PI / 4)
    + (i === 3 ? 0.13 : 0)));
  const mean = gaps.reduce((a, b) => a + b, 0) / 8;
  const x = gaps.reduce((a, g, i) => a + (g - mean) * Math.cos(i * Math.PI / 4), 0) / 4;
  const y = gaps.reduce((a, g, i) => a + (g - mean) * Math.sin(i * Math.PI / 4), 0) / 4;
  return {
    version: 1, state: step < 12 ? 2 : 3, sequence: 1,
    timestampMs: step * 160, batteryMv: 3890 - step, validMask: 255,
    suggestedSector: gaps.indexOf(Math.max(...gaps)), gaps, meanGap: mean,
    tilt: Math.hypot(x, y), direction: (Math.atan2(y, x) * 180 / Math.PI + 360) % 360,
    residual: 0.046, acousticRisk: step > 45 ? 52 : 18, hottestSector: 5
  };
}
if (typeof module !== 'undefined') module.exports = {crc16, decodeStatus, command, simulatedStatus};
if (typeof window !== 'undefined') window.FlangeProtocol = {crc16, decodeStatus, command, simulatedStatus};
