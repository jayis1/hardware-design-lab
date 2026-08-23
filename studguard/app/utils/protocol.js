/*
 * protocol.js — StudGuard telemetry helpers
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

export const author = 'jayis1';

export const sampleNodes = [
  {
    id: 'SG-03',
    zone: 'Unit 4B shower wall',
    leakActivity: 0.81,
    wetnessSpread: 0.63,
    confidence: 0.77,
    battery: 86,
    originBand: 1.02,
    event: 'active-pressure-leak',
    segments: [0.78, 0.52, 0.44, 0.39],
    spark: [0.32, 0.35, 0.41, 0.48, 0.56, 0.65, 0.73, 0.81]
  },
  {
    id: 'SG-04',
    zone: 'Unit 4B vanity chase',
    leakActivity: 0.56,
    wetnessSpread: 0.41,
    confidence: 0.69,
    battery: 91,
    originBand: 0.84,
    event: 'intermittent-leak',
    segments: [0.59, 0.48, 0.35, 0.31],
    spark: [0.21, 0.24, 0.31, 0.29, 0.37, 0.43, 0.49, 0.56]
  },
  {
    id: 'SG-07',
    zone: 'Mechanical riser west',
    leakActivity: 0.22,
    wetnessSpread: 0.18,
    confidence: 0.82,
    battery: 73,
    originBand: 0.36,
    event: 'post-repair-drying',
    segments: [0.28, 0.25, 0.21, 0.22],
    spark: [0.62, 0.57, 0.49, 0.45, 0.38, 0.31, 0.27, 0.22]
  }
];

export function riskBand(value) {
  if (value >= 0.75) return { label: 'Critical', color: '#dc2626' };
  if (value >= 0.50) return { label: 'Elevated', color: '#f97316' };
  if (value >= 0.25) return { label: 'Watch', color: '#eab308' };
  return { label: 'Stable', color: '#16a34a' };
}

export function summaryStats(nodes) {
  const critical = nodes.filter((n) => n.leakActivity >= 0.75).length;
  const averageLeak = nodes.reduce((sum, n) => sum + n.leakActivity, 0) / nodes.length;
  const lowestBattery = Math.min(...nodes.map((n) => n.battery));
  return {
    author,
    critical,
    averageLeak: Number(averageLeak.toFixed(2)),
    lowestBattery
  };
}
