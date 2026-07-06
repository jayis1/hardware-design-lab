/**
 * SettingsScreen.js — Device configuration and diagnostics
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet, ScrollView,
  Switch, Alert,
} from 'react-native';
import { BLEManager, Commands, ConnectionState } from '../utils/ble';

export default function SettingsScreen() {
  const [deviceName, setDeviceName] = useState('TactiScript');
  const [autoConnect, setAutoConnect] = useState(true);
  const [hapticTestRunning, setHapticTestRunning] = useState(false);
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');

  const handleReconnect = useCallback(() => {
    BLEManager.disconnect();
    setTimeout(() => BLEManager.connect(), 500);
    Alert.alert('Reconnecting', 'Attempting to reconnect to TactiScript ring.');
  }, []);

  const handleDisconnect = useCallback(() => {
    BLEManager.disconnect();
    Alert.alert('Disconnected', 'BLE connection closed.');
  }, []);

  const handleSleep = useCallback(() => {
    BLEManager.sendCommand(Commands.MODE_SLEEP);
    Alert.alert('Sleep', 'Device is entering sleep mode. Tap the ring to wake.');
  }, []);

  const handleHapticTest = useCallback(() => {
    setHapticTestRunning(true);
    BLEManager.sendCommand(Commands.MODE_TEXTURE);
    // Send a full-intensity test pattern
    const testFrame = new Array(24).fill(255);
    BLEManager.sendTexture(testFrame);
    setTimeout(() => {
      const offFrame = new Array(24).fill(0);
      BLEManager.sendTexture(offFrame);
      setHapticTestRunning(false);
    }, 500);
  }, []);

  const handleFactoryTest = useCallback(() => {
    Alert.alert(
      'Factory Test',
      'Running diagnostic test. The ring will cycle through all actuator elements.',
      [
        { text: 'Start', onPress: () => {
          BLEManager.sendCommand(Commands.MODE_TEXTURE);
          // Cycle through each element
          for (let i = 0; i < 24; i++) {
            const frame = new Array(24).fill(0);
            frame[i] = 255;
            setTimeout(() => BLEManager.sendTexture(frame), i * 200);
          }
          setTimeout(() => {
            BLEManager.sendTexture(new Array(24).fill(0));
          }, 24 * 200 + 500);
        }},
        { text: 'Cancel', style: 'cancel' },
      ]
    );
  }, []);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.subtitle}>Configure your TactiScript ring</Text>

      {/* Connection section */}
      <Text style={styles.sectionLabel}>Connection</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Device Name:</Text>
          <TextInput
            style={styles.textInput}
            value={deviceName}
            onChangeText={setDeviceName}
            editable={false}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-connect:</Text>
          <Switch value={autoConnect} onValueChange={setAutoConnect} />
        </View>
        <View style={styles.buttonRow}>
          <TouchableOpacity style={styles.reconnectBtn} onPress={handleReconnect}>
            <Text style={styles.btnText}>Reconnect</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.disconnectBtn} onPress={handleDisconnect}>
            <Text style={styles.btnText}>Disconnect</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Diagnostics section */}
      <Text style={styles.sectionLabel}>Diagnostics</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Firmware:</Text>
          <Text style={styles.valueText}>v{firmwareVersion}</Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Author:</Text>
          <Text style={styles.valueText}>jayis1</Text>
        </View>
        <View style={styles.buttonRow}>
          <TouchableOpacity
            style={[styles.testBtn, hapticTestRunning && styles.testBtnActive]}
            onPress={handleHapticTest}
            disabled={hapticTestRunning}
          >
            <Text style={styles.btnText}>
              {hapticTestRunning ? 'Testing...' : 'Haptic Test'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.factoryBtn} onPress={handleFactoryTest}>
            <Text style={styles.btnText}>Factory Test</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Power section */}
      <Text style={styles.sectionLabel}>Power</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.sleepBtn} onPress={handleSleep}>
          <Text style={styles.btnText}>Sleep Mode</Text>
        </TouchableOpacity>
      </View>

      {/* About section */}
      <Text style={styles.sectionLabel}>About</Text>
      <View style={styles.card}>
        <Text style={styles.aboutText}>TactiScript v1.0</Text>
        <Text style={styles.aboutText}>Wearable Fingertip Haptic Display Ring</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright (C) 2026 jayis1</Text>
        <Text style={styles.aboutText}>License: CERN-OHL-S v2 (HW), GPL-2.0 (FW), MIT (App)</Text>
      </View>

      <Text style={styles.footer}>TactiScript — by jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  sectionLabel: { fontSize: 16, fontWeight: 'bold', color: '#333', marginTop: 16, marginBottom: 8 },
  card: { backgroundColor: '#fff', borderRadius: 12, padding: 16, marginBottom: 8 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  settingLabel: { fontSize: 14, color: '#333' },
  valueText: { fontSize: 14, color: '#666' },
  textInput: { borderWidth: 1, borderColor: '#ccc', borderRadius: 6, padding: 8, fontSize: 14, width: 150 },
  buttonRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 12 },
  reconnectBtn: { backgroundColor: '#0066CC', padding: 10, borderRadius: 8, flex: 1, marginRight: 4, alignItems: 'center' },
  disconnectBtn: { backgroundColor: '#cc0000', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  testBtn: { backgroundColor: '#008800', padding: 10, borderRadius: 8, flex: 1, marginRight: 4, alignItems: 'center' },
  testBtnActive: { backgroundColor: '#999' },
  factoryBtn: { backgroundColor: '#cc8800', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  sleepBtn: { backgroundColor: '#333', padding: 14, borderRadius: 10, alignItems: 'center' },
  btnText: { color: '#fff', fontWeight: 'bold' },
  aboutText: { fontSize: 12, color: '#666', paddingVertical: 2 },
  footer: { fontSize: 10, color: '#999', marginTop: 20, marginBottom: 20, textAlign: 'center' },
});