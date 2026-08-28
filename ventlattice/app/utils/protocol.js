// VentLattice protocol helpers
// Author: jayis1
function sampleVentData() {
  return {
    live: {
      summary: 'Home office airflow has trended downward during afternoon cooling despite sustained occupancy.',
      serviceScore: 58,
      airflowCfm: 41.6,
      supplyTemp: 17.8,
      roomTemp: 25.7,
      staleAirRisk: 0.61,
      comfortWaste: 0.28,
      maintenancePriority: 0.67
    },
    map: [
      { period: '06:00-09:00', cfm: 63, status: 'balanced' },
      { period: '12:00-14:00', cfm: 55, status: 'light' },
      { period: '15:00-18:00', cfm: 39, status: 'restricted' },
      { period: '20:00-22:00', cfm: 47, status: 'light' }
    ],
    rooms: [
      { name: 'Home Office', serviceScore: 58, status: 'under-served' },
      { name: 'Primary Bedroom', serviceScore: 81, status: 'balanced' },
      { name: 'Guest Room', serviceScore: 69, status: 'over-conditioned' },
      { name: 'Hall', serviceScore: 76, status: 'balanced' }
    ],
    alerts: [
      {
        code: 'BLOCKAGE',
        severity: 'warning',
        detail: 'Vent face restriction likely after 15:00. Airflow fell 22% while blower signature remained stable.',
        action: 'Clear rug/chair overlap and verify register vane opening.'
      },
      {
        code: 'STALE_AIR',
        severity: 'warning',
        detail: 'Occupied room freshness degraded faster than learned baseline during active cooling.',
        action: 'Increase branch flow or improve return-air path when office door stays closed.'
      },
      {
        code: 'FILTER_TREND',
        severity: 'info',
        detail: 'Pressure ripple and multi-room flow suggest early filter loading trend.',
        action: 'Inspect central filter within 7 days.'
      }
    ],
    install: {
      magneticCoupling: 'Strong; four tabs engaged',
      nozzleAlignment: 'Centered within grille slot row',
      coverage: '92% register width sampled',
      tip: 'If you rotate the grille louver angle, rerun the 2-minute baseline capture.'
    },
    automation: {
      platform: 'Matter bridge / smart thermostat API',
      scene: 'Office focus mode airflow boost',
      note: 'Bias cooling call extension toward occupied office when whole-home setpoint is already satisfied.',
      smartVentHint: 'Avoid aggressive auto-closing in other rooms until filter trend is cleared.'
    },
    device: {
      name: 'VentLattice Office Node',
      author: 'jayis1',
      firmware: '1.0.0',
      battery: '91% · 58 days estimated',
      mesh: 'Thread ready, BLE active',
      lastSync: '2026-08-28 16:42 local'
    }
  };
}

module.exports = { sampleVentData };
