// PipeWhisper sample protocol helpers
// Author: jayis1
function samplePipeData() {
  return {
    live: {
      branch: 'Laundry hot branch',
      profile: '3/4 in copper clamp mode',
      leakConfidence: 47,
      freezeRisk: 22,
      battery: 86,
      healthIndex: 78,
      installQuality: 91,
      hammerSeverity: 64,
      summary: 'Detected elevated valve chatter during washer fill and a persistent low-flow overnight signature worth inspection.'
    },
    fingerprints: [
      { name: 'Sink / general draw', score: 72, note: 'Baseline branch fingerprint remains dominant during manual faucet use.' },
      { name: 'Washer solenoid', score: 81, note: 'High chatter and ring decay indicate likely appliance valve signature.' },
      { name: 'Small valve / drip', score: 58, note: 'Overnight periodicity suggests intermittent seepage or fill valve drift.' }
    ],
    timeline: [
      { time: '06:12', level: 'info', detail: 'Morning hot draw matched known faucet fingerprint.' },
      { time: '06:38', level: 'warning', detail: 'Hammer severity rose above learned baseline during washer fill cycle.' },
      { time: '23:14', level: 'caution', detail: 'Periodic low-flow acoustic pattern recurred for 26 minutes.' },
      { time: '03:10', level: 'info', detail: 'Surface temperature trend stabilized; freeze risk remained below escalation threshold.' }
    ],
    install: {
      acousticCoupling: 93,
      clampTension: 88,
      orientation: 92,
      note: 'Installation quality is strong. Re-seat only if the device is moved or pipe insulation is changed.'
    },
    report: {
      author: 'jayis1',
      summary: 'PipeWhisper observed branch behavior consistent with normal morning use, pronounced washer fill chatter, and a recurring overnight low-level event pattern. No emergency shutoff recommendation is implied, but targeted inspection is justified.',
      actions: [
        'Inspect washer inlet solenoid and nearby hammer arrestor if fitted.',
        'Check downstream fixture cartridges and supply stop valves for slow seepage.',
        'Retain device in place for three more nights to strengthen confidence interval.'
      ]
    },
    device: {
      name: 'PipeWhisper PW-LH-014',
      author: 'jayis1',
      firmware: '1.0.0-sim',
      lastCalibration: '2026-08-26',
      flashUsage: '31%',
      matterBridge: 'Ready'
    },
    settings: {
      quietHours: '22:00-06:00',
      leakSensitivity: 'balanced',
      freezeSensitivity: 'high',
      automationMode: 'local notify only',
      bleDiagnostics: 'enabled'
    }
  };
}

module.exports = { samplePipeData };
