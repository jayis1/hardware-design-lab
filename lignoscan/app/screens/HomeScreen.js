// ============================================================
// LignoScan App — Home Screen
// Connection status, battery, scan trigger, quick guide
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React from 'react';
import { View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert } from 'react-native';
import { useBle } from '../utils/BleContext';
import { BLE_STATE } from '../utils/protocol';

export default function HomeScreen({ navigation }) {
  const {
    connected, scanning, scanStatus, batteryLevel, sensorCount,
    startScan, startDeviceScan, error, scanProgress,
  } = useBle();

  const handleScan = () => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to the LignoScan device first.');
      return;
    }
    startDeviceScan();
    navigation.navigate('Tomogram');
  };

  const handleConnect = () => {
    startScan();
  };

  const statusText = () => {
    if (!connected) return 'Disconnected';
    switch (scanStatus.state) {
      case BLE_STATE.IDLE: return 'Ready';
      case BLE_STATE.CALIBRATING: return 'Calibrating...';
      case BLE_STATE.SCANNING: return `Scanning... ${scanStatus.progress}%`;
      case BLE_STATE.RECONSTRUCT: return 'Reconstructing...';
      case BLE_STATE.TRANSMITTING: return 'Transmitting...';
      case BLE_STATE.ERROR: return 'Error';
      default: return 'Ready';
    }
  };

  const statusColor = () => {
    if (!connected) return '#999';
    if (scanStatus.state === BLE_STATE.ERROR) return '#cc0000';
    if (scanStatus.state === BLE_STATE.IDLE) return '#2d8a2d';
    return '#e6c200';
  };

  return (
    <ScrollView style={styles.container}>
      {/* Connection Status Card */}
      <View style={styles.card}>
        <View style={styles.statusRow}>
          <View style={[styles.statusDot, { backgroundColor: statusColor() }]} />
          <Text style={styles.statusText}>{statusText()}</Text>
        </View>

        <View style={styles.infoGrid}>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Battery</Text>
            <Text style={styles.infoValue}>{batteryLevel}%</Text>
          </View>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Sensors</Text>
            <Text style={styles.infoValue}>{sensorCount}</Text>
          </View>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>GPS</Text>
            <Text style={styles.infoValue}>--</Text>
          </View>
        </View>

        {error && (
          <Text style={styles.errorText}>Error: {error}</Text>
        )}

        {!connected ? (
          <TouchableOpacity style={styles.connectButton} onPress={handleConnect}>
            <Text style={styles.buttonText}>
              {scanning ? 'Searching...' : 'Connect to LignoScan'}
            </Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={styles.scanButton} onPress={handleScan}>
            <Text style={styles.scanButtonText}>Start Scan</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Quick Navigation */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Actions</Text>
        <TouchableOpacity
          style={styles.navButton}
          onPress={() => navigation.navigate('ScanList')}
        >
          <Text style={styles.navButtonText}>📋 Scan History</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navButton}
          onPress={() => navigation.navigate('TreeInventory')}
        >
          <Text style={styles.navButtonText}>🗺️ Tree Inventory Map</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navButton}
          onPress={() => navigation.navigate('Settings')}
        >
          <Text style={styles.navButtonText}>⚙️ Settings</Text>
        </TouchableOpacity>
      </View>

      {/* Quick Start Guide */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Quick Start Guide</Text>
        <Text style={styles.guideText}>
          1. Wrap the sensor strap around the tree trunk{'\n'}
          2. Attach 8-16 ultrasonic sensors to magnetic clamps{'\n'}
          3. Enter tree ID and diameter in Settings{'\n'}
          4. Press "Start Scan" on the device or app{'\n'}
          5. Review the tomogram for decay visualization{'\n'}
          6. Generate and share PDF inspection report
        </Text>
      </View>

      <Text style={styles.footer}>
        LignoScan v1.0 — Author: jayis1{'\n'}
        Copyright © 2026 jayis1 — MIT License
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f0' },
  card: {
    backgroundColor: '#fff',
    margin: 12,
    padding: 16,
    borderRadius: 10,
    elevation: 2,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.2,
    shadowRadius: 2,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  statusDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 8,
  },
  statusText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333',
  },
  infoGrid: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginBottom: 12,
  },
  infoItem: {
    alignItems: 'center',
  },
  infoLabel: {
    fontSize: 12,
    color: '#888',
    marginBottom: 4,
  },
  infoValue: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#1a3a1a',
  },
  errorText: {
    color: '#cc0000',
    fontSize: 13,
    marginBottom: 8,
  },
  connectButton: {
    backgroundColor: '#1a3a1a',
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  scanButton: {
    backgroundColor: '#2d8a2d',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  scanButtonText: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#1a3a1a',
    marginBottom: 10,
  },
  navButton: {
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#eee',
  },
  navButtonText: {
    fontSize: 15,
    color: '#333',
  },
  guideText: {
    fontSize: 14,
    color: '#555',
    lineHeight: 24,
  },
  footer: {
    textAlign: 'center',
    fontSize: 11,
    color: '#aaa',
    marginVertical: 16,
  },
});

// EOF — HomeScreen.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT