// protocol.ts — Wire protocol definitions for the Inkwell BLE GATT service
//
// Defines the UUIDs of the Inkwell custom service and its characteristics,
// the 20-byte stroke segment notification payload layout, and the parse /
// serialize helpers used by BleManager and the screens.
//
// Payload (20 bytes, little-endian):
//   byte 0      flags (bit0 pen-down, bit1 stroke-start, bit2 stroke-end,
//                      bit3 optical-flow-valid)
//   bytes 1-4   seq (uint32)
//   bytes 5-8   ts_ms (uint32)
//   bytes 9-12  dx_um (int32)
//   bytes 13-16 dy_um (int32)
//   bytes 17-18 p_mN (uint16)
//   byte 19     crc8
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

export const UUIDS = {
  INKWELL_SERVICE:  '1b7e0001-0000-0000-0000-000000000000',
  STROKE_DATA:      '1b7e0002-0000-0000-0000-000000000000',
  CONTROL:          '1b7e0003-0000-0000-0000-000000000000',
  STATUS:           '1b7e0004-0000-0000-0000-000000000000',
  JOURNAL_REPLAY:   '1b7e0005-0000-0000-0000-000000000000',
} as const;

export type StrokeFlags = {
  penDown: boolean;
  strokeStart: boolean;
  strokeEnd: boolean;
  opticalFlowValid: boolean;
};

export type StrokeSegment = {
  flags: StrokeFlags;
  seq: number;
  tsMs: number;
  dxUm: number;
  dyUm: number;
  pressureMN: number;
  crc8: number;
};

export function parseStrokeSegment(base64: string): StrokeSegment {
  const buf = Buffer.from(base64, 'base64');
  const flagsByte = buf.readUInt8(0);
  return {
    flags: {
      penDown:           (flagsByte & 0x01) !== 0,
      strokeStart:       (flagsByte & 0x02) !== 0,
      strokeEnd:         (flagsByte & 0x04) !== 0,
      opticalFlowValid:  (flagsByte & 0x08) !== 0,
    },
    seq:        buf.readUInt32LE(1),
    tsMs:       buf.readUInt32LE(5),
    dxUm:       buf.readInt32LE(9),
    dyUm:       buf.readInt32LE(13),
    pressureMN: buf.readUInt16LE(17),
    crc8:       buf.readUInt8(19),
  };
}

/** Serialize a 10-byte replay-range request: seqStart (u32) + seqEnd (u32) + cmd (u8) + crc8. */
export function serializeReplayRequest(seqStart: number, seqEnd: number): string {
  const buf = Buffer.alloc(10);
  buf.writeUInt32LE(seqStart, 0);
  buf.writeUInt32LE(seqEnd, 4);
  buf.writeUInt8(0x01, 8); // cmd = replay range
  let crc = 0;
  for (let i = 0; i < 9; i++) crc ^= buf[i];
  buf.writeUInt8(crc, 9);
  return buf.toString('base64');
}

/** Control commands sent via the Control characteristic. */
export const ControlCommand = {
  START_SESSION: 0x01,
  STOP_SESSION: 0x02,
  SET_SEGMENT_RATE: 0x03,
  ENTER_DFU: 0x04,
} as const;

/** Power state enum (matches firmware board.h). */
export const PowerState = {
  OFF: 0,
  ADVERTISING: 1,
  CONNECTED_IDLE: 2,
  WRITING: 3,
  CHARGING: 4,
} as const;

export function powerStateName(state: number): string {
  switch (state) {
    case PowerState.OFF:             return 'Off';
    case PowerState.ADVERTISING:     return 'Advertising';
    case PowerState.CONNECTED_IDLE:  return 'Connected (idle)';
    case PowerState.WRITING:         return 'Writing';
    case PowerState.CHARGING:        return 'Charging';
    default:                         return 'Unknown';
  }
}