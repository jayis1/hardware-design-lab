// ============================================================
// LignoScan App — StatusBar Component
// Compact status bar showing connection, battery, scan state
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { useBle } from '../utils/BleContext';
import { BLE_STATE } from '../utils/protocol';

export default function StatusBar() {
  const { connected, batteryLevel, scanStatus, gpsData } = useBle();

  const stateColor = () => {
    if (!connected) return '#999';
    if (scanStatus?.state === BLE_STATE.ERROR) return '#cc0000';
    if (scanStatus?.state === BLE_STATE.IDLE) return '#2d8a2d';
    return '#e6c200';
  };

  const stateLabel = () => {
    if (!connected) return 'Offline';
    if (!scanStatus) return 'Ready';
    switch (scanStatus.state) {
      case BLE_STATE.IDLE: return 'Ready';
      case BLE_STATE.CALIBRATING: return 'Cal';
      case BLE_STATE.SCANNING: return `${scanStatus.progress}%`;
      case BLE_STATE.RECONSTRUCT: return 'Recon';
      case BLE_STATE.TRANSMITTING: return 'TX';
      case BLE_STATE.ERROR: return 'Error';
      default: return 'Ready';
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.section}>
        <View style={[styles.dot, { backgroundColor: stateColor() }]} />
        <Text style={styles.text}>{stateLabel()}</Text>
      </View>
      <View style={styles.section}>
        <Text style={styles.text}>🔋 {batteryLevel}%</Text>
      </View>
      <View style={styles.section}>
        <Text style={styles.text}>
          📡 {gpsData?.satellites || 0} sats
        </Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a3a1a',
    paddingHorizontal: 12,
    paddingVertical: 6,
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
  text: {
    color: '#e0f0e0',
    fontSize: 12,
  },
});

// EOF — StatusBar.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT