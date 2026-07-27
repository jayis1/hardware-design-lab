/**
 * SettingsScreen.tsx — App and device settings.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Switch,
  Alert,
  Linking,
} from 'react-native';
import bleManager from '../ble/BleManager';
import { useDatabase } from '../db/database';

export default function SettingsScreen() {
  const [sweepMode, setSweepMode] = useState<'fast' | 'full'>('fast');
  const [autoChemistry, setAutoChemistry] = useState(true);
  const [bleEnabled, setBleEnabled] = useState(true);
  const [connected, setConnected] = useState(false);
  const db = useDatabase();

  React.useEffect(() => {
    bleManager.onConnectionChange((conn) => setConnected(conn));
  }, []);

  const handleClearHistory = useCallback(() => {
    Alert.alert(
      'Clear History',
      'Delete all stored cell test results? This cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => db.clearAll(),
        },
      ]
    );
  }, [db]);

  const handleDisconnect = useCallback(() => {
    bleManager.disconnect();
  }, []);

  const handleCalibrate = useCallback(() => {
    if (!connected) {
      Alert.alert('Not Connected', 'Connect to the device first.');
      return;
    }
    Alert.alert(
      'Calibration',
      'Short the probes to the calibration resistor and press OK.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'OK',
          onPress: () => bleManager.sendCommand(0x08),
        },
      ]
    );
  }, [connected]);

  return (
    <View style={styles.container}>
      {/* Sweep settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sweep Settings</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Sweep mode</Text>
          <View style={styles.toggleGroup}>
            <TouchableOpacity
              style={[styles.toggleButton, sweepMode === 'fast' && styles.toggleActive]}
              onPress={() => setSweepMode('fast')}
            >
              <Text style={[styles.toggleText, sweepMode === 'fast' && styles.toggleTextActive]}>
                Fast (20s)
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.toggleButton, sweepMode === 'full' && styles.toggleActive]}
              onPress={() => setSweepMode('full')}
            >
              <Text style={[styles.toggleText, sweepMode === 'full' && styles.toggleTextActive]}>
                Full (12min)
              </Text>
            </TouchableOpacity>
          </View>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-detect chemistry</Text>
          <Switch
            value={autoChemistry}
            onValueChange={setAutoChemistry}
            trackColor={{ false: '#222244', true: '#0066cc' }}
          />
        </View>
      </View>

      {/* Connection settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>BLE enabled</Text>
          <Switch
            value={bleEnabled}
            onValueChange={setBleEnabled}
            trackColor={{ false: '#222244', true: '#0066cc' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Device status</Text>
          <Text style={[styles.settingValue, { color: connected ? '#00e676' : '#f44336' }]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {connected && (
          <TouchableOpacity style={styles.actionButton} onPress={handleDisconnect}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Device actions */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Actions</Text>
        <TouchableOpacity style={styles.actionButton} onPress={handleCalibrate}>
          <Text style={styles.buttonText}>Run Calibration</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, styles.dangerButton]}
          onPress={handleClearHistory}
        >
          <Text style={styles.buttonText}>Clear All History</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>LithoCore v1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>License: MIT (app), GPL-3.0 (firmware), CERN-OHL-S v2 (hardware)</Text>
        <TouchableOpacity onPress={() => Linking.openURL('https://github.com/jayis1/lithocore')}>
          <Text style={styles.linkText}>github.com/jayis1/lithocore</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#12122a', padding: 16 },
  section: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: { color: '#00b4ff', fontSize: 14, fontWeight: 'bold', marginBottom: 12 },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
  },
  settingLabel: { color: '#e0e0e0', fontSize: 14 },
  settingValue: { color: '#e0e0e0', fontSize: 14, fontWeight: '600' },
  toggleGroup: { flexDirection: 'row', gap: 4 },
  toggleButton: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    backgroundColor: '#222244',
  },
  toggleActive: { backgroundColor: '#0066cc' },
  toggleText: { color: '#8888aa', fontSize: 12 },
  toggleTextActive: { color: '#fff', fontWeight: '600' },
  actionButton: {
    backgroundColor: '#0066cc',
    paddingVertical: 12,
    borderRadius: 8,
    marginTop: 8,
  },
  dangerButton: { backgroundColor: '#c62828' },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: '600', textAlign: 'center' },
  aboutText: { color: '#8888aa', fontSize: 12, marginBottom: 4 },
  linkText: { color: '#00b4ff', fontSize: 13, marginTop: 8 },
});