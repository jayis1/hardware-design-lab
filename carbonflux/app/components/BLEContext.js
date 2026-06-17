/**
 * BLEContext.js — React Context for BLE state and device data.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';

const BLEContext = React.createContext({
  bleConnected: false,
  setBleConnected: () => {},
  deviceData: {
    state: 0,
    co2_ppm: 0,
    air_temp_c: 0,
    flux_umol: 0,
    battery_soc: 0,
    measurement_count: 0,
    pressure_hpa: 1013.25,
    soil_temp_5cm: -127,
    soil_temp_15cm: -127,
    soil_temp_30cm: -127,
    vwc_pct: 0,
    par_umol: 0,
  },
  setDeviceData: () => {},
});

export default BLEContext;