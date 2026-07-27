/**
 * protocol.ts — GATT characteristic definitions and binary data unpacking.
 *
 * Defines the UUIDs and binary format for the LithoCore BLE protocol.
 * All data is packed as little-endian binary.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

// GATT Service and Characteristic UUIDs
export const LITHOCORE_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
export const CMD_CHARACTERISTIC_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
export const SWEEP_DATA_CHARACTERISTIC_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';
export const RESULT_CHARACTERISTIC_UUID = '6e400004-b5a3-f393-e0a9-e50e24dcca9e';
export const STATUS_CHARACTERISTIC_UUID = '6e400005-b5a3-f393-e0a9-e50e24dcca9e';

// Degradation modes (must match firmware soh.h)
export enum DegradationMode {
  Healthy = 0,
  SEIGrowth = 1,
  LithiumPlating = 2,
  ElectrolyteDryout = 3,
  InternalShort = 4,
  Unknown = 5,
}

export enum QualityVerdict {
  Excellent = 0,
  Good = 1,
  Fair = 2,
  Poor = 3,
  Replace = 4,
}

// Chemistry names (must match firmware board.h)
export const CHEMISTRY_NAMES = [
  'NMC-18650', 'NMC-21700', 'LFP-26650', 'NCA-18650', 'LCO-pack',
];

export const DEGRADATION_NAMES: Record<DegradationMode, string> = {
  [DegradationMode.Healthy]: 'Healthy',
  [DegradationMode.SEIGrowth]: 'SEI Growth',
  [DegradationMode.LithiumPlating]: 'Lithium Plating',
  [DegradationMode.ElectrolyteDryout]: 'Electrolyte Dry-out',
  [DegradationMode.InternalShort]: 'Internal Short',
  [DegradationMode.Unknown]: 'Unknown',
};

export const VERDICT_NAMES: Record<QualityVerdict, string> = {
  [QualityVerdict.Excellent]: 'EXCELLENT',
  [QualityVerdict.Good]: 'GOOD',
  [QualityVerdict.Fair]: 'FAIR',
  [QualityVerdict.Poor]: 'POOR',
  [QualityVerdict.Replace]: 'REPLACE',
};

export const VERDICT_COLORS: Record<QualityVerdict, string> = {
  [QualityVerdict.Excellent]: '#00e676',
  [QualityVerdict.Good]: '#76ff03',
  [QualityVerdict.Fair]: '#ffeb3b',
  [QualityVerdict.Poor]: '#ff9800',
  [QualityVerdict.Replace]: '#f44336',
};

// --- Data types ---

export interface SweepPoint {
  freqHz: number;
  reZ: number;    // Re(Z) in mΩ
  imZ: number;    // Im(Z) in mΩ
  mag: number;    // |Z| in mΩ
  phase: number;  // phase in millidegrees
  flags: number;
}

export interface RandlesParams {
  rsMohm: number;
  rseiMohm: number;
  cseimF: number;
  rctMohm: number;
  cdlmF: number;
  sigma: number;
}

export interface CellResult {
  sohScore: number;
  degradation: DegradationMode;
  verdict: QualityVerdict;
  ocvMv: number;
  tempDc: number;
  dcirMohm: number;
  selfDischargeUvPerMin: number;
  chemistryIdx: number;
  fitValid: boolean;
  randles: RandlesParams | null;
  sweepPoints: SweepPoint[];
  timestamp: number;
}

export interface DeviceStatus {
  state: number;
  progress: number;
  resultValid: boolean;
}

// --- Binary packing/unpacking ---

// Base64 decode helper (react-native-ble-plx returns base64 strings)
function base64ToBytes(b64: string): Uint8Array {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = b64.replace(/=+$/, '');
  const bytes: number[] = [];
  let buffer = 0;
  let bits = 0;
  for (const c of clean) {
    const val = chars.indexOf(c);
    if (val === -1) continue;
    buffer = (buffer << 6) | val;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      bytes.push((buffer >> bits) & 0xff);
    }
  }
  return new Uint8Array(bytes);
}

function readInt32LE(bytes: Uint8Array, offset: number): number {
  return (
    (bytes[offset]) |
    (bytes[offset + 1] << 8) |
    (bytes[offset + 2] << 16) |
    (bytes[offset + 3] << 24)
  );
}

function readUint16LE(bytes: Uint8Array, offset: number): number {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

function readInt16LE(bytes: Uint8Array, offset: number): number {
  const val = readUint16LE(bytes, offset);
  return val > 0x7fff ? val - 0x10000 : val;
}

/**
 * Unpack a sweep data point (21 bytes).
 * Format: [freqHz(4)][reZ(4)][imZ(4)][mag(4)][phase(4)][flags(1)]
 */
export function unpackSweepPoint(b64: string): SweepPoint | null {
  const bytes = base64ToBytes(b64);
  if (bytes.length < 21) return null;
  return {
    freqHz: readInt32LE(bytes, 0) >>> 0,  // unsigned
    reZ: readInt32LE(bytes, 4),
    imZ: readInt32LE(bytes, 8),
    mag: readInt32LE(bytes, 12),
    phase: readInt32LE(bytes, 16),
    flags: bytes[20],
  };
}

/**
 * Unpack a result notification.
 * The result is sent as multiple frames: summary first, then sweep points.
 * This function handles the summary frame; sweep points are accumulated
 * separately.
 */
export function unpackResult(b64: string): CellResult | null {
  const bytes = base64ToBytes(b64);
  if (bytes.length < 15) return null;

  const sohScore = bytes[0];
  const degradation = bytes[1] as DegradationMode;
  const verdict = bytes[2] as QualityVerdict;
  const ocvMv = readUint16LE(bytes, 3);
  const tempDc = readUint16LE(bytes, 5);
  const dcirMohm = readUint16LE(bytes, 7);
  const selfDischarge = readInt32LE(bytes, 9);
  const chemistryIdx = bytes[13];
  const fitValid = bytes[14] === 1;

  let randles: RandlesParams | null = null;
  if (fitValid && bytes.length >= 15 + 24) {
    randles = {
      rsMohm: readInt32LE(bytes, 15),
      rseiMohm: readInt32LE(bytes, 19),
      cseimF: readInt32LE(bytes, 23),
      rctMohm: readInt32LE(bytes, 27),
      cdlmF: readInt32LE(bytes, 31),
      sigma: readInt32LE(bytes, 35),
    };
  }

  return {
    sohScore,
    degradation,
    verdict,
    ocvMv,
    tempDc,
    dcirMohm,
    selfDischargeUvPerMin: selfDischarge,
    chemistryIdx,
    fitValid,
    randles,
    sweepPoints: [],  // filled by sweep data notifications
    timestamp: Date.now(),
  };
}

/**
 * Unpack a status notification (3 bytes).
 */
export function unpackStatus(b64: string): DeviceStatus | null {
  const bytes = base64ToBytes(b64);
  if (bytes.length < 3) return null;
  return {
    state: bytes[0],
    progress: bytes[1],
    resultValid: bytes[2] === 1,
  };
}

/**
 * Pack a command to send to the device.
 */
export function packCommand(command: number, data?: Uint8Array): string {
  const len = data?.length || 0;
  const bytes = new Uint8Array(2 + len);
  bytes[0] = command;
  bytes[1] = len;
  if (data && len > 0) {
    bytes.set(data, 2);
  }
  // Convert to base64
  let b64 = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b0 = bytes[i];
    const b1 = bytes[i + 1] || 0;
    const b2 = bytes[i + 2] || 0;
    b64 += 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'[
      (b0 >> 2) & 0x3f
    ];
    b64 += 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'[
      ((b0 << 4) | (b1 >> 4)) & 0x3f
    ];
    b64 += 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'[
      ((b1 << 2) | (b2 >> 6)) & 0x3f
    ];
    b64 += 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'[
      b2 & 0x3f
    ];
  }
  return b64;
}