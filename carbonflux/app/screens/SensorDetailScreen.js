/**
 * SensorDetailScreen.js — All raw sensor values in real time.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {useContext} from 'react';
import {View, ScrollView, Text, StyleSheet} from 'react-native';
import BLEContext from '../components/BLEContext';

const SensorRow = ({label, value, unit}) => (
  <View style={styles.row}>
    <Text style={styles.label}>{label}</Text>
    <View style={styles.valueContainer}>
      <Text style={styles.value}>{value}</Text>
      <Text style={styles.unit}>{unit}</Text>
    </View>
  </View>
);

const SensorDetailScreen = () => {
  const {deviceData} = useContext(BLEContext);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Atmospheric</Text>
      <SensorRow label="CO₂ Concentration" value={deviceData.co2_ppm.toFixed(0)} unit="ppm" />
      <SensorRow label="Air Temperature" value={deviceData.air_temp_c.toFixed(1)} unit="°C" />
      <SensorRow label="Barometric Pressure" value={deviceData.pressure_hpa.toFixed(1)} unit="hPa" />

      <Text style={styles.sectionTitle}>Soil Profile</Text>
      <SensorRow label="Soil Temp @5 cm" value={deviceData.soil_temp_5cm.toFixed(1)} unit="°C" />
      <SensorRow label="Soil Temp @15 cm" value={deviceData.soil_temp_15cm.toFixed(1)} unit="°C" />
      <SensorRow label="Soil Temp @30 cm" value={deviceData.soil_temp_30cm.toFixed(1)} unit="°C" />
      <SensorRow label="Volumetric Water" value={deviceData.vwc_pct.toFixed(1)} unit="%" />

      <Text style={styles.sectionTitle}>Radiation</Text>
      <SensorRow label="PAR" value={deviceData.par_umol.toFixed(0)} unit="µmol/m²/s" />

      <Text style={styles.sectionTitle}>Power</Text>
      <SensorRow label="Battery SOC" value={`${deviceData.battery_soc}`} unit="%" />
      <SensorRow label="State" value={`${deviceData.state}`} unit="" />

      <Text style={styles.sectionTitle}>Flux Summary</Text>
      <SensorRow label="Soil CO₂ Efflux" value={deviceData.flux_umol.toFixed(3)} unit="µmol/m²/s" />
      <SensorRow label="Total Measurements" value={`${deviceData.measurement_count}`} unit="" />

      {/* Export data hint */}
      <View style={styles.hintBox}>
        <Text style={styles.hintText}>
          Real-time values shown when connected via BLE or USB. Go to the Export tab to download
          historical data as CSV.
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 8,
  },
  sectionTitle: {
    color: '#4CAF50',
    fontSize: 14,
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 1.5,
    marginTop: 16,
    marginBottom: 8,
    marginLeft: 4,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginVertical: 2,
    borderWidth: 1,
    borderColor: '#252540',
  },
  label: {
    color: '#aaa',
    fontSize: 14,
    flex: 1,
  },
  valueContainer: {
    flexDirection: 'row',
    alignItems: 'baseline',
  },
  value: {
    color: '#e0e0e0',
    fontSize: 16,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  unit: {
    color: '#666',
    fontSize: 12,
    marginLeft: 4,
  },
  hintBox: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    marginTop: 20,
    borderWidth: 1,
    borderColor: '#333',
  },
  hintText: {
    color: '#888',
    fontSize: 12,
    lineHeight: 18,
  },
});

export default SensorDetailScreen;