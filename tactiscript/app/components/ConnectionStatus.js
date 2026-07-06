/**
 * ConnectionStatus.js — BLE connection status banner component
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { ConnectionState } from '../utils/ble';

const MODE_NAMES = ['INIT', 'READER', 'NAV', 'MUSIC', 'TUTR', 'TXT', 'SLEEP'];

export default function ConnectionStatus({ state, batteryPct, mode }) {
  const stateInfo = {
    [ConnectionState.DISCONNECTED]: { color: '#cc0000', label: 'Disconnected' },
    [ConnectionState.SCANNING]: { color: '#cc8800', label: 'Scanning...' },
    [ConnectionState.CONNECTING]: { color: '#cc8800', label: 'Connecting...' },
    [ConnectionState.CONNECTED]: { color: '#008800', label: 'Connected' },
    [ConnectionState.ERROR]: { color: '#cc0000', label: 'Error' },
  };

  const info = stateInfo[state] || stateInfo[ConnectionState.DISCONNECTED];
  const modeName = MODE_NAMES[mode] || '?';

  return (
    <View style={[styles.banner, { backgroundColor: info.color }]}>
      <Text style={styles.text}>TactiScript</Text>
      <Text style={styles.text}>{info.label}</Text>
      <Text style={styles.text}>Mode: {modeName}</Text>
      <Text style={styles.text}>BAT: {batteryPct}%</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  banner: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 12,
  },
  text: {
    color: '#fff',
    fontSize: 12,
    fontWeight: 'bold',
  },
});