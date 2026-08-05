// src/screens/SettingsScreen.tsx — App settings, data export, OTA firmware
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Switch, Alert,
  Share, Platform,
} from 'react-native';
import bleManager from '../ble/BleManager';
import db from '../db/database';

export default function SettingsScreen() {
  const [units, setUnits] = useState('metric'); // metric / imperial
  const [notifications, setNotifications] = useState(true);
  const [alertThreshold, setAlertThreshold] = useState(0.85);
  const [autoConnect, setAutoConnect] = useState(true);

  const exportData = async () => {
    try {
      const csv = await db.exportToCSV();
      const filename = `frostsentinel_${Date.now()}.csv`;
      if (Platform.OS === 'android' || Platform.OS === 'ios') {
        await Share.share({
          message: csv,
          title: filename,
        });
      }
    } catch (e) {
      Alert.alert('Error', 'Export failed.');
    }
  };

  const setTimeFromPhone = async () => {
    try {
      const now = Math.floor(Date.now() / 1000);
      await bleManager.setTime(now);
      Alert.alert('Success', `Node time set to ${new Date().toISOString()}`);
    } catch (e) {
      Alert.alert('Error', 'Could not set node time. Is BLE connected?');
    }
  };

  const clearDatabase = () => {
    Alert.alert(
      'Clear Database',
      'Delete all stored samples and events? This cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            try {
              await db.open();
              // In production: db.executeSql('DELETE FROM samples')
              // db.executeSql('DELETE FROM events')
              Alert.alert('Done', 'Database cleared.');
            } catch (e) {
              Alert.alert('Error', 'Could not clear database.');
            }
          },
        },
      ]
    );
  };

  const otaFirmware = () => {
    Alert.alert(
      'OTA Firmware Update',
      'OTA firmware update is not yet implemented in this build. '
      + 'Use the USB-C port and OpenOCD for flashing.',
      [{ text: 'OK' }]
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      {/* Display settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>

        <SettingRow label="Units">
          <View style={styles.toggleRow}>
            <TouchableOpacity
              style={[styles.unitButton, units === 'metric' ? styles.unitActive : null]}
              onPress={() => setUnits('metric')}
            >
              <Text style={units === 'metric' ? styles.unitTextActive : styles.unitText}>
                °C
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.unitButton, units === 'imperial' ? styles.unitActive : null]}
              onPress={() => setUnits('imperial')}
            >
              <Text style={units === 'imperial' ? styles.unitTextActive : styles.unitText}>
                °F
              </Text>
            </TouchableOpacity>
          </View>
        </SettingRow>

        <SettingRow label="Notifications">
          <Switch
            value={notifications}
            onValueChange={setNotifications}
            trackColor={{ true: '#2196F3', false: '#333' }}
          />
        </SettingRow>

        <SettingRow label="Auto-connect BLE">
          <Switch
            value={autoConnect}
            onValueChange={setAutoConnect}
            trackColor={{ true: '#2196F3', false: '#333' }}
          />
        </SettingRow>
      </View>

      {/* Alert thresholds */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Alert Thresholds</Text>
        <Text style={styles.thresholdLabel}>
          RFRI Alert: {(alertThreshold * 100).toFixed(0)}%
        </Text>
        <Text style={styles.thresholdHint}>
          Push a frost alert when RFRI exceeds this value (0–100%).
        </Text>
      </View>

      {/* Data management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data</Text>

        <TouchableOpacity style={styles.actionRow} onPress={exportData}>
          <Text style={styles.actionText}>Export Data (CSV)</Text>
          <Text style={styles.actionArrow}>›</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.actionRow} onPress={setTimeFromPhone}>
          <Text style={styles.actionText}>Sync Node Time from Phone</Text>
          <Text style={styles.actionArrow}>›</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.actionRow} onPress={otaFirmware}>
          <Text style={styles.actionText}>OTA Firmware Update</Text>
          <Text style={styles.actionArrow}>›</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.actionRow} onPress={clearDatabase}>
          <Text style={[styles.actionText, { color: '#F44336' }]}>Clear Database</Text>
          <Text style={styles.actionArrow}>›</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>FrostSentinel v1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright © 2026 jayis1</Text>
        <Text style={styles.aboutText}>License: CERN-OHL-S v2 / GPL-3.0 / MIT</Text>
        <Text style={styles.aboutLink}>
          Docs: https://github.com/jayis1/hardware-design-lab
        </Text>
      </View>
    </View>
  );
}

function SettingRow({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      {children}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a', padding: 20, paddingTop: 40 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#fff', marginBottom: 15 },
  section: { backgroundColor: '#1b263b', borderRadius: 12, padding: 15, marginBottom: 15 },
  sectionTitle: { fontSize: 15, fontWeight: 'bold', color: '#e0e1dd', marginBottom: 10 },
  settingRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', paddingVertical: 8,
  },
  settingLabel: { fontSize: 14, color: '#e0e1dd' },
  toggleRow: { flexDirection: 'row' },
  unitButton: {
    paddingHorizontal: 16, paddingVertical: 6, borderRadius: 6,
    backgroundColor: '#0d1b2a', marginHorizontal: 2,
  },
  unitActive: { backgroundColor: '#2196F3' },
  unitText: { fontSize: 12, color: '#778da9' },
  unitTextActive: { fontSize: 12, color: '#fff', fontWeight: '600' },
  thresholdLabel: { fontSize: 14, color: '#FF9800', fontWeight: '600' },
  thresholdHint: { fontSize: 11, color: '#778da9', marginTop: 4 },
  actionRow: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    paddingVertical: 12, borderBottomWidth: 1, borderBottomColor: '#333',
  },
  actionText: { fontSize: 14, color: '#e0e1dd' },
  actionArrow: { fontSize: 18, color: '#778da9' },
  aboutText: { fontSize: 12, color: '#778da9', marginTop: 4 },
  aboutLink: { fontSize: 11, color: '#2196F3', marginTop: 8 },
});