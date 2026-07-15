/**
 * BatteryWidget — Battery and solar power gauge
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import type { Telemetry } from '../types';

interface Props {
  telemetry: Telemetry | null;
}

export const BatteryWidget: React.FC<Props> = ({ telemetry }) => {
  const pct = telemetry?.batteryPct ?? 0;
  const solarMv = telemetry?.solarMv ?? 0;
  const solarActive = solarMv > 100;
  const state = telemetry?.state ?? 'idle';

  const batteryColor = pct > 50 ? '#4a4' : pct > 20 ? '#aa4' : '#a44';
  const stateColor = {
    active: '#4a4', idle: '#88a', sleep: '#558',
    solar_sustain: '#fa4', fault: '#a44',
  }[state] ?? '#888';

  return (
    <View style={styles.container}>
      <View style={styles.row}>
        <Text style={styles.label}>Battery</Text>
        <View style={styles.batteryBar}>
          <View style={[styles.batteryFill, { width: `${pct}%`, backgroundColor: batteryColor }]} />
        </View>
        <Text style={styles.value}>{pct}%</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Solar</Text>
        <Text style={[styles.value, { color: solarActive ? '#fa4' : '#666' }]}>
          {solarActive ? `${solarMv} mV` : 'Off'}
        </Text>
        <Text style={styles.label}>State</Text>
        <Text style={[styles.value, { color: stateColor }]}>{state}</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Skin Temp</Text>
        <Text style={styles.value}>
          {telemetry ? `${telemetry.skinTempAvgC}°C avg / ${telemetry.skinTempMaxC}°C max` : '—'}
        </Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>LoRa RSSI</Text>
        <Text style={styles.value}>
          {telemetry ? `${telemetry.loraRssi} dBm` : '—'}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    marginVertical: 8,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    marginVertical: 4,
  },
  label: {
    color: '#888',
    fontSize: 12,
    width: 80,
  },
  value: {
    color: '#ccc',
    fontSize: 13,
    fontFamily: 'monospace',
    flex: 1,
  },
  batteryBar: {
    flex: 1,
    height: 14,
    backgroundColor: '#222',
    borderRadius: 4,
    overflow: 'hidden',
  },
  batteryFill: {
    height: '100%',
    borderRadius: 4,
  },
});