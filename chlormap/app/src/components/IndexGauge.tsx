// src/components/IndexGauge.tsx — Circular gauge widget for vegetation indices
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface IndexGaugeProps {
  label: string;
  value: number;
  min: number;
  max: number;
  unit: string;
  color: string;
}

export default function IndexGauge({ label, value, min, max, unit, color }: IndexGaugeProps) {
  const range = max - min;
  const pct = Math.max(0, Math.min(1, (value - min) / range));

  // Circular arc gauge (semi-circle)
  const arcAngle = pct * 180; // 0 to 180 degrees

  return (
    <View style={[styles.container, { borderColor: color }]}>
      {/* Label */}
      <Text style={styles.label}>{label}</Text>

      {/* Value */}
      <Text style={[styles.value, { color }]}>
        {value.toFixed(label === 'SPAD' || label === 'Temp' ? 1 : 3)}
        <Text style={styles.unit}> {unit}</Text>
      </Text>

      {/* Bar gauge */}
      <View style={styles.barBg}>
        <View
          style={[
            styles.barFg,
            {
              width: `${pct * 100}%`,
              backgroundColor: color,
            },
          ]}
        />
      </View>

      {/* Min/max labels */}
      <View style={styles.rangeRow}>
        <Text style={styles.rangeText}>{min}</Text>
        <Text style={styles.rangeText}>{max}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#152815',
    borderRadius: 8,
    padding: 8,
    marginHorizontal: 4,
    borderWidth: 1,
    alignItems: 'center',
  },
  label: {
    color: '#b0bec5',
    fontSize: 11,
    fontWeight: '600',
    marginBottom: 4,
  },
  value: {
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  unit: {
    fontSize: 10,
    fontWeight: 'normal',
  },
  barBg: {
    width: '100%',
    height: 6,
    backgroundColor: '#0a1f0a',
    borderRadius: 3,
    overflow: 'hidden',
  },
  barFg: {
    height: '100%',
    borderRadius: 3,
  },
  rangeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    width: '100%',
    marginTop: 2,
  },
  rangeText: {
    color: '#558b55',
    fontSize: 8,
  },
});