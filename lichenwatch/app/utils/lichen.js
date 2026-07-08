/*
 * lichen.js — Lichen biology interpretation helpers.
 *
 * Author: jayis1
 * License: MIT
 *
 * Provides human-readable interpretations of the sensor readings, useful
 * for the dashboard and field notes.
 */

export const STATE_NAMES = {
  0: 'Healthy',
  1: 'Stressed',
  2: 'Bleaching',
  3: 'Recovering',
  4: 'Dormant',
};

export const STATE_DESCRIPTIONS = {
  0: 'Colony is photosynthesizing normally. Quantum yield is in the healthy range (>0.65) and spectral indices are stable.',
  1: 'Colony is under stress. Quantum yield has dropped and UV-screening pigments may be increasing. Investigate local air quality or microclimate.',
  2: 'Colony is bleaching. Chlorophyll is degrading and the thallus is losing its photosynthetic capacity. Immediate site investigation recommended.',
  3: 'Colony is recovering from a prior stress event. Yield is rising and wetness is sufficient for metabolic activity.',
  4: 'Colony is dormant — desiccated or frozen. This is a normal seasonal state for many lichens; metabolic activity will resume when wetted.',
};

export const STATE_COLORS = {
  0: '#2e7d32',
  1: '#f9a825',
  2: '#c62828',
  3: '#1565c0',
  4: '#6a1b9a',
};

/* Interpret Fv/Fm (PSII quantum yield) into a qualitative verdict. */
export function interpretYield(fvFm) {
  if (fvFm < 0) return { label: 'No signal', color: '#9e9e9e' };
  if (fvFm >= 0.65) return { label: 'Optimal', color: '#2e7d32' };
  if (fvFm >= 0.45) return { label: 'Reduced', color: '#f9a825' };
  return { label: 'Severely depressed', color: '#c62828' };
}

/* Interpret LNDVI (lichen NDVI analog). */
export function interpretLndvi(lndvi) {
  if (lndvi > 0.3) return { label: 'Vigorous', color: '#2e7d32' };
  if (lndvi > 0.1) return { label: 'Moderate', color: '#f9a825' };
  return { label: 'Degraded', color: '#c62828' };
}

/* Interpret surface wetness. */
export function interpretWetness(w) {
  if (w > 0.7) return { label: 'Wet — metabolically active', color: '#1565c0' };
  if (w > 0.3) return { label: 'Damp', color: '#42a5f5' };
  return { label: 'Dry — dormant', color: '#8d6e63' };
}

/* Interpret acoustic click count (desiccation events). */
export function interpretClicks(count) {
  if (count === 0) return { label: 'No fragmentation', color: '#2e7d32' };
  if (count <= 10) return { label: 'Minor cracking', color: '#f9a825' };
  return { label: 'Active desiccation damage', color: '#c62828' };
}

/* Battery state-of-charge from voltage. */
export function interpretBattery(mv) {
  if (mv >= 3500) return { label: 'Good', color: '#2e7d32', pct: 100 };
  if (mv >= 3400) return { label: 'Fair', color: '#f9a825', pct: 70 };
  if (mv >= 3300) return { label: 'Low', color: '#ef6c00', pct: 40 };
  return { label: 'Critical', color: '#c62828', pct: 15 };
}

/* Format uptime in seconds to a human-readable duration. */
export function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  if (days > 0) return `${days}d ${hours}h ${mins}m`;
  if (hours > 0) return `${hours}h ${mins}m`;
  return `${mins}m`;
}