// CordCanary protocol helpers
// Author: jayis1

(function () {
  const scenarios = {
    nominal: {
      label: 'Nominal Lamp Load',
      state: 'NOMINAL',
      risk: 0.08,
      currentA: 0.46,
      hotspotC: 0.7,
      batteryPct: 96,
      humidityPct: 44,
      advisory: 'Connection healthy. Periodic monitoring active.',
      cause: 'No combined thermal, mechanical, or electrical signature exceeds baseline.',
      incidents: [
        {time: '08:10', title: 'Device installed', detail: 'CordCanary clipped securely around lamp cord head.'},
        {time: '10:25', title: 'Routine sample', detail: 'Low load, no wobble, healthy outlet profile.'}
      ]
    },
    heater: {
      label: 'Portable Heater on Aging Outlet',
      state: 'OUTLET_WEAR',
      risk: 0.67,
      currentA: 12.4,
      hotspotC: 5.6,
      batteryPct: 88,
      humidityPct: 38,
      advisory: 'Plug face is heating faster than cord. Reseat or replace outlet.',
      cause: 'Localized blade-side hotspot plus wobble indicates poor receptacle grip or oxidation.',
      incidents: [
        {time: '18:02', title: 'Load rise', detail: 'Current jumped above 12 A when heater switched to high.'},
        {time: '18:07', title: 'Hotspot growth', detail: 'Plug-facing sensor rose 5.6 C above cord-neck channel.'},
        {time: '18:09', title: 'Action suggested', detail: 'Advise user to move heater to a different receptacle.'}
      ]
    },
    garage: {
      label: 'Damp Garage Extension Lead',
      state: 'DAMP_LEAKAGE',
      risk: 0.58,
      currentA: 2.1,
      hotspotC: 1.3,
      batteryPct: 84,
      humidityPct: 81,
      advisory: 'Moisture risk rising. Dry the area and inspect for contamination.',
      cause: 'High humidity, low dew margin, and leakage current suggest surface tracking conditions.',
      incidents: [
        {time: '06:12', title: 'Humidity spike', detail: 'Internal shell humidity crossed 80% after overnight cooling.'},
        {time: '06:18', title: 'Leakage anomaly', detail: 'Leakage proxy increased above expected dry baseline.'}
      ]
    },
    workshop: {
      label: 'Intermittent Arc on Shop Vacuum',
      state: 'ARC_SUSPECT',
      risk: 0.82,
      currentA: 7.5,
      hotspotC: 4.2,
      batteryPct: 79,
      humidityPct: 49,
      advisory: 'Electrical instability suspected. Unplug soon and inspect plug, outlet, and load.',
      cause: 'Burst noise, elevated crest factor, and hotspot rise are consistent with intermittent arcing.',
      incidents: [
        {time: '14:41', title: 'HF burst detected', detail: 'Noise proxy rose sharply during vacuum motor startup.'},
        {time: '14:43', title: 'Arc suspicion', detail: 'Transient density and crest factor exceeded safety threshold.'},
        {time: '14:43', title: 'Urgent alert', detail: 'Companion app escalated to unplug-and-inspect message.'}
      ]
    }
  };

  function scoreTone(risk) {
    if (risk >= 0.75) {
      return 'danger';
    }
    if (risk >= 0.4) {
      return 'warn';
    }
    return 'ok';
  }

  function buildMetrics(snapshot) {
    return [
      {label: 'State', value: snapshot.state},
      {label: 'Risk', value: snapshot.risk.toFixed(2)},
      {label: 'Current', value: snapshot.currentA.toFixed(2) + ' A'},
      {label: 'Hotspot Δ', value: snapshot.hotspotC.toFixed(1) + ' C'},
      {label: 'Humidity', value: snapshot.humidityPct.toFixed(0) + ' %'},
      {label: 'Battery', value: snapshot.batteryPct.toFixed(0) + ' %'}
    ];
  }

  function placementGuide() {
    return [
      'Clip the hinge over the cord neck, not directly over plug blades.',
      'Align the narrow sensor window toward the outlet face.',
      'Do not clamp over exposed strain-relief seams or damaged insulation.',
      'If the app shows poor clip security, rotate the device 90° and retry.'
    ];
  }

  window.CordCanaryProtocol = {
    scenarios,
    scoreTone,
    buildMetrics,
    placementGuide
  };
})();
