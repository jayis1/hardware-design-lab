// PressureMeter.tsx — Live nib pressure gauge
//
// Renders a horizontal bar that fills proportional to current nib pressure,
// with color bands for light / medium / heavy. Useful during the calligraphy
// coaching flow and the calibration screen.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

type Props = { pressureMN: number; thresholdMN: number };

export default function PressureMeter({ pressureMN, thresholdMN }: Props) {
  const pct = Math.min(100, (pressureMN / 3000) * 100);
  const color = pressureMN < thresholdMN
    ? '#9aa0a6'
    : pressureMN < 800 ? '#4caf50'
    : pressureMN < 1500 ? '#ff9800'
    : '#f44336';

  return (
    <View style={styles.wrap}>
      <View style={styles.bar}>
        <View style={[styles.fill, { width: `${pct}%`, backgroundColor: color }]} />
      </View>
      <Text style={styles.label}>{pressureMN} mN  (thr {thresholdMN})</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap:   { marginVertical: 8 },
  bar:    { height: 12, borderRadius: 6, backgroundColor: '#e0e0e0', overflow: 'hidden' },
  fill:   { height: '100%', borderRadius: 6 },
  label:  { fontSize: 12, color: '#555', marginTop: 4 },
});