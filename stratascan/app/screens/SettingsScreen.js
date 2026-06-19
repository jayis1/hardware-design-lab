// SettingsScreen.js — App & Device Settings
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert, Switch } from 'react-native';
import { useBle } from '../utils/BleContext';
import { BAND_PRESETS } from '../utils/protocol';

export default function SettingsScreen() {
  const ble = useBle();
  const [autoGain, setAutoGain] = useState(true);
  const [backgroundSubtract, setBackgroundSubtract] = useState(true);
  const [autoRequestTraces, setAutoRequestTraces] = useState(true);
  const [exportFormat, setExportFormat] = useState('SEG-Y');
  const [firmwareVersion] = useState('1.0.0');

  const handleShutdown = () => {
    Alert.alert(
      'Shutdown Device',
      'Are you sure you want to power off the StrataScan device?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Shutdown', style: 'destructive', onPress: () => ble.sendCommand(0x0A) },
      ]
    );
  };

  const handleExport = () => {
    Alert.alert('Export', `Exporting B-scan as ${exportFormat}... (connect device to USB for file transfer)`);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Settings</Text>
      </View>

      {/* Connection settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Device</Text>
          <Text style={styles.rowValue}>{ble.deviceName || 'Not connected'}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Status</Text>
          <Text style={[styles.rowValue, { color: ble.connected ? '#0a0' : '#a00' }]}>
            {ble.connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {!ble.connected && (
          <TouchableOpacity style={styles.button} onPress={ble.connect}>
            <Text style={styles.buttonText}>Connect</Text>
          </TouchableOpacity>
        )}
        {ble.connected && (
          <TouchableOpacity style={[styles.button, styles.disconnectBtn]} onPress={ble.disconnect}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Processing settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Processing</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Auto-Gain Control</Text>
          <Switch
            value={autoGain}
            onValueChange={setAutoGain}
            trackColor={{ false: '#333', true: '#0066CC' }}
            thumbColor={autoGain ? '#0ff' : '#888'}
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Background Subtraction</Text>
          <Switch
            value={backgroundSubtract}
            onValueChange={setBackgroundSubtract}
            trackColor={{ false: '#333', true: '#0066CC' }}
            thumbColor={backgroundSubtract ? '#0ff' : '#888'}
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Auto-Request Traces</Text>
          <Switch
            value={autoRequestTraces}
            onValueChange={setAutoRequestTraces}
            trackColor={{ false: '#333', true: '#0066CC' }}
            thumbColor={autoRequestTraces ? '#0ff' : '#888'}
          />
        </View>
      </View>

      {/* Export settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Export</Text>
        <Text style={styles.subLabel}>Format</Text>
        <View style={styles.formatRow}>
          {['SEG-Y', 'HDF5', 'CSV'].map(fmt => (
            <TouchableOpacity
              key={fmt}
              style={[styles.formatBtn, exportFormat === fmt && styles.formatSelected]}
              onPress={() => setExportFormat(fmt)}
            >
              <Text style={styles.formatText}>{fmt}</Text>
            </TouchableOpacity>
          ))}
        </View>
        <TouchableOpacity style={[styles.button, styles.exportBtn]} onPress={handleExport}>
          <Text style={styles.buttonText}>Export Current B-Scan</Text>
        </TouchableOpacity>
      </View>

      {/* Device info */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Information</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Firmware</Text>
          <Text style={styles.rowValue}>v{firmwareVersion}</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>MCU</Text>
          <Text style={styles.rowValue}>STM32H743 (480MHz)</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Architecture</Text>
          <Text style={styles.rowValue}>SFCW GPR</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Frequency Range</Text>
          <Text style={styles.rowValue}>1 MHz – 3 GHz</Text>
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Max Depth</Text>
          <Text style={styles.rowValue}>~30 m (DEEP band)</Text>
        </View>
      </View>

      {/* Band reference */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Band Preset Reference</Text>
        {BAND_PRESETS.map(band => (
          <View key={band.id} style={styles.bandRefRow}>
            <Text style={styles.bandRefName}>{band.name}</Text>
            <Text style={styles.bandRefRange}>{band.range}</Text>
            <Text style={styles.bandRefDepth}>{band.depth}</Text>
            <Text style={styles.bandRefSteps}>{band.steps} steps</Text>
          </View>
        ))}
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          StrataScan is an open-source Stepped-Frequency Ground Penetrating Radar
          designed by jayis1. Copyright © 2026 jayis1.
        </Text>
        <Text style={styles.aboutText}>
          Hardware: CERN-OHL-S v2{'\n'}Firmware: GPL-2.0{'\n'}App: MIT
        </Text>
        <Text style={styles.aboutLink}>
          The first open-source GPR with complete SFCW architecture, on-device
          migration, and a real-time companion imaging app.
        </Text>
      </View>

      {/* Danger zone */}
      {ble.connected && (
        <View style={styles.section}>
          <Text style={[styles.sectionTitle, { color: '#a00' }]}>Danger Zone</Text>
          <TouchableOpacity style={[styles.button, styles.shutdownBtn]} onPress={handleShutdown}>
            <Text style={styles.buttonText}>Shutdown Device</Text>
          </TouchableOpacity>
        </View>
      )}

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  header: { padding: 15, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  title: { color: '#0ff', fontSize: 22, fontWeight: 'bold' },
  section: {
    backgroundColor: '#16213e', margin: 10, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
  },
  sectionTitle: { color: '#0ff', fontSize: 15, fontWeight: 'bold', marginBottom: 10 },
  row: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: '#0a0a1a',
  },
  rowLabel: { color: '#ccc', fontSize: 13 },
  rowValue: { color: '#888', fontSize: 13, fontFamily: 'monospace' },
  subLabel: { color: '#aaa', fontSize: 12, marginBottom: 8 },
  formatRow: { flexDirection: 'row', gap: 8, marginBottom: 10 },
  formatBtn: {
    backgroundColor: '#0a0a1a', paddingHorizontal: 16, paddingVertical: 8,
    borderRadius: 6, borderWidth: 1, borderColor: '#333',
  },
  formatSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  formatText: { color: '#ccc', fontSize: 12 },
  button: {
    backgroundColor: '#16213e', paddingHorizontal: 20, paddingVertical: 10,
    borderRadius: 8, borderWidth: 1, borderColor: '#333', alignItems: 'center', marginTop: 10,
  },
  exportBtn: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  disconnectBtn: { borderColor: '#aa0000', backgroundColor: '#2a1010' },
  shutdownBtn: { borderColor: '#aa0000', backgroundColor: '#2a1010' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  bandRefRow: {
    flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6,
    borderBottomWidth: 1, borderBottomColor: '#0a0a1a',
  },
  bandRefName: { color: '#0ff', fontSize: 12, fontWeight: 'bold', width: 50 },
  bandRefRange: { color: '#ccc', fontSize: 10, flex: 1 },
  bandRefDepth: { color: '#5a5', fontSize: 10, width: 70 },
  bandRefSteps: { color: '#888', fontSize: 10, width: 50 },
  aboutText: { color: '#aaa', fontSize: 12, lineHeight: 18, marginBottom: 8 },
  aboutLink: { color: '#666', fontSize: 11, fontStyle: 'italic', marginTop: 5 },
});