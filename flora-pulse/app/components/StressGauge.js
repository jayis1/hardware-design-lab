/**
 * StressGauge.js — Plant Stress Level Gauge
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays a visual stress level indicator based on AP firing rate
 * and VPD (vapor pressure deficit).
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StressGauge({ stressLevel, apRate, vpd }) {
  const color = stressLevel === 'HIGH' ? '#f44336' :
                stressLevel === 'NORMAL' ? '#4CAF50' : '#888';

  const vpdColor = vpd > 1.6 ? '#f44336' :
                   vpd > 1.2 ? '#ff9800' :
                   vpd > 0.4 ? '#4CAF50' : '#2196F3';

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Plant Stress Level</Text>
      <View style={[styles.gauge, { borderColor: color }]}>
        <Text style={[styles.gaugeValue, { color }]}>{stressLevel}</Text>
      </View>
      <View style={styles.metrics}>
        <View style={styles.metric}>
          <Text style={styles.metricLabel}>AP Rate</Text>
          <Text style={styles.metricValue}>{apRate.toFixed(1)} Hz</Text>
        </View>
        <View style={styles.metric}>
          <Text style={styles.metricLabel}>VPD</Text>
          <Text style={[styles.metricValue, { color: vpdColor }]}>{vpd.toFixed(2)} kPa</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1a2e1a',
    borderRadius: 12,
    padding: 20,
    marginBottom: 12,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  title: {
    fontSize: 14,
    color: '#888',
    marginBottom: 12,
  },
  gauge: {
    width: 200,
    height: 80,
    borderRadius: 40,
    borderWidth: 3,
    alignItems: 'center',
    justifyContent: 'center',
    marginBottom: 16,
  },
  gaugeValue: {
    fontSize: 28,
    fontWeight: 'bold',
  },
  metrics: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    width: '100%',
  },
  metric: {
    alignItems: 'center',
  },
  metricLabel: {
    fontSize: 12,
    color: '#888',
    marginBottom: 4,
  },
  metricValue: {
    fontSize: 18,
    fontWeight: '600',
    color: '#fff',
  },
});