/**
 * Gauge.js — Animated gauge component for power quality metrics
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function Gauge({ label, value, unit, min, max, format, color }) {
  const safeValue = Math.max(min, Math.min(max, value));
  const ratio = (safeValue - min) / (max - min);
  const fillWidth = `${Math.max(0, Math.min(100, ratio * 100))}%`;

  // Color based on value position
  let barColor = color || '#00CC44';
  if (!color) {
    if (ratio > 0.85) barColor = '#FF3333';
    else if (ratio > 0.65) barColor = '#FFAA00';
    else barColor = '#00CC44';
  }

  const displayValue = format ? format(value) : value.toFixed(1);

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.label}>{label}</Text>
        <Text style={styles.value}>{displayValue} <Text style={styles.unit}>{unit}</Text></Text>
      </View>
      <View style={styles.barBg}>
        <View style={[styles.barFill, { width: fillWidth, backgroundColor: barColor }]} />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { marginVertical: 6, paddingHorizontal: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  label: { fontSize: 13, color: '#888', fontWeight: '500' },
  value: { fontSize: 16, color: '#FFF', fontWeight: '700' },
  unit: { fontSize: 12, color: '#888', fontWeight: '400' },
  barBg: { height: 10, backgroundColor: '#222', borderRadius: 5, overflow: 'hidden' },
  barFill: { height: '100%', borderRadius: 5 },
});