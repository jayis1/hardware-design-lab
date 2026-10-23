// QuadraAlign BLE protocol and leveling model
// Author: jayis1
export const AUTHOR = 'jayis1';
export const SERVICE_UUID = '7a100001-6fd5-4b6c-a4f8-34aa51414c31';
export const TELEMETRY_UUID = '7a100002-6fd5-4b6c-a4f8-34aa51414c31';
export const COMMAND_UUID = '7a100003-6fd5-4b6c-a4f8-34aa51414c31';

export type FootIndex = 0 | 1 | 2 | 3;
export interface Measurement {
  sequence: number;
  feet: [number, number, number, number];
  totalKg: number;
  imbalancePercent: number;
  centerXmm: number;
  centerYmm: number;
  pitchDeg: number;
  rollDeg: number;
  vibrationRmsG: number;
  recommendedFoot: FootIndex;
  recommendedTurns: number;
  batteryMv: number;
  stable: boolean;
  overloaded: boolean;
  receivedAt: number;
}
export interface Capture extends Measurement { id: string; label: string; }
export interface Project {
  name: string;
  widthMm: number;
  depthMm: number;
  screwPitchMm: number;
  imbalanceLimit: number;
  tiltLimit: number;
}

const crc16 = (data: Uint8Array): number => {
  let crc = 0xffff;
  data.forEach(byte => {
    crc ^= byte << 8;
    for (let bit = 0; bit < 8; bit += 1) crc = (crc & 0x8000) !== 0 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff;
  });
  return crc;
};

export const decodeTelemetry = (frame: Uint8Array): Measurement => {
  if (frame.length < 5 || frame[0] !== 0xa5 || frame[1] !== 0x01) throw new Error('Invalid telemetry frame');
  const payloadLength = frame[2];
  if (frame.length !== payloadLength + 5) throw new Error('Truncated telemetry frame');
  const expected = frame[frame.length - 2] | (frame[frame.length - 1] << 8);
  if (crc16(frame.slice(1, -2)) !== expected) throw new Error('Telemetry CRC mismatch');
  const view = new DataView(frame.buffer, frame.byteOffset + 3, payloadLength);
  let at = 0;
  const u32 = () => { const v = view.getUint32(at, true); at += 4; return v; };
  const f32 = () => { const v = view.getFloat32(at, true); at += 4; return v; };
  const sequence = u32();
  const feet: [number, number, number, number] = [f32(), f32(), f32(), f32()];
  const totalKg = f32();
  const imbalancePercent = f32();
  const centerXmm = f32();
  const centerYmm = f32();
  const pitchDeg = f32();
  const rollDeg = f32();
  const vibrationRmsG = f32();
  const recommendedFoot = view.getUint8(at++) as FootIndex;
  const recommendedTurns = f32();
  const batteryMv = view.getUint16(at, true); at += 2;
  const flags = view.getUint8(at);
  return { sequence, feet, totalKg, imbalancePercent, centerXmm, centerYmm, pitchDeg, rollDeg,
    vibrationRmsG, recommendedFoot, recommendedTurns, batteryMv, stable: (flags & 1) !== 0,
    overloaded: (flags & 2) !== 0, receivedAt: Date.now() };
};

const commandFrame = (payload: number[]): Uint8Array => {
  const frame = new Uint8Array(payload.length + 5);
  frame.set([0xa5, 0x10, payload.length, ...payload]);
  const crc = crc16(frame.slice(1, -2));
  frame[frame.length - 2] = crc & 0xff;
  frame[frame.length - 1] = crc >> 8;
  return frame;
};

const floatBytes = (value: number): number[] => {
  const bytes = new Uint8Array(4);
  new DataView(bytes.buffer).setFloat32(0, value, true);
  return Array.from(bytes);
};
export const commands = {
  tare: () => commandFrame([0x01]),
  capture: () => commandFrame([0x02]),
  setLimits: (imbalance: number, tilt: number) => commandFrame([0x03, ...floatBytes(imbalance), ...floatBytes(tilt)]),
  calibrate: (foot: FootIndex, kg: number) => commandFrame([0x04, foot, ...floatBytes(kg)]),
  sleep: () => commandFrame([0x05]),
};

export const demoMeasurement = (phase: number): Measurement => {
  const feet = [112 + Math.sin(phase) * 2, 108 - Math.sin(phase) * 2, 111, 109] as [number, number, number, number];
  return { sequence: Math.floor(phase * 10), feet, totalKg: feet.reduce((a, b) => a + b, 0),
    imbalancePercent: 3.6, centerXmm: -2.5, centerYmm: 1.1, pitchDeg: 0.18,
    rollDeg: -0.11, vibrationRmsG: 0.006, recommendedFoot: 1, recommendedTurns: 0.42,
    batteryMv: 4012, stable: true, overloaded: false, receivedAt: Date.now() };
};
