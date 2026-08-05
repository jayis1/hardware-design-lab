// src/ble/protocol.ts — FrostSentinel BLE protocol definitions
//
// Defines the GATT service/characteristic UUIDs and the binary framing
// protocol used between the FrostSentinel node and this companion app.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

// Custom GATT service UUID for FrostSentinel
export const FrostSentinelServiceUUID = 'f5b00001-5b3e-4f6a-9c2d-1e7f8a3b5c4d';

// Characteristic UUIDs
export const FrostSentinelCharUUIDs = {
  liveData:  'f5b00002-5b3e-4f6a-9c2d-1e7f8a3b5c4d', // notify
  command:   'f5b00003-5b3e-4f6a-9c2d-1e7f8a3b5c4d', // write
  logDump:   'f5b00004-5b3e-4f6a-9c2d-1e7f8a3b5c4d', // notify (chunked)
  status:    'f5b00005-5b3e-4f6a-9c2d-1e7f8a3b5c4d', // read
};

// Frame format:
//   [SOF=0xA5] [type:1] [len:1] [payload:len] [checksum:1] [EOF=0x5A]
export const SOF = 0xA5;
export const EOF = 0x5A;

export interface BleFrame {
  type: number;
  payload: Uint8Array;
}

export function parseFrame(hexString: string): BleFrame | null {
  // Convert hex string to byte array
  const bytes: number[] = [];
  for (let i = 0; i < hexString.length; i += 2) {
    bytes.push(parseInt(hexString.substr(i, 2), 16));
  }
  if (bytes.length < 5) return null;
  if (bytes[0] !== SOF) return null;
  if (bytes[bytes.length - 1] !== EOF) return null;

  const type = bytes[1];
  const len = bytes[2];
  if (bytes.length !== len + 5) return null;

  const payload = new Uint8Array(bytes.slice(3, 3 + len));

  // Verify checksum (XOR of type, len, and payload)
  let cksum = type ^ len;
  for (let i = 0; i < payload.length; i++) {
    cksum ^= payload[i];
  }
  if (cksum !== bytes[3 + len]) return null;

  return { type, payload };
}

export function buildFrame(type: number, payload: Uint8Array): string {
  const len = payload.length;
  let cksum = type ^ len;
  for (let i = 0; i < payload.length; i++) {
    cksum ^= payload[i];
  }
  const bytes = [SOF, type, len, ...Array.from(payload), cksum, EOF];
  return bytes.map(b => b.toString(16).padStart(2, '0')).join('');
}

// Mesh message types (match firmware radio.h)
export const MESH_MSG_DATA    = 0x01;
export const MESH_MSG_ALERT   = 0x02;
export const MESH_MSG_BEACON  = 0x03;
export const MESH_MSG_JOIN    = 0x04;
export const MESH_MSG_ACK     = 0x05;

// AE status codes (match firmware acoustic.h)
export const AE_STATUS_IDLE       = 0;
export const AE_STATUS_ARMED      = 1;
export const AE_STATUS_NUCLEATION = 2;

// RFRI thresholds (match firmware board.h)
export const RFRI_GREEN  = 0.30;
export const RFRI_YELLOW = 0.60;
export const RFRI_RED    = 0.85;

// Helper: get RFRI color
export function rfriColor(rfri: number): string {
  if (rfri >= RFRI_RED)    return '#F44336'; // red
  if (rfri >= RFRI_YELLOW) return '#FF9800'; // orange
  if (rfri >= RFRI_GREEN)  return '#FFC107'; // yellow
  return '#4CAF50';                              // green
}

// Helper: AE status label
export function aeStatusLabel(status: number): string {
  switch (status) {
    case AE_STATUS_IDLE:       return 'Idle';
    case AE_STATUS_ARMED:      return 'Armed';
    case AE_STATUS_NUCLEATION: return 'ICE DETECTED';
    default: return 'Unknown';
  }
}

// Helper: time-to-critical-freeze label
export function ttcLabel(hours: number): string {
  if (hours === 255) return '> 12 h';
  if (hours === 0)   return 'CRITICAL NOW';
  return `${hours} h`;
}