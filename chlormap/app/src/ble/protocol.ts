// src/ble/protocol.ts — Binary protocol parser for ChloroMap BLE packets
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

// BLE measurement packet format (48 bytes)
// All multi-byte fields are little-endian.

export interface Measurement {
  spad: number;          // SPAD equivalent (0-100)
  ndvi: number;          // NDVI (-0.2 to 1.0)
  nsi: number;           // Nitrogen sufficiency index
  lwbi: number;          // Leaf water band index
  rededge: number;       // Red-edge slope
  lat: number;           // Latitude (degrees)
  lon: number;           // Longitude (degrees)
  timestampMs: number;   // Unix timestamp (ms)
  bands: number[];       // 8 key band reflectances
  battMv: number;        // Battery voltage (mV)
  tempC: number;         // Temperature (°C)
  sats: number;          // GPS satellites
}

export interface DeviceStatus {
  battMv: number;
  state: number;
  sats: number;
  fixType: number;
  battPct: number;
}

const BLE_PKT_MAGIC = 0xCF;
const BLE_PKT_LEN = 48;

function readInt16LE(buf: Uint8Array, off: number): number {
  return (buf[off] | (buf[off + 1] << 8)) << 16 >> 16; // sign-extend
}

function readUInt16LE(buf: Uint8Array, off: number): number {
  return buf[off] | (buf[off + 1] << 8);
}

function readInt32LE(buf: Uint8Array, off: number): number {
  return (
    (buf[off] |
      (buf[off + 1] << 8) |
      (buf[off + 2] << 16) |
      (buf[off + 3] << 24)) |
    0
  );
}

function readUInt32LE(buf: Uint8Array, off: number): number {
  return (
    buf[off] |
    (buf[off + 1] << 8) |
    (buf[off + 2] << 16) |
    (buf[off + 3] << 24)
  ) >>> 0;
}

function crc8(buf: Uint8Array, len: number): number {
  let crc = 0;
  for (let i = 0; i < len; i++) {
    crc ^= buf[i];
    for (let b = 0; b < 8; b++) {
      crc = crc & 0x80 ? ((crc << 1) ^ 0x07) & 0xff : (crc << 1) & 0xff;
    }
  }
  return crc;
}

export function parseMeasurementPacket(data: Uint8Array): Measurement | null {
  if (data.length < BLE_PKT_LEN) return null;
  if (data[0] !== BLE_PKT_MAGIC) return null;

  // Verify CRC
  const calcCrc = crc8(data, BLE_PKT_LEN - 2);
  if (calcCrc !== data[BLE_PKT_LEN - 2]) return null;

  const spad = readInt16LE(data, 2);
  const ndviX1000 = readInt16LE(data, 4);
  const nsiX1000 = readInt16LE(data, 6);
  const lwbiX1000 = readInt16LE(data, 8);
  const rededgeX1000 = readInt16LE(data, 10);
  const latE7 = readInt32LE(data, 12);
  const lonE7 = readInt32LE(data, 16);
  const timestampMs = readUInt32LE(data, 20);

  // 8 key bands: 450, 531, 660, 680, 700, 800, 900, 970 nm
  const bandOffsets = [24, 26, 28, 30, 32, 34, 36, 38];
  const bands = bandOffsets.map((off) => readInt16LE(data, off));

  const battMv = readUInt16LE(data, 40);
  const tempCX10 = readInt16LE(data, 42);
  const sats = data[44];

  return {
    spad,
    ndvi: ndviX1000 / 1000,
    nsi: nsiX1000 / 1000,
    lwbi: lwbiX1000 / 1000,
    rededge: rededgeX1000 / 1000,
    lat: latE7 / 1e7,
    lon: lonE7 / 1e7,
    timestampMs,
    bands: bands.map((b) => b / 1000),
    battMv,
    tempC: tempCX10 / 10,
    sats,
  };
}

export function parseStatusPacket(data: Uint8Array): DeviceStatus | null {
  if (data.length < 16) return null;
  if (data[0] !== BLE_PKT_MAGIC) return null;

  const calcCrc = crc8(data, 15);
  if (calcCrc !== data[15]) return null;

  const battMv = readUInt16LE(data, 2);
  const state = data[4];
  const sats = data[5];
  const fixType = data[6];

  // Calculate battery percentage: 3400-4200 mV → 0-100%
  const battPct = Math.max(
    0,
    Math.min(100, Math.round(((battMv - 3400) / 800) * 100))
  );

  return { battMv, state, sats, fixType, battPct };
}

// Band wavelength labels (nm)
export const BAND_WAVELENGTHS = [450, 531, 660, 680, 700, 800, 900, 970];

// Chlorophyll (SPAD) interpretation
export function interpretSpad(spad: number): { label: string; color: string } {
  if (spad < 20) return { label: 'Very Low N', color: '#d32f2f' };
  if (spad < 35) return { label: 'Low N', color: '#f57c00' };
  if (spad < 50) return { label: 'Moderate N', color: '#fbc02d' };
  if (spad < 65) return { label: 'Sufficient N', color: '#689f38' };
  return { label: 'High N', color: '#2e7d32' };
}

// NDVI interpretation
export function interpretNdvi(ndvi: number): { label: string; color: string } {
  if (ndvi < 0.3) return { label: 'Stressed', color: '#d32f2f' };
  if (ndvi < 0.5) return { label: 'Moderate', color: '#fbc02d' };
  if (ndvi < 0.7) return { label: 'Healthy', color: '#689f38' };
  return { label: 'Very Healthy', color: '#2e7d32' };
}

// LWBI (water) interpretation
export function interpretLwbi(lwbi: number): { label: string; color: string } {
  if (lwbi < 0.95) return { label: 'Water Stressed', color: '#d32f2f' };
  if (lwbi < 1.05) return { label: 'Moderate', color: '#fbc02d' };
  return { label: 'Well Hydrated', color: '#2e7d32' };
}