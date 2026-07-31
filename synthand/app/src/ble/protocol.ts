/**
 * protocol.ts — BLE GATT protocol definitions for Synthand.
 *
 * Defines UUIDs, message types, and parsing functions for the
 * BLE-MIDI and OSC communication between the app and the glove.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

// BLE-MIDI Service UUID (standard MIDI Association)
export const MIDI_SERVICE_UUID = '03b80e5a-a84b-460d-9e0f-8c0d84e766e0';
export const MIDI_CHAR_UUID = '7772e5db-3868-4112-a1a9-f2669d106bf3';

// Synthand OSC custom service
export const OSC_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
export const OSC_TX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';

// Synthand configuration service (custom)
export const CONFIG_SERVICE_UUID = '6e400010-b5a3-f393-e0a9-e50e24dcca9e';
export const CONFIG_CHAR_UUID = '6e400011-b5a3-f393-e0a9-e50e24dcca9e';
export const CALIBRATION_CHAR_UUID = '6e400012-b5a3-f393-e0a9-e50e24dcca9e';

// Device info service (standard)
export const DEVICE_INFO_SERVICE_UUID = '0000180a-0000-1000-8000-00805f9b34fb';
export const FIRMWARE_VERSION_UUID = '00002a26-0000-1000-8000-00805f9b34fb';
export const MANUFACTURER_UUID = '00002a29-0000-1000-8000-00805f9b34fb';
export const BATTERY_SERVICE_UUID = '0000180f-0000-1000-8000-00805f9b34fb';
export const BATTERY_LEVEL_UUID = '00002a19-0000-1000-8000-00805f9b34fb';

// MIDI status bytes
export const MIDI_STATUS_NOTE_OFF = 0x80;
export const MIDI_STATUS_NOTE_ON = 0x90;
export const MIDI_STATUS_CC = 0xb0;
export const MIDI_STATUS_PROGRAM_CHANGE = 0xc0;
export const MIDI_STATUS_CHANNEL_PRESSURE = 0xd0;
export const MIDI_STATUS_PITCH_BEND = 0xe0;

// Gesture IDs (must match firmware gesture.h)
export enum GestureId {
  TAP = 0,
  PRESS = 1,
  RELEASE = 2,
  PLUCK = 3,
  STRUM_DOWN = 4,
  STRUM_UP = 5,
  VIBRATO = 6,
  TREMOLO = 7,
  GLIDE = 8,
  FIST = 9,
  OPEN = 10,
  SNAP = 11,
}

export const GESTURE_NAMES: Record<number, string> = {
  0: 'Tap',
  1: 'Press',
  2: 'Release',
  3: 'Pluck',
  4: 'Strum Down',
  5: 'Strum Up',
  6: 'Vibrato',
  7: 'Tremolo',
  8: 'Glide',
  9: 'Fist',
  10: 'Open',
  11: 'Snap',
};

// Finger names
export const FINGER_NAMES = ['Thumb', 'Index', 'Middle', 'Ring', 'Pinky'];

// MIDI event interface
export interface MidiEvent {
  status: number;
  channel: number;
  data1: number;
  data2: number;
  timestamp: number;
}

// OSC data interface
export interface OscData {
  emgEnvelopes: number[];      // 5 channels, 0.0-1.0
  fingerCurls: number[];       // 5 fingers, 0.0-1.0
  fingerVelocities: number[];  // 5 fingers, 0.0-1.0
  wristQuaternion: number[];   // w, x, y, z
  gestureId: number;
  gestureConfidence: number;
}

// Calibration data interface
export interface CalibrationData {
  emgBaseline: number[];
  emgMvc: number[];
  gyroBias: number[][];
  accelBias: number[][];
  handedness: number;
}

// Mapping data interface
export interface MappingData {
  midiChannel: number;
  notes: number[];
  ccEmg: number[];
  ccCurl: number[];
  ccVibrato: number;
  ccMod: number;
  hapticWaveforms: number[];
  emgThreshold: number;
  tapAccelThreshold: number;
  vibratoSensitivity: number;
}

// Default mapping
export const DEFAULT_MAPPING: MappingData = {
  midiChannel: 0,
  notes: [36, 38, 42, 46, 49],
  ccEmg: [20, 21, 22, 23, 24],
  ccCurl: [30, 31, 32, 33, 34],
  ccVibrato: 35,
  ccMod: 1,
  hapticWaveforms: [17, 22, 0, 47, 17, 17, 0, 17, 0, 72, 0, 56],
  emgThreshold: 0x4000,
  tapAccelThreshold: 200,
  vibratoSensitivity: 5,
};

/**
 * Parse a BLE-MIDI packet into individual MIDI events.
 * BLE-MIDI format: [header byte] [timestamp hi+lo] [MIDI msg] ...
 * Author: jayis1
 */
export function parseBleMidiPacket(data: number[]): MidiEvent[] {
  const events: MidiEvent[] = [];
  if (data.length < 3) return events;

  // Header byte: bits 6:0 = timestamp hi (bits 12:6 of 13-bit timestamp)
  const header = data[0];
  const tsHi = (header & 0x3f) << 7;

  let pos = 1;
  let runningStatus = 0;
  let lastTsLo = 0;

  while (pos < data.length) {
    // Check if this is a timestamp byte (bit 7 = 1)
    if (data[pos] & 0x80) {
      lastTsLo = data[pos] & 0x7f;
      pos++;
      if (pos >= data.length) break;
    }

    // Check if this is a status byte
    if (data[pos] & 0x80) {
      runningStatus = data[pos];
      pos++;
    }

    // Now we have a status byte (from running status or just read)
    const status = runningStatus;
    const statusType = status & 0xf0;
    const channel = status & 0x0f;
    const timestamp = (tsHi | lastTsLo) * 1; // ms

    if (statusType === MIDI_STATUS_PROGRAM_CHANGE ||
        statusType === MIDI_STATUS_CHANNEL_PRESSURE) {
      // 1 data byte
      if (pos >= data.length) break;
      events.push({
        status: statusType,
        channel,
        data1: data[pos] & 0x7f,
        data2: 0,
        timestamp,
      });
      pos++;
    } else {
      // 2 data bytes
      if (pos + 1 >= data.length) break;
      events.push({
        status: statusType,
        channel,
        data1: data[pos] & 0x7f,
        data2: data[pos + 1] & 0x7f,
        timestamp,
      });
      pos += 2;
    }
  }

  return events;
}

/**
 * Parse OSC bundle data received via GATT characteristic.
 * Author: jayis1
 */
export function parseOscBundle(data: number[]): OscData {
  const result: OscData = {
    emgEnvelopes: [0, 0, 0, 0, 0],
    fingerCurls: [0, 0, 0, 0, 0],
    fingerVelocities: [0, 0, 0, 0, 0],
    wristQuaternion: [1, 0, 0, 0],
    gestureId: -1,
    gestureConfidence: 0,
  };

  // Simplified OSC parsing — in production this would properly
  // decode OSC address patterns and type tags from the byte array.
  // The data format from the firmware is a structured binary blob
  // that encodes the same information as the OSC address space.
  if (data.length < 20) return result;

  let offset = 0;

  // EMG envelopes (5 × float32 = 20 bytes)
  for (let i = 0; i < 5; i++) {
    const bytes = data.slice(offset, offset + 4);
    result.emgEnvelopes[i] = bytesToFloat(bytes);
    offset += 4;
  }

  // Finger curls (5 × float32 = 20 bytes)
  for (let i = 0; i < 5; i++) {
    const bytes = data.slice(offset, offset + 4);
    result.fingerCurls[i] = bytesToFloat(bytes);
    offset += 4;
  }

  // Finger velocities (5 × float32 = 20 bytes)
  for (let i = 0; i < 5; i++) {
    const bytes = data.slice(offset, offset + 4);
    result.fingerVelocities[i] = bytesToFloat(bytes);
    offset += 4;
  }

  // Wrist quaternion (4 × float32 = 16 bytes)
  for (let i = 0; i < 4; i++) {
    const bytes = data.slice(offset, offset + 4);
    result.wristQuaternion[i] = bytesToFloat(bytes);
    offset += 4;
  }

  // Gesture ID (int32) + confidence (float32)
  if (offset + 8 <= data.length) {
    result.gestureId = (data[offset] << 24) | (data[offset + 1] << 16) |
                       (data[offset + 2] << 8) | data[offset + 3];
    offset += 4;
    const bytes = data.slice(offset, offset + 4);
    result.gestureConfidence = bytesToFloat(bytes);
  }

  return result;
}

function bytesToFloat(bytes: number[]): number {
  // Big-endian IEEE 754 float
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  view.setUint8(0, bytes[0] || 0);
  view.setUint8(1, bytes[1] || 0);
  view.setUint8(2, bytes[2] || 0);
  view.setUint8(3, bytes[3] || 0);
  return view.getFloat32(0, false); // big-endian
}