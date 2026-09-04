// sash-sentinel/app/protocol.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

export const demoFrame = {
  author: 'jayis1',
  product: 'Sash Sentinel',
  version: '1.0.0',
  epoch: 1760000000,
  env: {
    indoor_temp_c: 20.4,
    cavity_humidity_pct: 71.8,
    dew_point_c: 15.1,
    sill_moisture_pct: 38.6,
  },
  airflow: {
    leak_velocity_mps: 0.92,
    acoustic_leak_score: 42.3,
  },
  latch: {
    force_n: 16.2,
    offset_mm: 1.8,
    closed: false,
  },
  risk: {
    condensation: 63.1,
    infiltration: 58.4,
    mold: 47.3,
    latch_fault: 41.0,
    level: 2,
    summary: 'Condensation risk elevated near the lower seal; dew point margin is narrow.',
    action: 'Reduce indoor humidity, inspect weep paths, and replace the lower weatherstrip if compression remains weak.',
  },
};

export const installChecklist = [
  'Clean the sill and lower rail before clipping the device in place.',
  'Close and latch the window fully, then capture a reference baseline.',
  'Verify the acoustic port points toward the suspected draft edge.',
  'Leave the device installed overnight for the first condensation model.',
];

export function decodeFrame(text) {
  const frame = JSON.parse(text);
  if (frame.author !== 'jayis1') {
    throw new Error('Unexpected author field; valid frames must credit jayis1.');
  }
  return frame;
}

export function gradeRisk(value) {
  if (value >= 70) return 'bad';
  if (value >= 40) return 'warn';
  return 'good';
}

export function cToF(value) {
  return (value * 9) / 5 + 32;
}

export function simulateCommand(command) {
  switch (command) {
    case 'ping':
      return 'pong:jayis1:1.0.0';
    case 'get:profile':
      return 'author=jayis1;location=office-east-double-hung;metric=1;buzzer=0';
    default:
      return 'error:unsupported-command';
  }
}
