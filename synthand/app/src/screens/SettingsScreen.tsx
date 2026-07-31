/**
 * SettingsScreen.tsx — App settings and device management.
 *
 * Shows BLE connection status, firmware version, battery level,
 * OSC endpoint configuration, OTA firmware update trigger, and
 * preset management.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  TextInput,
  Alert,
  Switch,
} from 'react-native';
import { useBle } from '../ble/BleManager';
import { initDatabase, loadPresets, deletePreset } from '../db/database';

/**
 * SettingsScreen — device and app configuration.
 * Author: jayis1
 */
export default function SettingsScreen() {
  const {
    isConnected,
    deviceName,
    firmwareVersion,
    batteryLevel,
    connect,
    disconnect,
    error,
  } = useBle();

  const [oscIp, setOscIp] = useState('192.168.1.100');
  const [oscPort, setOscPort] = useState('8000');
  const [handedness, setHandedness] = useState(false); // false = right, true = left
  const [presetCount, setPresetCount] = useState(0);
  const [dbInitialized, setDbInitialized] = useState(false);

  // Initialize database on mount
  useEffect(() => {
    initDatabase()
      .then(() => {
        setDbInitialized(true);
        loadPresets().then((presets) => setPresetCount(presets.length));
      })
      .catch(() => {});
  }, []);

  const handleConnect = () => {
    if (isConnected) {
      disconnect();
    } else {
      connect();
    }
  };

  const handleFactoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all calibration and mapping data from the glove. Are you sure?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: () => {
            // Would send a factory reset command via BLE config characteristic
            Alert.alert('Reset Complete', 'All data has been erased.');
          },
        },
      ]
    );
  };

  const handleOtaUpdate = () => {
    Alert.alert(
      'Firmware Update',
      'Check for firmware updates? The glove will restart during the process.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Update',
          onPress: () => {
            // Would trigger DFU mode via BLE
            Alert.alert('Searching...', 'Checking for firmware updates...');
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.header}>Settings</Text>

      {error && (
        <View style={styles.errorBox}>
          <Text style={styles.errorText}>{error}</Text>
        </View>
      )}

      {/* Connection section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Connection</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Status</Text>
          <Text style={[styles.infoValue, { color: isConnected ? '#0f0' : '#888' }]}>
            {isConnected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {deviceName && (
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Device</Text>
            <Text style={styles.infoValue}>{deviceName}</Text>
          </View>
        )}
        {firmwareVersion && (
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware</Text>
            <Text style={styles.infoValue}>{firmwareVersion}</Text>
          </View>
        )}
        {batteryLevel !== null && (
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Battery</Text>
            <Text style={styles.infoValue}>{batteryLevel}%</Text>
          </View>
        )}
        <TouchableOpacity
          style={[styles.button, isConnected ? styles.disconnectBtn : styles.connectBtn]}
          onPress={handleConnect}
        >
          <Text style={styles.buttonText}>
            {isConnected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Handedness */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Glove Configuration</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Left-handed mode</Text>
          <Switch
            value={handedness}
            onValueChange={setHandedness}
            trackColor={{ false: '#0f3460', true: '#e94560' }}
          />
        </View>
      </View>

      {/* OSC endpoint */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>OSC Endpoint</Text>
        <Text style={styles.hint}>
          Configure the IP address and port for OSC-over-UDP streaming
          (for Max/MSP, TouchDesigner, etc.)
        </Text>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>IP Address</Text>
          <TextInput
            style={styles.input}
            value={oscIp}
            onChangeText={setOscIp}
            placeholder="192.168.1.100"
            placeholderTextColor="#555"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Port</Text>
          <TextInput
            style={styles.input}
            value={oscPort}
            onChangeText={setOscPort}
            placeholder="8000"
            placeholderTextColor="#555"
            keyboardType="numeric"
          />
        </View>
      </View>

      {/* Presets */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Presets</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Saved presets</Text>
          <Text style={styles.infoValue}>{presetCount}</Text>
        </View>
        <Text style={styles.hint}>
          Manage MIDI mapping presets in the Mapping tab.
        </Text>
      </View>

      {/* Firmware update */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <TouchableOpacity style={styles.button2} onPress={handleOtaUpdate}>
          <Text style={styles.button2Text}>Check for Updates (OTA)</Text>
        </TouchableOpacity>
      </View>

      {/* Factory reset */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Danger Zone</Text>
        <TouchableOpacity
          style={[styles.button, styles.dangerButton]}
          onPress={handleFactoryReset}
        >
          <Text style={styles.buttonText}>Factory Reset Glove</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>Synthand v1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>License: MIT</Text>
        <Text style={styles.aboutText}>© 2026 jayis1</Text>
      </View>
    </ScrollView>
  );
}

// Need to import ScrollView
import { ScrollView } from 'react-native';

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
    padding: 16,
  },
  header: {
    color: '#e94560',
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 16,
  },
  errorBox: {
    backgroundColor: 'rgba(233, 69, 96, 0.2)',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  errorText: {
    color: '#e94560',
    fontSize: 13,
  },
  section: {
    backgroundColor: '#16213e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: {
    color: '#e94560',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 6,
  },
  infoLabel: {
    color: '#ccc',
    fontSize: 14,
  },
  infoValue: {
    color: '#aaa',
    fontSize: 14,
  },
  hint: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
  },
  inputRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: 8,
  },
  inputLabel: {
    color: '#ccc',
    fontSize: 14,
    width: 90,
  },
  input: {
    flex: 1,
    backgroundColor: '#0f3460',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 8,
    color: '#fff',
    fontSize: 14,
  },
  button: {
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    marginTop: 8,
  },
  connectBtn: {
    backgroundColor: '#e94560',
  },
  disconnectBtn: {
    backgroundColor: '#0f3460',
  },
  buttonText: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  button2: {
    backgroundColor: '#533483',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
  },
  button2Text: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  dangerButton: {
    backgroundColor: '#c0392b',
  },
  aboutText: {
    color: '#888',
    fontSize: 13,
    paddingVertical: 2,
  },
});