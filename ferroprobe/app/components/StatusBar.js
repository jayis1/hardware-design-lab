/**
 * StatusBar.js — Device Connection & Status Bar
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const MODE_NAMES = {
  0: 'BOOT', 1: 'IDLE', 2: 'SURVEY', 3: 'CALIB', 4: 'MONITOR', 5: 'CONFIG',
};

export default function StatusBar({ connectionState, status, deviceInfo }) {
  const connected = connectionState === 'connected';

  return (
    <View style={styles.container}>
      {/* Connection indicator */}
      <View style={styles.section}>
        <View style={[styles.dot, { backgroundColor: connected ? '#4CAF50' : '#F44336' }]} />
        <Text style={styles.text}>
          {connected ? 'Connected' : connectionState}
        </Text>
      </View>

      {/* Device mode */}
      {status && (
        <View style={styles.section}>
          <Text style={styles.label}>Mode:</Text>
          <Text style={styles.value}>{MODE_NAMES[status.mode] || '—'}</Text>
        </View>
      )}

      {/* Battery */}
      {status && (
        <View style={styles.section}>
          <Text style={styles.label}>🔋</Text>
          <Text style={styles.value}>{status.battery}%</Text>
        </View>
      )}

      {/* GPS fix */}
      {status && (
        <View style={styles.section}>
          <Text style={styles.label}>GPS</Text>
          <Text style={[styles.value, { color: status.gpsFix ? '#4CAF50' : '#F44336' }]}>
            {status.gpsFix ? '✓' : '✗'}
          </Text>
        </View>
      )}

      {/* Temperature */}
      {status && (
        <View style={styles.section}>
          <Text style={styles.label}>Temp</Text>
          <Text style={styles.value}>{status.temperature}°C</Text>
        </View>
      )}

      {/* Firmware version */}
      {deviceInfo && (
        <View style={styles.section}>
          <Text style={styles.label}>FW</Text>
          <Text style={styles.value}>
            v{deviceInfo.major}.{deviceInfo.minor}.{deviceInfo.patch}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'center',
    backgroundColor: '#16162e',
    paddingVertical: 8,
    paddingHorizontal: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  section: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 6,
  },
  label: {
    color: '#888',
    fontSize: 11,
    marginRight: 4,
  },
  value: {
    color: '#fff',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  text: {
    color: '#ccc',
    fontSize: 12,
  },
});