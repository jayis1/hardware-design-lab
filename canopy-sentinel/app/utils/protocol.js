// Canopy Sentinel mock protocol helpers
// Author: jayis1

function sampleThermal() {
  return Array.from({ length: 48 }, (_, index) => 16.6 + (index % 8) * 0.35 + ((index / 8) | 0) * 0.12);
}

function sampleSessionData() {
  return {
    live: {
      riskScore: 72.4,
      riskLevel: 'elevated',
      dewMargin: -0.6,
      wetness: 81,
      sporeIndex: 64.2,
      stagnation: 68,
      thermalPreview: sampleThermal()
    },
    history: [
      { id: '1', row: 'GV-01', risk: 'critical', dewMargin: -1.2, wetness: 92, spore: 71.4, note: 'Dense fruit zone; sunrise condensation still present.' },
      { id: '2', row: 'GV-02', risk: 'elevated', dewMargin: -0.4, wetness: 78, spore: 58.0, note: 'North side shade with weak airflow.' },
      { id: '3', row: 'GV-03', risk: 'moderate', dewMargin: 1.1, wetness: 44, spore: 24.9, note: 'Open canopy after leaf thinning.' }
    ],
    device: {
      serial: 'CS-260822-A1',
      battery: 87,
      firmware: '1.0.0',
      crop: 'grape'
    },
    reports: [
      { id: 'r1', title: 'Block A dawn scout', summary: 'Rows 1-4 show clustered mildew pressure near the lower east-facing canopy wall.' },
      { id: 'r2', title: 'Greenhouse bay 3 ventilation audit', summary: 'Persistent stagnation and elevated wetness indicate fan balancing is required.' }
    ],
    settings: {
      cropPreset: 'grape',
      autoSync: true,
      thermalSmoothing: 'medium',
      sporeThreshold: 0.32
    }
  };
}

module.exports = {
  sampleSessionData
};
