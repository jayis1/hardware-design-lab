// utils/calibration.js — HydroFluor calibration workflow helpers
// Author: jayis1  License: MIT
//
// Guides the user through a blank + single-point calibration for each
// analyte. Returns reference values that are pushed to the device via
// the BLE calibrate characteristic.

export const CALIBRATION_STANDARDS = {
  cdom: {
    label: 'CDOM (Quinine Sulfate)',
    blank:  { label: '0.1 N H₂SO₄',              value: 0 },
    points: [
      { label: '10 ppb QSU',   value: 10 },
      { label: '100 ppb QSU', value: 100 },
      { label: '500 ppb QSU', value: 500 },
    ],
  },
  chla: {
    label: 'Chlorophyll-a extract',
    blank:  { label: '90% acetone blank', value: 0 },
    points: [
      { label: '10 µg/L',  value: 10 },
      { label: '100 µg/L', value: 100 },
    ],
  },
  phyc: {
    label: 'Phycocyanin extract',
    blank:  { label: 'Phosphate buffer', value: 0 },
    points: [
      { label: '5 µg/L',  value: 5 },
      { label: '50 µg/L', value: 50 },
    ],
  },
  oil: {
    label: 'Crude oil dispersion',
    blank:  { label: 'Deionized water', value: 0 },
    points: [
      { label: '1 ppb',  value: 1 },
      { label: '10 ppb', value: 10 },
    ],
  },
  turb: {
    label: 'Formazin turbidity',
    blank:  { label: 'Filtered DI water', value: 0 },
    points: [
      { label: '20 NTU',  value: 20 },
      { label: '100 NTU', value: 100 },
    ],
  },
};

// Analyte IDs must match firmware enum analyte_t
export const ANALYTE_IDS = {
  cdom: 0, chla: 1, phyc: 2, oil: 3, turb: 4,
};

// Returns the ordered steps for a calibration workflow.
export function buildWorkflow(analyte) {
  const std = CALIBRATION_STANDARDS[analyte];
  if (!std) return [];
  const steps = [
    { phase: 'blank',  label: std.blank.label,  refValue: std.blank.value },
    ...std.points.map(p => ({ phase: 'point', label: p.label, refValue: p.value })),
    { phase: 'done',   label: 'Calibration complete', refValue: 0 },
  ];
  return steps;
}