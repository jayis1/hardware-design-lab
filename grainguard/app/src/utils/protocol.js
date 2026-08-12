/**
 * @file    protocol.js
 * @brief   Binary mesh packet parser and grain type lookup.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 *
 * Parses the 18-byte GrainGuard mesh packet (as received from the gateway
 * in JSON form) into a human-readable data structure used by the UI.
 */

// Grain type constants (must match firmware board.h)
export const GRAIN_TYPES = {
  1: { name: 'Wheat',   safeMc: 13.5 },
  2: { name: 'Corn',    safeMc: 15.5 },
  3: { name: 'Barley',  safeMc: 14.0 },
  4: { name: 'Rice',    safeMc: 13.0 },
  5: { name: 'Oats',    safeMc: 14.0 },
  6: { name: 'Soybean', safeMc: 13.0 },
};

export const INSECT_SPECIES = {
  0:   'None detected',
  1:   'Sitophilus granarius (Granary Weevil)',
  2:   'Tribolium castaneum (Red Flour Beetle)',
  3:   'Rhyzopertha dominica (Lesser Grain Borer)',
  254: 'Unknown species',
};

/**
 * Parse a raw mesh packet (from gateway JSON) into a structured object.
 * @param {Object} raw - raw packet fields from gateway
 * @returns {Object} parsed data
 */
export function parseMeshPacket(raw) {
  const grainType = raw.grainType || 1;
  const grainInfo = GRAIN_TYPES[grainType] || { name: 'Unknown', safeMc: 14.0 };

  // Decode temperature (offset +128 to fit uint8)
  const tmaxC = raw.tmaxOffset !== undefined ? raw.tmaxOffset - 128 : 0;
  const tminC = raw.tminOffset !== undefined ? raw.tminOffset - 128 : 0;
  const deltaTC = raw.deltaT || (tmaxC - tminC);

  // Build 9-zone temperature profile (only max/min are in the mesh packet;
  // full 9-zone data comes via the REST API historical endpoint)
  const tempZones = Array.from({ length: 9 }, (_, i) => ({
    zone: i,
    tempC: 20 + (i * (deltaTC / 8)),  // interpolated; replaced by historical data
    valid: true,
  }));

  // EMC: stored as ×10 (e.g. 135 = 13.5%)
  const emcPct = raw.emcX10 ? raw.emcX10 / 10 : 0;

  // CO2: stored as ÷10
  const co2Ppm = raw.co2PpmX10 ? raw.co2PpmX10 * 10 : 0;

  // Battery percentage estimate (LiSOCl2: 3600 mV full, 3000 mV crit)
  const batteryMv = raw.batteryMv || 0;
  const batteryPct = batteryMv
    ? Math.round(Math.max(0, Math.min(100,
        ((batteryMv - 3000) / (3600 - 3000)) * 100)))
    : 0;

  // Alert level
  const criticalThreshold = raw.criticalThreshold || 70;
  const cautionThreshold = raw.cautionThreshold || 40;
  let alertLevel = 0;
  if (raw.sri >= criticalThreshold) alertLevel = 2;
  else if (raw.sri >= cautionThreshold) alertLevel = 1;

  return {
    serial: raw.serial,
    sri: raw.sri || 0,
    alertLevel,
    co2Ppm,
    tmaxC,
    tminC,
    deltaTC,
    tmaxZone: raw.tmaxZone || 0,
    tminZone: raw.tminZone || 0,
    rhPct: raw.rhPct || 0,
    emcPct,
    safeMcPct: grainInfo.safeMc,
    grainName: grainInfo.name,
    aeEventsPerMin: raw.aeEventsPerMin || 0,
    insectId: raw.insectId || 0,
    insectName: INSECT_SPECIES[raw.insectId || 0] || 'Unknown',
    aeConfidence: raw.aeConfidence,
    aePeakMv: raw.aePeakMv,
    aeAvgDurMs: raw.aeAvgDurMs,
    batteryMv,
    batteryPct: `${batteryPct}%`,
    tempZones,
    co2Contribution: raw.co2Contribution,
    tempGradContribution: raw.tempGradContribution,
    tempAbsContribution: raw.tempAbsContribution,
    emcContribution: raw.emcContribution,
    acousticContribution: raw.acousticContribution,
    timestamp: raw.timestamp_min,
    hopCount: raw.hopCount,
  };
}

/**
 * Pack a configuration command to send to a probe via the gateway.
 */
export function packConfig(grainType, cautionThresh, criticalThresh,
                            measIntervalMin, acousticIntervalMin) {
  return {
    type: 'configure_probe',
    payload: {
      grainType,
      cautionThreshold: cautionThresh,
      criticalThreshold: criticalThresh,
      measIntervalMin,
      acousticIntervalMin,
    },
  };
}

/**
 * Compute an estimated SRI from raw sensor values (for display when
 * the probe hasn't reported an SRI yet).
 */
export function estimateSRI(co2, deltaT, tmax, emc, safeMc, aeEvents) {
  let sri = 0;

  // CO2 (max 35)
  if (co2 > 2000) sri += 28 + Math.min(7, (co2 - 2000) * 7 / 3000);
  else if (co2 > 1000) sri += 15 + (co2 - 1000) * 13 / 1000;
  else if (co2 > 600) sri += (co2 - 600) * 15 / 400;

  // Temp gradient (max 25)
  if (deltaT > 5) sri += Math.min(25, (deltaT - 5) * 25 / 45);

  // Temp absolute (max 15)
  if (tmax > 15) sri += Math.min(15, (tmax - 15) * 15 / 15);

  // EMC (max 15)
  if (emc > safeMc) sri += Math.min(15, (emc - safeMc) * 15 / 2);

  // Acoustic (max 10)
  if (aeEvents > 0) sri += Math.min(10, aeEvents * 10 / 50);

  return Math.round(Math.min(100, Math.max(0, sri)));
}

// Author: jayis1