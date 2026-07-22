/**
 * BLE protocol — framing, commands, parsing
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
export const BLE_SERVICE_UUID = '0000f4c1-0000-1000-8000-00805f9b34fb';
export const BLE_RX_CHAR_UUID = '0000f4c2-0000-1000-8000-00805f9b34fb';
export const BLE_TX_CHAR_UUID = '0000f4c3-0000-1000-8000-00805f9b34fb';

export const CMD = {
  PING: 0x01,
  GET_STATUS: 0x02,
  SET_CONFIG: 0x03,
  START_SURVEY: 0x04,
  STOP_SURVEY: 0x05,
  TRIGGER: 0x06,
  LPR: 0x07,
  GET_CAL: 0x08,
  SET_CAL: 0x09,
  GET_HASH: 0x0a,
} as const;

export const RESP = {
  PONG: 0x81,
  STATUS: 0x82,
  CONFIG_ACK: 0x84,
  POINT: 0x86,
  LPR: 0x87,
  CAL: 0x88,
  HASH: 0x8a,
  ERROR: 0xe0,
} as const;

export const HCP_CLASS = {
  NO_CORROSION: 0,
  UNCERTAIN: 1,
  ACTIVE_CORROSION: 2,
} as const;

export const RESISTIVITY_CLASS = {
  LOW: 0,
  MODERATE: 1,
  HIGH: 2,
  VERY_HIGH: 3,
} as const;

export interface SurveyPoint {
  x_mm: number;
  y_mm: number;
  heading_deg: number;
  hcp_mv: number;
  hcp_class: number;
  rho_ohm_m: number;
  rho_class: number;
  cover_mm: number;
  rebar_diam_mm: number;
  timestamp_ms: number;
  hash: Uint8Array;
}

export function parseSurveyPoint(data: Uint8Array): SurveyPoint {
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const f32 = (off: number) => dv.getFloat32(off, true);
  const hash = data.slice(52, 84);
  return {
    x_mm: f32(0),
    y_mm: f32(4),
    heading_deg: f32(8),
    hcp_mv: f32(12),
    hcp_class: data[16],
    rho_ohm_m: f32(17),
    rho_class: data[21],
    cover_mm: f32(22),
    rebar_diam_mm: f32(26),
    timestamp_ms: dv.getUint32(30, true),
    hash,
  };
}

export interface DeviceStatus {
  state: number;
  ringHead: number;
  batteryPct: number;
  firmwareVersion: string;
  refElectrode: number;
  modalityMask: number;
}

export function parseStatus(data: Uint8Array): DeviceStatus {
  return {
    state: data[0],
    ringHead: data[1],
    batteryPct: data[2],
    firmwareVersion: `${data[3]}.${data[4]}`,
    refElectrode: data[5],
    modalityMask: data[6],
  };
}

export interface LprResult {
  vCorr_mm_yr: number;
  rp_kohm: number;
  iCorr_ua_cm2: number;
}

export function parseLprResult(data: Uint8Array): LprResult {
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  return {
    vCorr_mm_yr: dv.getFloat32(0, true),
    rp_kohm: dv.getFloat32(4, true),
    iCorr_ua_cm2: dv.getFloat32(8, true),
  };
}

export function hcpClassLabel(cls: number): string {
  switch (cls) {
    case HCP_CLASS.NO_CORROSION:
      return 'No corrosion (> -200 mV)';
    case HCP_CLASS.UNCERTAIN:
      return 'Uncertain (-200 to -350 mV)';
    case HCP_CLASS.ACTIVE_CORROSION:
      return 'Active corrosion (< -350 mV)';
    default:
      return 'Unknown';
  }
}

export function hcpClassColor(cls: number): string {
  switch (cls) {
    case HCP_CLASS.NO_CORROSION:
      return '#2ecc71';
    case HCP_CLASS.UNCERTAIN:
      return '#f39c12';
    case HCP_CLASS.ACTIVE_CORROSION:
      return '#e74c3c';
    default:
      return '#95a5a6';
  }
}

export function resistivityClassLabel(cls: number): string {
  switch (cls) {
    case RESISTIVITY_CLASS.LOW:
      return 'Low (< 50 kΩ·cm)';
    case RESISTIVITY_CLASS.MODERATE:
      return 'Moderate (50–100 kΩ·cm)';
    case RESISTIVITY_CLASS.HIGH:
      return 'High (100–200 kΩ·cm)';
    case RESISTIVITY_CLASS.VERY_HIGH:
      return 'Very high (≥ 200 kΩ·cm)';
    default:
      return 'Unknown';
  }
}

export function resistivityClassColor(cls: number): string {
  switch (cls) {
    case RESISTIVITY_CLASS.LOW:
      return '#e74c3c';
    case RESISTIVITY_CLASS.MODERATE:
      return '#f39c12';
    case RESISTIVITY_CLASS.HIGH:
      return '#2ecc71';
    case RESISTIVITY_CLASS.VERY_HIGH:
      return '#27ae60';
    default:
      return '#95a5a6';
  }
}