/**
 * SettingsScreen.js — Device settings, calibration, and data management
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Alert,
  Switch,
  Share,
} from 'react-native';
import { useBle } from '../utils/BleContext';

const SettingsScreen = () => {
  const {
    connected,
    status,
    device,
    history,
    startCalibration,
    setDeviceTime,
    disconnect,
    clearHistory,
    startScan,
  } = useBle();

  const [autoSync, setAutoSync] = useState(true);
  const [notifications, setNotifications] = useState(true);
  const [units, setUnits] = useState('metric');

  const handleCalibrate = () => {
    Alert.alert(
      'Start Calibration',
      'Place your BreathPrint device in clean outdoor air for 60 seconds. This will recalibrate the sensor baselines. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Calibrate',
          onPress: async () => {
            const success = await startCalibration();
            if (success) {
              Alert.alert(
                'Calibration Started',
                'Keep the device in clean air. Calibration will take about 60 seconds.'
              );
            } else {
              Alert.alert('Error', 'Failed to start calibration.');
            }
          },
        },
      ]
    );
  };

  const handleSyncTime = async () => {
    const timestamp = Math.floor(Date.now() / 1000);
    const success = await setDeviceTime(timestamp);
    if (success) {
      Alert.alert('Time Synced', 'Device clock updated.');
    } else {
      Alert.alert('Error', 'Failed to sync time.');
    }
  };

  const handleDisconnect = () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from BreathPrint device?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Disconnect', onPress: () => disconnect() },
      ]
    );
  };

  const handleExportData = async () => {
    if (history.length === 0) {
      Alert.alert('No Data', 'No breath samples to export.');
      return;
    }

    // Convert history to CSV
    const header =
      'timestamp,state,quality,acetone_ppm,h2_ppm,ch4_ppm,ethanol_ppm,isoprene_ppb,nh3_ppm,h2s_ppb,voc_index,co2_ppm,temp,humidity,pressure,battery\n';
    const rows = history
      .map((s) =>
        [
          s.timestamp,
          s.metabolicState,
          s.breathQuality,
          s.acetonePpm.toFixed(2),
          s.h2Ppm.toFixed(1),
          s.ch4Ppm.toFixed(1),
          s.ethanolPpm.toFixed(2),
          s.isoprenePpb,
          s.nh3Ppm.toFixed(1),
          s.h2sPpb,
          s.vocIndex,
          s.co2Ppm,
          s.temperature.toFixed(1),
          s.humidity.toFixed(0),
          s.pressure,
          s.batteryPct,
        ].join(',')
      )
      .join('\n');

    const csv = header + rows;

    try {
      await Share.share({
        message: csv,
        title: 'BreathPrint Data Export',
      });
    } catch (e) {
      Alert.alert('Export Failed', e.message);
    }
  };

  const handleClearHistory = () => {
    Alert.alert(
      'Clear History',
      'Delete all stored breath samples? This cannot be undone.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: () => clearHistory(),
        },
      ]
    );
  };

  const SettingRow = ({ label, value, onPress, color }) => (
    <TouchableOpacity
      style={styles.settingRow}
      onPress={onPress}
      disabled={!onPress}
    >
      <Text style={styles.settingLabel}>{label}</Text>
      <Text style={[styles.settingValue, color && { color }]}>
        {value}
      </Text>
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.pageTitle}>Settings</Text>

      {/* Connection status */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Connection</Text>
        <View style={styles.card}>
          <SettingRow
            label="Status"
            value={connected ? 'Connected' : 'Disconnected'}
            color={connected ? '#4CAF50' : '#F44336'}
          />
          {connected && (
            <>
              <SettingRow
                label="Device ID"
                value={device?.id || 'Unknown'}
              />
              <SettingRow
                label="Battery"
                value={`${status.battery}%`}
                color={status.battery < 20 ? '#F44336' : '#4CAF50'}
              />
              <SettingRow
                label="Charging"
                value={status.charging ? 'Yes ⚡' : 'No'}
              />
              <SettingRow
                label="Device State"
                value={
                  ['Idle', 'Warming Up', 'Sampling', 'Processing',
                   'Analyzing', 'Result', 'Calibrating', 'Sleeping', 'Error'][status.state] || 'Unknown'
                }
              />
              <TouchableOpacity
                style={styles.actionButton}
                onPress={handleDisconnect}
              >
                <Text style={styles.actionButtonText}>Disconnect</Text>
              </TouchableOpacity>
            </>
          )}
          {!connected && (
            <TouchableOpacity
              style={styles.actionButton}
              onPress={() => startScan()}
            >
              <Text style={styles.actionButtonText}>Scan for Device</Text>
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Device controls */}
      {connected && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Device Controls</Text>
          <View style={styles.card}>
            <TouchableOpacity
              style={styles.actionButton}
              onPress={handleCalibrate}
            >
              <Text style={styles.actionButtonText}>
                🔄 Start Ambient Calibration
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.actionButtonSecondary}
              onPress={handleSyncTime}
            >
              <Text style={styles.actionButtonText}>🕐 Sync Device Time</Text>
            </TouchableOpacity>
          </View>
        </View>
      )}

      {/* App settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>App Settings</Text>
        <View style={styles.card}>
          <View style={styles.settingRow}>
            <Text style={styles.settingLabel}>Auto-sync data</Text>
            <Switch
              value={autoSync}
              onValueChange={setAutoSync}
              trackColor={{ false: '#333', true: '#2E86AB' }}
              thumbColor="#fff"
            />
          </View>
          <View style={styles.settingRow}>
            <Text style={styles.settingLabel}>Notifications</Text>
            <Switch
              value={notifications}
              onValueChange={setNotifications}
              trackColor={{ false: '#333', true: '#2E86AB' }}
              thumbColor="#fff"
            />
          </View>
          <SettingRow
            label="Units"
            value={units === 'metric' ? 'Metric (ppm, °C)' : 'Imperial'}
            onPress={() =>
              setUnits(units === 'metric' ? 'imperial' : 'metric')
            }
          />
        </View>
      </View>

      {/* Data management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Management</Text>
        <View style={styles.card}>
          <SettingRow
            label="Stored samples"
            value={history.length.toString()}
          />
          <TouchableOpacity
            style={styles.actionButton}
            onPress={handleExportData}
          >
            <Text style={styles.actionButtonText}>
              📤 Export Data (CSV)
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.actionButton, styles.dangerButton]}
            onPress={handleClearHistory}
          >
            <Text style={styles.dangerButtonText}>
              🗑 Clear All History
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <View style={styles.card}>
          <SettingRow label="App Version" value="1.0.0" />
          <SettingRow label="Firmware" value="v1.0" />
          <SettingRow label="Author" value="jayis1" />
          <SettingRow label="License" value="MIT" />
          <SettingRow
            label="Device Model"
            value="BreathPrint v1.0"
          />
        </View>
      </View>

      <Text style={styles.footer}>
        BreathPrint — Your metabolic fingerprint{'\n'}
        Copyright (c) 2026 jayis1 — MIT License
      </Text>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  pageTitle: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
    padding: 20,
  },
  section: {
    marginBottom: 8,
  },
  sectionTitle: {
    fontSize: 14,
    color: '#888',
    textTransform: 'uppercase',
    paddingHorizontal: 20,
    marginBottom: 8,
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    marginHorizontal: 16,
    overflow: 'hidden',
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 14,
    paddingHorizontal: 20,
    borderBottomWidth: 1,
    borderBottomColor: '#222',
  },
  settingLabel: {
    color: '#ccc',
    fontSize: 15,
  },
  settingValue: {
    color: '#888',
    fontSize: 15,
  },
  actionButton: {
    backgroundColor: '#2E86AB',
    paddingVertical: 14,
    paddingHorizontal: 20,
    alignItems: 'center',
    margin: 12,
    borderRadius: 12,
  },
  actionButtonSecondary: {
    backgroundColor: '#333',
    paddingVertical: 14,
    paddingHorizontal: 20,
    alignItems: 'center',
    marginHorizontal: 12,
    marginBottom: 12,
    borderRadius: 12,
  },
  actionButtonText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: 'bold',
  },
  dangerButton: {
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#F44336',
  },
  dangerButtonText: {
    color: '#F44336',
    fontSize: 15,
    fontWeight: 'bold',
  },
  footer: {
    color: '#555',
    fontSize: 12,
    textAlign: 'center',
    padding: 20,
    lineHeight: 18,
  },
});

export default SettingsScreen;