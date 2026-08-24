// SplintSense sample protocol helpers
// Author: jayis1
function sampleSessionData() {
  return {
    live: {
      rsi: 74,
      fitScore: 61,
      battery: 79,
      moisture: 32,
      odorRisk: 48,
      activeAlert: 'Heel pressure concentration persisted for 18 minutes',
      profile: 'Ankle recovery boot',
      hapticMode: 'double-short'
    },
    trends: {
      pressureAsymmetry: [8, 10, 12, 18, 19, 22, 26],
      moistureBurden: [14, 15, 17, 20, 24, 29, 32],
      vocRise: [4, 6, 8, 12, 14, 15, 18],
      impactCounts: [0, 1, 0, 1, 2, 0, 1]
    },
    alerts: [
      { time: '08:40', level: 'warning', code: 'FIT', detail: 'Heel pad pressure remained elevated during rest window.' },
      { time: '09:20', level: 'caution', code: 'MOISTURE', detail: 'Forefoot liner moisture above comfort target for 24 minutes.' },
      { time: '11:05', level: 'critical', code: 'IMPACT', detail: 'Detected impact event above care-plan threshold.' }
    ],
    device: {
      name: 'SplintSense Pod A-014',
      author: 'jayis1',
      firmware: '1.0.0-sim',
      linerProfile: 'Ankle / 8 zone',
      calibrationAgeDays: 4,
      flashUsage: '28%'
    },
    clinician: {
      lastReview: '2026-08-24 11:25',
      recommendedAction: 'Inspect liner fold at heel and reassess boot tightness.',
      exportReady: true,
      note: 'Trend suggests overnight dwell risk rather than transient gait loading.'
    },
    settings: {
      vibrationStrength: 'medium',
      quietHours: '22:00-06:30',
      scanCadence: '4 min adaptive',
      profileMode: 'post-op ankle'
    }
  };
}

module.exports = { sampleSessionData };
