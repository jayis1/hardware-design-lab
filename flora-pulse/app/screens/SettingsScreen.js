/**
 * SettingsScreen.js — Device Settings & Data Management
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Provides device configuration, firmware info, calibration,
 * data download, and log management.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ScrollView,
  Alert, Switch, Slider
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { CMD, RSP, parseVersion } from '../utils/protocol';

export default function SettingsScreen() {
  const { connected, sendCommand, setFrameHandler, deviceInfo } = useBle();
  const [version, setVersion] = useState(null);
  const [threshold, setThreshold] = useState(50);
  const [adaptive, setAdaptive] = useState(true);
  const [sampleRate, setSampleRate] = useState(1);
  const [logCount, setLogCount] = useState(0);
  const [downloading, setDownloading] = useState(false);

  useEffect(() => {
    setFrameHandler((opcode, payload) => {
      if (opcode === RSP.VERSION) {
        setVersion(parseVersion(payload));
      }
      if (opcode === RSP.STATUS && payload.length >= 8) {
        const count = payload[3] | (payload[4] << 8) | (payload[5] << 16) | (payload[6] << 24);
        setLogCount(count >>> 0);
      }
      if (opcode === RSP.LOG_CHUNK) {
        // Handle downloaded log data
      }
    });
  }, [setFrameHandler]);

  useEffect(() => {
    if (connected) {
      sendCommand(CMD.GET_VERSION);
      sendCommand(CMD.GET_STATUS);
      sendCommand(CMD.GET_THRESHOLD);
    }
  }, [connected]);

  const handleSetThreshold = (val) => {
    setThreshold(val);
    const payload = [
      val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF,
    ];
    sendCommand(CMD.SET_THRESHOLD, payload);
  };

  const handleSetRate = (val) => {
    setSampleRate(val);
    sendCommand(CMD.SET_RATE, [val]);
  };

  const handleSyncTime = () => {
    const now = Math.floor(Date.now() / 1000);
    const payload = [
      now & 0xFF, (now >> 8) & 0xFF, (now >> 16) & 0xFF, (now >> 24) & 0xFF,
    ];
    sendCommand(CMD.SET_TIME, payload);
    Alert.alert('Time Synced', 'Device clock synchronized with phone');
  };

  const handleDownloadLogs = () => {
    setDownloading(true);
    Alert.alert(
      'Download Logs',
      `${logCount} records available. Download will begin now.`,
      [
        {
          text: 'OK',
          onPress: () => {
            sendCommand(CMD.DOWNLOAD_LOG, [0, 0, 0, 0]);
            setDownloading(false);
          }
        },
        { text: 'Cancel', onPress: () => setDownloading(false) },
      ]
    );
  };

  const handleEraseLogs = () => {
    Alert.alert(
      'Erase All Logs',
      'This will permanently delete all recorded data on the SD card. Are you sure?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Erase',
          style: 'destructive',
          onPress: () => {
            sendCommand(CMD.ERASE_LOG);
            setLogCount(0);
            Alert.alert('Done', 'All logs erased');
          }
        },
      ]
    );
  };

  const handleCalibration = () => {
    Alert.alert(
      'Calibration',
      'Start sensor calibration? This takes approximately 30 seconds.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start',
          onPress: () => {
            sendCommand(CMD.START_CALIB);
            Alert.alert('Calibrating', 'Calibration in progress...');
          }
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>⚙️ Settings</Text>
      </View>

      {connected ? (
        <View style={styles.content}>
          {/* Device info */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Device Information</Text>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Name</Text>
              <Text style={styles.infoValue}>{deviceInfo?.name || 'FloraPulse'}</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Firmware</Text>
              <Text style={styles.infoValue}>
                {version ? `v${version.major}.${version.minor}.${version.patch}` : '—'}
              </Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Author</Text>
              <Text style={styles.infoValue}>jayis1</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Hardware Rev</Text>
              <Text style={styles.infoValue}>{version?.hardwareRev || '1.0'}</Text>
            </View>
            <TouchableOpacity style={styles.syncButton} onPress={handleSyncTime}>
              <Text style={styles.syncButtonText}>🕐 Sync Clock</Text>
            </TouchableOpacity>
          </View>

          {/* Detection settings */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Detection Settings</Text>
            <View style={styles.settingRow}>
              <View>
                <Text style={styles.settingLabel}>AP Threshold</Text>
                <Text style={styles.settingValue}>{threshold} µV</Text>
              </View>
              <View style={styles.sliderContainer}>
                <Slider
                  style={styles.slider}
                  minimumValue={10}
                  maximumValue={500}
                  step={10}
                  value={threshold}
                  onValueChange={setThreshold}
                  onSlidingComplete={handleSetThreshold}
                  minimumTrackTintColor="#4CAF50"
                  maximumTrackTintColor="#333"
                />
              </View>
            </View>
            <View style={styles.settingRow}>
              <View>
                <Text style={styles.settingLabel}>Adaptive Threshold</Text>
                <Text style={styles.settingValue}>
                  {adaptive ? 'Enabled (noise tracking)' : 'Fixed threshold'}
                </Text>
              </View>
              <Switch
                value={adaptive}
                onValueChange={setAdaptive}
                trackColor={{ false: '#333', true: '#4CAF50' }}
              />
            </View>
            <View style={styles.settingRow}>
              <View>
                <Text style={styles.settingLabel}>Sample Rate</Text>
                <Text style={styles.settingValue}>{sampleRate} Hz</Text>
              </View>
              <View style={styles.rateButtons}>
                {[1, 5, 10].map((r) => (
                  <TouchableOpacity
                    key={r}
                    style={[styles.rateButton, sampleRate === r ? styles.rateButtonActive : null]}
                    onPress={() => handleSetRate(r)}
                  >
                    <Text style={styles.rateButtonText}>{r}Hz</Text>
                  </TouchableOpacity>
                ))}
              </View>
            </View>
          </View>

          {/* Data management */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Data Management</Text>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>SD Card Records</Text>
              <Text style={styles.infoValue}>{logCount.toLocaleString()}</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Record Size</Text>
              <Text style={styles.infoValue}>48 bytes</Text>
            </View>
            <View style={styles.infoRow}>
              <Text style={styles.infoLabel}>Est. Storage</Text>
              <Text style={styles.infoValue}>
                {((logCount * 48) / 1024).toFixed(1)} KB
              </Text>
            </View>
            <TouchableOpacity
              style={styles.actionButton}
              onPress={handleDownloadLogs}
              disabled={downloading || logCount === 0}
            >
              <Text style={styles.actionButtonText}>
                {downloading ? '⏳ Downloading...' : '📥 Download Logs'}
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.actionButton, styles.dangerButton]}
              onPress={handleEraseLogs}
              disabled={logCount === 0}
            >
              <Text style={styles.actionButtonText}>🗑 Erase All Logs</Text>
            </TouchableOpacity>
          </View>

          {/* Calibration */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>Calibration</Text>
            <Text style={styles.calibText}>
              Calibrate sensors for optimal accuracy. This includes:
              {'\n'}• ADS1292 offset calibration
              {'\n'}• ADS1220 thermistor baseline
              {'\n'}• BME280 environmental reference
              {'\n'}• Leaf-wetness dry-point calibration
            </Text>
            <TouchableOpacity style={styles.actionButton} onPress={handleCalibration}>
              <Text style={styles.actionButtonText}>🔧 Start Calibration</Text>
            </TouchableOpacity>
          </View>

          {/* About */}
          <View style={styles.card}>
            <Text style={styles.cardTitle}>About FloraPulse</Text>
            <Text style={styles.aboutText}>
              FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
            </Text>
            <Text style={styles.aboutText}>
              An open-source hardware platform for real-time plant stress
              monitoring via action potential detection, heat-pulse sap flow
              measurement, and environmental sensing.
            </Text>
            <Text style={styles.copyright}>
              Author: jayis1{'\n'}
              Copyright © 2026 jayis1. All rights reserved.{'\n'}
              License: CERN-OHL-S v2 (HW), GPL-2.0 (FW), MIT (App)
            </Text>
          </View>
        </View>
      ) : (
        <View style={styles.disconnected}>
          <Text style={styles.disconnectedIcon}>⚙️</Text>
          <Text style={styles.disconnectedText}>Not Connected</Text>
          <Text style={styles.disconnectedSubtext}>
            Connect to FloraPulse to configure settings
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d1b0d',
  },
  header: {
    padding: 16,
    paddingBottom: 8,
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  content: {
    padding: 16,
    paddingTop: 0,
  },
  card: {
    backgroundColor: '#1a2e1a',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#2d4a2d',
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#4CAF50',
    marginBottom: 12,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  infoLabel: {
    fontSize: 14,
    color: '#888',
  },
  infoValue: {
    fontSize: 14,
    color: '#fff',
    fontWeight: '500',
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
  },
  settingLabel: {
    fontSize: 14,
    color: '#fff',
    fontWeight: '500',
  },
  settingValue: {
    fontSize: 12,
    color: '#888',
    marginTop: 2,
  },
  sliderContainer: {
    flex: 1,
    marginLeft: 20,
  },
  slider: {
    width: '100%',
    height: 40,
  },
  rateButtons: {
    flexDirection: 'row',
    gap: 8,
  },
  rateButton: {
    backgroundColor: '#2d4a2d',
    borderRadius: 6,
    paddingHorizontal: 12,
    paddingVertical: 6,
  },
  rateButtonActive: {
    backgroundColor: '#4CAF50',
  },
  rateButtonText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '600',
  },
  syncButton: {
    backgroundColor: '#2196F3',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    marginTop: 12,
  },
  syncButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 14,
  },
  actionButton: {
    backgroundColor: '#2d4a2d',
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    marginTop: 8,
  },
  dangerButton: {
    backgroundColor: '#d32f2f',
  },
  actionButtonText: {
    color: '#fff',
    fontWeight: '600',
    fontSize: 14,
  },
  calibText: {
    color: '#ccc',
    fontSize: 13,
    lineHeight: 20,
    marginBottom: 8,
  },
  aboutText: {
    color: '#ccc',
    fontSize: 13,
    lineHeight: 20,
    marginBottom: 8,
  },
  copyright: {
    color: '#666',
    fontSize: 12,
    lineHeight: 18,
    marginTop: 8,
  },
  disconnected: {
    alignItems: 'center',
    paddingTop: 80,
    paddingHorizontal: 40,
  },
  disconnectedIcon: { fontSize: 64, marginBottom: 20 },
  disconnectedText: { fontSize: 24, fontWeight: 'bold', color: '#fff', marginBottom: 8 },
  disconnectedSubtext: { fontSize: 14, color: '#888', textAlign: 'center' },
});