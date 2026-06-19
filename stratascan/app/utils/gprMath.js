// gprMath.js — GPR Math Helpers (depth calculation, ε_r estimation)
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

export const SPEED_OF_LIGHT = 299792458;  // m/s

// Compute subsurface velocity from relative permittivity
export function subsurfaceVelocity(epsR) {
  return SPEED_OF_LIGHT / Math.sqrt(epsR);
}

// Compute range resolution: δR = c / (2 × B × √ε_r)
export function rangeResolution(bandwidthHz, epsR) {
  return SPEED_OF_LIGHT / (2 * bandwidthHz * Math.sqrt(epsR));
}

// Compute unambiguous range: R_max = c × (N-1) / (2 × Δf × √ε_r)
export function unambiguousRange(numSteps, stepSizeHz, epsR) {
  return (SPEED_OF_LIGHT * (numSteps - 1)) / (2 * stepSizeHz * Math.sqrt(epsR));
}

// Convert depth bins to depth in meters
export function depthAxis(numBins, bandwidthHz, epsR) {
  const dR = rangeResolution(bandwidthHz, epsR);
  const depths = [];
  for (let i = 0; i < numBins; i++) {
    depths.push(i * dR);
  }
  return depths;
}

// Estimate relative permittivity from a known-depth reflection
// Given: measured two-way time t (ns), known depth d (m)
// ε_r = (c × t / (2 × d))²
export function estimateEpsR(twoWayTimeNs, knownDepthM) {
  const t_s = twoWayTimeNs * 1e-9;
  if (t_s <= 0 || knownDepthM <= 0) return 6.0;  // default
  const epsR = Math.pow((SPEED_OF_LIGHT * t_s) / (2 * knownDepthM), 2);
  return Math.max(1, Math.min(81, epsR));
}

// Common material permittivity reference table
export const MATERIAL_EPS_R = {
  'Air':           1.0,
  'Fresh water':   81,
  'Sea water':     80,
  'Ice':           3.4,
  'Dry sand':      4.0,
  'Wet sand':      25,
  'Dry soil':      3.5,
  'Moist soil':    12,
  'Wet soil':      30,
  'Clay (dry)':    4,
  'Clay (wet)':    20,
  'Limestone':     7,
  'Granite':       5,
  'Concrete':      6.5,
  'Asphalt':       5.5,
  'PVC':           3.0,
};

// Auto-detect peaks in an A-scan trace (for layer/interface detection)
export function detectPeaks(trace, threshold = 0.3) {
  if (!trace || trace.length === 0) return [];
  const maxVal = Math.max(...trace.map(v => Math.abs(v)));
  if (maxVal < 1e-9) return [];
  const peaks = [];
  for (let i = 1; i < trace.length - 1; i++) {
    const v = Math.abs(trace[i]);
    if (v > threshold * maxVal && v >= Math.abs(trace[i - 1]) && v >= Math.abs(trace[i + 1])) {
      peaks.push({ index: i, value: trace[i], amplitude: v / maxVal });
    }
  }
  // Sort by amplitude descending and return top peaks
  peaks.sort((a, b) => b.amplitude - a.amplitude);
  return peaks;
}

// Convert peak depth (m) to two-way travel time (ns)
export function depthToTime(depthM, epsR) {
  const v = subsurfaceVelocity(epsR);
  return (2 * depthM / v) * 1e9;  // ns
}

// Format depth for display
export function formatDepth(depthM) {
  if (depthM < 0.1) return `${(depthM * 100).toFixed(1)} cm`;
  if (depthM < 1.0) return `${(depthM * 100).toFixed(0)} cm`;
  return `${depthM.toFixed(2)} m`;
}

// Format distance for display
export function formatDistance(m) {
  if (m < 1) return `${(m * 100).toFixed(0)} cm`;
  if (m < 1000) return `${m.toFixed(1)} m`;
  return `${(m / 1000).toFixed(2)} km`;
}