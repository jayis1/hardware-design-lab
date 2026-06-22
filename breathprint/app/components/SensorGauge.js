/**
 * SensorGauge.js — Reusable sensor value display component
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const SensorGauge = ({ label, value, unit, color, level }) => {
  return (
    <View style={styles.gauge}>
      <Text style={styles.label}>{label}</Text>
      <View style={styles.valueRow}>
        <Text style={[styles.value, { color }]}>{value}</Text>
        {unit ? <Text style={styles.unit}>{unit}</Text> : null}
      </View>
      {level && (
        <View style={[styles.levelBadge, { backgroundColor: color + '20' }]}>
          <Text style={[styles.levelText, { color }]}>{level}</Text>
        </View>
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  gauge: {
    flex: 1,
    backgroundColor: '#0f0f1e',
    borderRadius: 12,
    padding: 14,
    margin: 4,
    alignItems: 'center',
  },
  label: {
    color: '#888',
    fontSize: 12,
    marginBottom: 6,
  },
  valueRow: {
    flexDirection: 'row',
    alignItems: 'baseline',
  },
  value: {
    fontSize: 22,
    fontWeight: 'bold',
  },
  unit: {
    color: '#666',
    fontSize: 12,
    marginLeft: 4,
  },
  levelBadge: {
    borderRadius: 8,
    paddingHorizontal: 8,
    paddingVertical: 3,
    marginTop: 6,
  },
  levelText: {
    fontSize: 10,
    fontWeight: 'bold',
  },
});

export default SensorGauge;