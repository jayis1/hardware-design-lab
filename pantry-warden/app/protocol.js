// Pantry Warden protocol helpers
// Author: jayis1

export function deriveAction(frame) {
  if (frame.state === 'CRITICAL_INTERVENE') {
    return 'Isolate suspect food, sanitize shelf, and inspect the front-right cluster now.';
  }
  if (frame.state === 'PEST_WATCH') {
    return 'Inspect flour and cereal packages, add a trap, and enable a longer night sweep.';
  }
  if (frame.state === 'SPOILAGE_SUSPECT') {
    return 'Check for swelling, leakage, or odor changes and rotate that zone first.';
  }
  if (frame.state === 'CONDENSATION_WATCH') {
    return 'Dry the rear wall, separate dense stacks, and improve local airflow.';
  }
  if (frame.state === 'RESTOCKED') {
    return 'Restock logged; verify oldest items still sit at the front.';
  }
  return 'Shelf stable. Continue normal rotation and weekly wipe-down.';
}

export function classifyZones(frame) {
  const left = {
    name: 'Left zone',
    mass: (frame.mass * 0.52).toFixed(2),
    freshness: Math.max(30, frame.health - frame.moist * 0.2).toFixed(1),
    severity: frame.moist > 45 ? 'warn' : 'normal'
  };
  const center = {
    name: 'Center zone',
    mass: (frame.mass * 0.18).toFixed(2),
    freshness: Math.max(30, frame.health - frame.voc * 0.4).toFixed(1),
    severity: frame.voc > 35 ? 'warn' : 'normal'
  };
  const right = {
    name: 'Right zone',
    mass: (frame.mass * 0.30).toFixed(2),
    freshness: Math.max(20, frame.health - frame.wing * 0.35 - frame.chew * 0.25).toFixed(1),
    severity: frame.state === 'CRITICAL_INTERVENE' || frame.state === 'PEST_WATCH' ? 'alert' : 'normal'
  };
  return [left, center, right];
}

export function telemetryInterpretation(frame) {
  return [
    `VOC index ${frame.voc.toFixed(1)} suggests ${frame.voc > 30 ? 'a meaningful odor rise' : 'normal shelf chemistry'}.`,
    `Moisture strip at ${frame.moist.toFixed(1)}% indicates ${frame.moist > 40 ? 'rear-shelf moisture retention' : 'dry rear-shelf conditions'}.`,
    `Wingbeat ${frame.wing.toFixed(1)} and chew ${frame.chew.toFixed(1)} imply ${frame.wing > 50 || frame.chew > 45 ? 'possible pest activity' : 'no strong pest evidence'}.`,
    `Shelf mass ${frame.mass.toFixed(2)} kg with gap ${frame.gap.toFixed(1)} mm indicates ${frame.gap < 58 ? 'dense front packaging or bulge growth' : 'comfortable front clearance'}.`
  ];
}

export function buildFrame(overrides = {}) {
  const frame = {
    tick: 0,
    mode: 'AUTO',
    state: 'STABLE',
    temp: 22.6,
    rh: 50.4,
    co2: 526.0,
    voc: 18.0,
    mass: 6.4,
    gap: 67.2,
    moist: 18.0,
    wing: 10.0,
    chew: 8.0,
    bat: 94.0,
    health: 91.0,
    ...overrides
  };

  if (frame.voc > 38 || frame.gap < 56) {
    frame.state = 'SPOILAGE_SUSPECT';
    frame.health = Math.min(frame.health, 58);
  }
  if (frame.moist > 48) {
    frame.state = 'CONDENSATION_WATCH';
    frame.health = Math.min(frame.health, 63);
  }
  if (frame.wing > 52 || frame.chew > 48) {
    frame.state = 'PEST_WATCH';
    frame.health = Math.min(frame.health, 52);
  }
  if ((frame.voc > 46 && frame.gap < 55) || (frame.wing > 62 && frame.moist > 52)) {
    frame.state = 'CRITICAL_INTERVENE';
    frame.health = Math.min(frame.health, 34);
  }
  return frame;
}
