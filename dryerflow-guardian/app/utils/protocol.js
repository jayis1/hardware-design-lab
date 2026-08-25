/**
 * protocol.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */

export const sampleTelemetry = {
  author: 'jayis1',
  device: 'DFG-2608-A01',
  seq: 84,
  run: 4,
  pressure_pa: 4.8,
  flow_cfm: 60.4,
  temp_c: 40.1,
  humidity_rh: 42.7,
  co_ppm: 4.2,
  vri: 73.4,
  ces: 84.8,
  bss: 36.6,
  service_horizon: 4.5,
  alerts: 17
};

export const sampleHistory = [28, 31, 32, 37, 41, 48, 52, 57, 61, 64, 66, 68];
export const sampleHumidity = [62, 66, 71, 76, 73, 67, 61, 56, 50, 46, 42, 39];

export function alertFlagsToList(flags) {
  const alerts = [];
  if (flags & 1) alerts.push('Flow restricted');
  if (flags & 2) alerts.push('Drying slower than baseline');
  if (flags & 4) alerts.push('Backdraft risk detected');
  if (flags & 8) alerts.push('Exhaust overheat condition');
  if (flags & 16) alerts.push('Service soon');
  if (flags & 32) alerts.push('Service now');
  return alerts.length ? alerts : ['No active alerts'];
}

export function healthTone(vri, bss) {
  if (bss > 45 || vri > 80) return { label: 'Critical', color: '#ef4444' };
  if (vri > 62) return { label: 'Service Soon', color: '#f59e0b' };
  return { label: 'Healthy', color: '#22c55e' };
}
