// ============================================================
// LignoScan App — BLE Protocol Parser
// Parses binary packets received from the LignoScan scanner
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

// BLE Service UUID for LignoScan
export const LIGNOSCAN_SERVICE = '0000LIGN-0000-1000-8000-00805F9B34FB';

// BLE state codes
export const BLE_STATE = {
  IDLE: 0x00,
  CALIBRATING: 0x01,
  SCANNING: 0x02,
  RECONSTRUCT: 0x03,
  TRANSMITTING: 0x04,
  ERROR: 0xFF,
};

// Decay classification codes
export const DECAY_CLASS = {
  SOUND: 0,
  MODERATE: 1,
  SEVERE: 2,
  HOLLOW: 3,
};

// Decay classification labels and colors
export const DECAY_LABELS = {
  0: 'Sound Wood',
  1: 'Moderate Decay',
  2: 'Severe Decay',
  3: 'Hollow',
};

export const DECAY_COLORS = {
  0: '#2d8a2d',    // Green
  1: '#e6c200',    // Yellow
  2: '#e65c00',    // Orange
  3: '#cc0000',    // Red
};

// ---- Parse status packet ----
// Format: [type(1)] [state(1)] [progress(4)]
export function parseStatusPacket(bytes) {
  if (bytes.length < 5) return null;
  const state = bytes[1];
  const progress = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
  return {
    state,
    stateLabel: getStateLabel(state),
    progress: Math.min(progress, 100),
  };
}

function getStateLabel(state) {
  switch (state) {
    case BLE_STATE.IDLE: return 'Idle';
    case BLE_STATE.CALIBRATING: return 'Calibrating';
    case BLE_STATE.SCANNING: return 'Scanning';
    case BLE_STATE.RECONSTRUCT: return 'Reconstructing';
    case BLE_STATE.TRANSMITTING: return 'Transmitting';
    case BLE_STATE.ERROR: return 'Error';
    default: return 'Unknown';
  }
}

// ---- Parse ToF matrix packet ----
// Format: [type(1)] [n_sensors(1)] [reserved(1)] [pairs... (4 bytes each float32)]
export function parseTofMatrix(bytes) {
  if (bytes.length < 3) return null;
  const nSensors = bytes[1];
  const pairs = [];

  const view = new DataView(bytes.buffer, bytes.byteOffset);
  let offset = 3;

  while (offset + 4 <= bytes.length) {
    const tof = view.getFloat32(offset, true); // little-endian
    pairs.push(tof);
    offset += 4;
  }

  return {
    nSensors,
    pairs,
    // Reconstruct matrix from upper triangle
    matrix: reconstructMatrix(pairs, nSensors),
  };
}

function reconstructMatrix(pairs, n) {
  const matrix = Array(n).fill(null).map(() => Array(n).fill(0));
  let idx = 0;
  for (let tx = 0; tx < n; tx++) {
    for (let rx = tx + 1; rx < n; rx++) {
      matrix[tx][rx] = pairs[idx] || 0;
      matrix[rx][tx] = pairs[idx] || 0; // Symmetric
      idx++;
    }
  }
  return matrix;
}

// ---- Parse tomogram packet ----
// Format: [type(1)] [n_cells_lo(1)] [n_cells_hi(1)] [seq(1)] [velocity(4) class(1)] per cell
export function parseTomogram(bytes) {
  if (bytes.length < 4) return null;
  const nCells = bytes[1] | (bytes[2] << 8);
  const cells = [];

  const view = new DataView(bytes.buffer, bytes.byteOffset);
  let offset = 4;

  while (offset + 5 <= bytes.length && cells.length < nCells) {
    const velocity = view.getFloat32(offset, true);
    const classification = bytes[offset + 4];
    cells.push({ velocity, classification });
    offset += 5;
  }

  return {
    nCells,
    cells,
    // Compute TDI (Tomographic Decay Index)
    tdi: computeTDI(cells),
  };
}

function computeTDI(cells) {
  if (!cells || cells.length === 0) return 0;
  const decayed = cells.filter(c => c.classification !== DECAY_CLASS.SOUND).length;
  return decayed / cells.length;
}

// ---- Parse GPS data packet ----
// Format: [type(1)] [lat(4)] [lon(4)] [alt(4)] [hdop(4)] [fix(1)] [sats(1)] [ts_len(1)] [ts...]
export function parseGpsData(bytes) {
  if (bytes.length < 19) return null;
  const view = new DataView(bytes.buffer, bytes.byteOffset);
  let offset = 1;

  const latitude = view.getFloat32(offset, true); offset += 4;
  const longitude = view.getFloat32(offset, true); offset += 4;
  const altitude = view.getFloat32(offset, true); offset += 4;
  const hdop = view.getFloat32(offset, true); offset += 4;
  const fixQuality = bytes[offset++];
  const satellites = bytes[offset++];
  const tsLen = bytes[offset++];
  let timestamp = '';
  for (let i = 0; i < tsLen && offset < bytes.length; i++) {
    timestamp += String.fromCharCode(bytes[offset++]);
  }

  return {
    latitude,
    longitude,
    altitude,
    hdop,
    fixQuality,
    satellites,
    timestamp,
  };
}

// ---- Format GPS coordinates for display ----
export function formatCoordinate(lat, lon) {
  const latDir = lat >= 0 ? 'N' : 'S';
  const lonDir = lon >= 0 ? 'E' : 'W';
  return `${Math.abs(lat).toFixed(6)}° ${latDir}, ${Math.abs(lon).toFixed(6)}° ${lonDir}`;
}

// ---- Compute severity level from TDI ----
export function severityFromTDI(tdi) {
  if (tdi < 0.1) return { label: 'Low Risk', color: '#2d8a2d' };
  if (tdi < 0.25) return { label: 'Moderate Risk', color: '#e6c200' };
  if (tdi < 0.5) return { label: 'High Risk', color: '#e65c00' };
  return { label: 'Critical', color: '#cc0000' };
}

// EOF — protocol.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT