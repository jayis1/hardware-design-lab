/**
 * StatusBar.js — Connection & Battery Status Bar
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

export default function StatusBar({ connected, scanning, deviceName, batteryPct, onConnect, onDisconnect }) {
  const batteryColor = batteryPct > 50 ? '#4CAF50' : batteryPct > 20 ? '#ff9800' : '#f44336';

  return (
    <View style={styles.container}>
      <View style={styles.leftSection}>
        <View style={[styles.statusDot, connected ? styles.dotConnected : styles.dotDisconnected]} />
        <Text style={styles.statusText}>
          {connected ? (deviceName || 'Connected') : scanning ? 'Scanning...' : 'Disconnected'}
        </Text>
      </View>

      <View style={styles.rightSection}>
        {connected && batteryPct != null && (
          <View style={styles.batteryContainer}>
            <View style={styles.batteryIcon}>
              <View style={[styles.batteryFill, { width: `${batteryPct}%`, backgroundColor: batteryColor }]} />
            </View>
            <Text style={styles.batteryText}>{batteryPct}%</Text>
          </View>
        )}

        <TouchableOpacity
          style={[styles.button, connected ? styles.disconnectButton : styles.connectButton]}
          onPress={connected ? onDisconnect : onConnect}
        >
          <Text style={styles.buttonText}>
            {connected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#1a2e1a',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#2d4a2d',
  },
  leftSection: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  dotConnected: {
    backgroundColor: '#4CAF50',
    shadowColor: '#4CAF50',
    shadowOffset: { width: 0, height: 0 },
    shadowRadius: 4,
    shadowOpacity: 0.8,
  },
  dotDisconnected: {
    backgroundColor: '#666',
  },
  statusText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: '500',
  },
  rightSection: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  batteryContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 12,
  },
  batteryIcon: {
    width: 24,
    height: 12,
    borderWidth: 1,
    borderColor: '#888',
    borderRadius: 2,
    padding: 1,
    marginRight: 4,
  },
  batteryFill: {
    height: '100%',
    borderRadius: 1,
  },
  batteryText: {
    color: '#ccc',
    fontSize: 12,
  },
  button: {
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
  },
  connectButton: {
    backgroundColor: '#4CAF50',
  },
  disconnectButton: {
    backgroundColor: '#d32f2f',
  },
  buttonText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '600',
  },
});