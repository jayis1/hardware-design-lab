/**
 * SettingsScreen.js — App settings and device info
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Linking, Alert,
} from 'react-native';
import { useBle } from '../ble/BleContext';
import Colors from '../constants/Colors';

const SettingsScreen = () => {
  const { connected, deviceInfo, disconnect, device } = useBle();

  const handleDisconnect = () => {
    Alert.alert('Disconnect?', 'Disconnect from AlloyScan device?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Disconnect', style: 'destructive', onPress: disconnect },
    ]);
  };

  const openDocs = () => {
    Linking.openURL('https://github.com/jayis1/alloyscan/wiki');
  };

  return (
    <View style={styles.container}>
      {/* Connection section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Status</Text>
          <Text style={[styles.rowValue, { color: connected ? Colors.green : Colors.red }]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {device && (
          <View style={styles.row}>
            <Text style={styles.rowLabel}>Device</Text>
            <Text style={styles.rowValue}>{device.name || 'AlloyScan'}</Text>
          </View>
        )}
        {deviceInfo && (
          <>
            <View style={styles.row}>
              <Text style={styles.rowLabel}>Firmware</Text>
              <Text style={styles.rowValue}>{deviceInfo.fw || '1.0.0'}</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.rowLabel}>Battery</Text>
              <Text style={styles.rowValue}>{deviceInfo.battery || '?'}%</Text>
            </View>
            <View style={styles.row}>
              <Text style={styles.rowLabel}>Calibration</Text>
              <Text style={styles.rowValue}>
                {deviceInfo.calibrated ? 'Valid' : 'Needed'}
              </Text>
            </View>
          </>
        )}
        {connected && (
          <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
            <Text style={styles.disconnectButtonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Device settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Settings</Text>
        <TouchableOpacity style={styles.settingRow}>
          <Text style={styles.settingLabel}>Excitation Amplitude</Text>
          <Text style={styles.settingValue}>100%</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.settingRow}>
          <Text style={styles.settingLabel}>Confidence Threshold</Text>
          <Text style={styles.settingValue}>70%</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-scan Interval</Text>
          <Text style={styles.settingValue}>Off</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.settingRow}>
          <Text style={styles.settingLabel}>Haptic Feedback</Text>
          <Text style={styles.settingValue}>On</Text>
        </TouchableOpacity>
      </View>

      {/* About section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <View style={styles.aboutRow}>
          <Text style={styles.aboutLabel}>App Version</Text>
          <Text style={styles.aboutValue}>1.0.0</Text>
        </View>
        <View style={styles.aboutRow}>
          <Text style={styles.aboutLabel}>Author</Text>
          <Text style={styles.aboutValue}>jayis1</Text>
        </View>
        <View style={styles.aboutRow}>
          <Text style={styles.aboutLabel}>License</Text>
          <Text style={styles.aboutValue}>MIT</Text>
        </View>
        <View style={styles.aboutRow}>
          <Text style={styles.aboutLabel}>Hardware License</Text>
          <Text style={styles.aboutValue}>CERN-OHL-S v2</Text>
        </View>
      </View>

      {/* Help section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Help</Text>
        <TouchableOpacity style={styles.linkRow} onPress={openDocs}>
          <Text style={styles.linkText}>📖 Documentation</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.linkRow} onPress={openDocs}>
          <Text style={styles.linkText}>🔧 Troubleshooting</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.linkRow} onPress={openDocs}>
          <Text style={styles.linkText}>📡 BLE Connection Guide</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.footer}>AlloyScan © 2026 jayis1</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.dark },
  section: {
    backgroundColor: Colors.darkGray,
    marginHorizontal: 12,
    marginTop: 12,
    borderRadius: 12,
    padding: 16,
  },
  sectionTitle: {
    color: Colors.accent,
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 12,
    textTransform: 'uppercase',
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  rowLabel: { color: Colors.gray, fontSize: 14 },
  rowValue: { color: Colors.white, fontSize: 14, fontWeight: 'bold' },
  disconnectButton: {
    backgroundColor: Colors.red,
    borderRadius: 10,
    paddingVertical: 12,
    alignItems: 'center',
    marginTop: 16,
  },
  disconnectButtonText: { color: Colors.white, fontWeight: 'bold' },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  settingLabel: { color: Colors.lightGray, fontSize: 15 },
  settingValue: { color: Colors.cyan, fontSize: 15 },
  aboutRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
  },
  aboutLabel: { color: Colors.gray, fontSize: 14 },
  aboutValue: { color: Colors.white, fontSize: 14 },
  linkRow: { paddingVertical: 12 },
  linkText: { color: Colors.cyan, fontSize: 15 },
  footer: {
    color: Colors.gray,
    fontSize: 12,
    textAlign: 'center',
    padding: 20,
  },
});

export default SettingsScreen;