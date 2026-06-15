/**
 * components/ImuGauge.js — Reusable IMU value gauge
 *
 * Displays a single-axis sensor reading with a color-coded
 * bar indicator and numeric value.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

function valueToColor(value, range) {
  const ratio = Math.abs(value) / range;
  if (ratio < 0.25) return '#4CD964';
  if (ratio < 0.5) return '#FFCC00';
  if (ratio < 0.75) return '#FF9500';
  return '#FF3B30';
}

export default function ImuGauge({ label, value = 0, unit = '', range = 1 }) {
  const barWidth = Math.min(100, (Math.abs(value) / range) * 100);
  const color = valueToColor(value, range);

  return (
    <View style={styles.gauge}>
      <Text style={styles.label}>{label}</Text>
      <View style={styles.barContainer}>
        <View style={[styles.bar, { width: `${barWidth}%`, backgroundColor: color }]} />
      </View>
      <Text style={styles.value}>
        {value >= 0 ? '+' : ''}{value.toFixed(2)} {unit}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  gauge: {
    width: '30%',
    backgroundColor: '#3A3A3C',
    borderRadius: 8,
    padding: 8,
    alignItems: 'center',
  },
  label: {
    color: '#CCCCCC',
    fontSize: 10,
    marginBottom: 4,
  },
  barContainer: {
    width: '100%',
    height: 4,
    backgroundColor: '#555555',
    borderRadius: 2,
    overflow: 'hidden',
    marginBottom: 4,
  },
  bar: {
    height: '100%',
    borderRadius: 2,
  },
  value: {
    color: '#FFFFFF',
    fontSize: 11,
    fontWeight: '500',
  },
});