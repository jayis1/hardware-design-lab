// DrainVeil protocol simulator
// Author: jayis1
function sampleDrainVeilData() {
  return {
    summary: {
      status: 'Grease interceptor branch needs service this week',
      message: 'DrainVeil detected rising fill height, elevated H2S, and longer clear times on the interceptor inlet. Confidence remains high and maintenance is recommended before peak service hours.'
    },
    dashboard: {
      cards: [
        { label: 'Clog Risk', value: '0.81', caption: 'Escalating due to fill-height trend and backpressure growth.' },
        { label: 'Odor Risk', value: '0.67', caption: 'Biofilm and gas signature indicate anaerobic buildup.' },
        { label: 'Freeze Risk', value: '0.14', caption: 'No seasonal exposure concern at this site.' },
        { label: 'Battery', value: '92%', caption: 'Expected 211 days remaining in commercial mode.' }
      ]
    },
    nodes: [
      { name: 'Prep Sink A', risk: 0.42, detail: 'Moderate turbulence but acceptable clear time.' },
      { name: 'Dish Pit Branch', risk: 0.56, detail: 'Trap oscillation suggests vent interaction during peak bursts.' },
      { name: 'Interceptor Inlet', risk: 0.81, detail: 'Slow-clear signature and grease proxy dominate.' },
      { name: 'Mop Sink', risk: 0.18, detail: 'Stable and dry with low odor activity.' }
    ],
    events: [
      { time: '08:12', title: 'Drain lag increased', detail: 'Clear time moved from 24 s to 31 s after morning prep flush.' },
      { time: '11:05', title: 'Odor plume window', detail: 'H2S peaked at 4.6 ppm during idle stagnant interval.' },
      { time: '14:44', title: 'Maintenance opportunity', detail: 'Confidence reached 0.82 with blockage gradient above 0.75.' }
    ],
    chemistry: {
      h2s: '4.6 ppm peak',
      voc: '231 index',
      humidity: '86% RH local chamber',
      biofilm: 'High growth proxy in last 7 days',
      narrative: 'Chemistry data suggests an anaerobic organic film rather than a transient detergent event. The app should recommend flush-and-clean before odor complaints reach occupied spaces.'
    },
    tasks: [
      { title: 'Jet or mechanically clean interceptor inlet branch', effect: 'Largest expected reduction in clog risk and drain time.' },
      { title: 'Inspect vent balance at dish pit branch', effect: 'May reduce trap breathing and gurgle complaints.' },
      { title: 'Replace gas membrane at next quarterly service', effect: 'Restores chemistry channel accuracy margin.' }
    ],
    device: {
      name: 'DrainVeil-IC-014',
      firmware: '1.0.0',
      radio: 'BLE 5.4 / Thread ready',
      confidence: '0.82',
      storage: '16 MB QSPI history buffer',
      author: 'jayis1'
    },
    setup: {
      profile: 'Grease Interceptor',
      pipe: '3 in PVC',
      sync: 'BLE daily + manual technician sync',
      thresholds: 'Commercial conservative',
      notes: 'Mounted on horizontal inlet run, 450 mm upstream of interceptor lid.'
    }
  };
}

module.exports = { sampleDrainVeilData };
