// Threshold Veil protocol helpers
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

const BASE_EVENTS = [
  {time: '18:42', state: 'ODOR_PUSH', confidence: 0.82, detail: 'Cooking odor migrated from corridor. Latch-side seal inflated.'},
  {time: '19:10', state: 'QUIET_HOURS', confidence: 0.77, detail: 'Speech leakage rose after quiet-hours window. Acoustic profile tightened.'},
  {time: '21:03', state: 'PRESSURE_SURGE', confidence: 0.75, detail: 'Stairwell pressure pulse detected. Threshold chambers preloaded.'},
  {time: '22:14', state: 'CALM', confidence: 0.91, detail: 'Boundary stabilized after corridor traffic slowed.'},
];

export function buildMockSnapshot(mode) {
  const map = {
    AUTO: {state: 'CALM', ingressScore: 0.64, sealPressure: 3.1, batteryPct: 91, pm25: 9.4, vocDelta: 8.2},
    QUIET: {state: 'QUIET_HOURS', ingressScore: 1.18, sealPressure: 5.4, batteryPct: 89, pm25: 8.1, vocDelta: 12.6},
    SHELTER: {state: 'SHELTER', ingressScore: 2.92, sealPressure: 8.9, batteryPct: 84, pm25: 41.2, vocDelta: 28.7},
    OPEN_FLOW: {state: 'DOOR_OPEN', ingressScore: 0.35, sealPressure: 0.8, batteryPct: 93, pm25: 7.8, vocDelta: 4.4},
  };
  const current = map[mode] || map.AUTO;
  return {
    ...current,
    mode,
    pressurePa: mode === 'SHELTER' ? 3.6 : 0.8,
    sealHealthPct: 94,
    recommendation:
      mode === 'SHELTER'
        ? 'Keep the door shut and reduce corridor exposure until particulate levels fall.'
        : mode === 'QUIET'
        ? 'Quiet-hours optimization active. Speech-band leakage is being attenuated.'
        : 'Boundary stable. Threshold Veil is comparing indoor and corridor gradients.',
    acousticBands: [33, 38, 29],
    leakByHour: [0.3, 0.4, 0.5, 1.0, 1.1, 1.4, 1.2, 0.8],
    installationChecks: [
      {label: 'Top jamb alignment', status: 'Pass'},
      {label: 'Threshold strip compression', status: 'Pass'},
      {label: 'Sample inlet lint filter', status: 'Clean'},
      {label: 'Battery service level', status: 'Good'},
    ],
    events: BASE_EVENTS,
  };
}

export function summarizeTimeline(events) {
  const active = events.filter(item => item.state !== 'CALM').length;
  return `${events.length} recent events, ${active} active interventions, latest state ${events[events.length - 1].state}.`;
}
