// StatusBar.js — Device Status Bar Component
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StatusBar({ status }) {
  if (!status) {
    return (
      <View style={styles.container}>
        <Text style={styles.disconnected}>No device connected</Text>
      </View>
    );
  }

  const stateColor = {
    BOOT: '#ff8800',
    IDLE: '#00aa00',
    CALIBRATE: '#ffaa00',
    SURVEY: '#0066ff',
    PAUSE: '#ffaa00',
    SHUTDOWN: '#ff0000',
  }[status.stateName] || '#888';

  return (
    <View style={styles.container}>
      <View style={styles.row}>
        <View style={[styles.pill, { backgroundColor: stateColor }]}>
          <Text style={styles.pillText}>{status.stateName}</Text>
        </View>
        <View style={styles.pill}>
          <Text style={styles.pillText}>BAND: {status.bandName}</Text>
        </View>
        <View style={styles.pill}>
          <Text style={styles.pillText}>TR: {status.tracesAcquired}</Text>
        </View>
      </View>
      <View style={styles.row}>
        <Text style={styles.info}>Battery: {status.batteryPct}%</Text>
        <Text style={styles.info}>
          Cal: {status.calibrated ? '✓' : '✗'}
        </Text>
        {status.lat !== 0 && (
          <Text style={styles.info}>
            GPS: {status.lat.toFixed(5)}, {status.lon.toFixed(5)}
          </Text>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1a1a2e',
    padding: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'center',
    marginBottom: 4,
  },
  pill: {
    backgroundColor: '#16213e',
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: '#333',
  },
  pillText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  info: {
    color: '#aaa',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  disconnected: {
    color: '#ff4444',
    textAlign: 'center',
    fontSize: 14,
  },
});