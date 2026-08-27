// SealBeat protocol mock data
// Author: jayis1
function sampleSealBeatData() {
  return {
    summary: {
      status: 'Top-edge seal drift detected',
      message: 'Closure quality is still acceptable, but the top-right zone is recovering 19% slower than the baseline. Clean the gasket and reduce upper-bin load before scheduling hinge service.'
    },
    dashboard: {
      sealScore: 72,
      safetyScore: 88,
      hingeWear: 41,
      battery: 91,
      lastRecovery: 61,
      nightOpenings: 3
    },
    sealMap: {
      top: 0.58,
      latch: 0.81,
      bottom: 0.69,
      hinge: 0.74,
      leakSide: 'top-right',
      note: 'Magnetic pull remains strong on the latch edge, but compression uniformity falls off near the upper corner.'
    },
    cycles: {
      totalToday: 37,
      averageOpenSeconds: 11,
      longestOpenSeconds: 42,
      pattern: [6, 4, 2, 1, 3, 5, 8, 10, 7, 9, 6, 3],
      insight: 'Morning clusters are normal, but three bounce-back closures occurred during late-evening use.'
    },
    recovery: {
      warmReboundC: 1.6,
      tauSeconds: 61,
      edgeTempC: 5.9,
      compartmentTempC: 4.8,
      foodSafe: true,
      commentary: 'Recovery remains inside the pharmacy-safe profile, but margin is shrinking after repeated top-shelf loading.'
    },
    tasks: [
      { priority: 'High', title: 'Clean and inspect top gasket edge', detail: 'Wipe oils and debris, then warm-form the top section with controlled heat to restore compression memory.' },
      { priority: 'Medium', title: 'Reduce top-door bin mass', detail: 'Door sag and bounce rise when the upper bin is fully loaded with glass containers.' },
      { priority: 'Medium', title: 'Re-level hinge side', detail: 'Measured hinge skew has risen from 0.19 to 0.28 over the last seven days.' }
    ],
    device: {
      profile: 'Pharmacy Cooler',
      firmware: '0.1.0',
      connectivity: 'BLE + Thread ready',
      batteryDays: 214,
      installQuality: 'Excellent',
      author: 'jayis1'
    },
    setup: {
      placement: 'Mount 30 mm below the top corner on the hinge-side frame rail. Route the compression strip across the top seal landing zone.',
      calibration: 'Perform three normal closes and one intentional gentle close. SealBeat stores magnetic and compression baselines automatically.',
      export: 'Technician mode can export a 14-day closure digest as JSON or PDF summary through the paired app.'
    }
  };
}

module.exports = {
  sampleSealBeatData
};
