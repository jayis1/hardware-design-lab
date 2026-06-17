/**
 * SettingsScreen.js — Device Configuration & Data Export
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Alert, Share } from 'react-native';
import { useBle } from '../utils/BleContext';
import StatusBar from '../components/StatusBar';
import * as protocol from '../utils/protocol';

const RATE_NAMES = ['Fast (100 Hz)', 'Survey (10 Hz)', 'Precision (1 Hz)', 'Ultra (0.1 Hz)'];

export default function SettingsScreen() {
  const ble = useBle();
  const [selectedRate, setSelectedRate] = useState(1);
  const [threshold, setThreshold] = useState(2000);
  const [versionInfo, setVersionInfo] = useState(null);

  useEffect(() => {
    if (ble.deviceInfo) {
      setVersionInfo(ble.deviceInfo);
    }
  }, [ble.deviceInfo]);

  const handleSetRate = (rate) => {
    setSelectedRate(rate);
    ble.setRate(rate);
  };

  const handleThresholdChange = (delta) => {
    const newThreshold = Math.max(100, Math.min(50000, threshold + delta));
    setThreshold(newThreshold);
    ble.setThreshold(newThreshold);
  };

  const handleEraseLogs = () => {
    Alert.alert(
      'Erase All Logs',
      'This will permanently delete all survey data on the SD card. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Erase', style: 'destructive', onPress: () => ble.sendCommand(protocol.buildEraseLog()) },
      ]
    );
  };

  const handleExportData = async () => {
    try {
      await Share.share({
        message: 'FerroProbe survey data export',
        title: 'FerroProbe Data',
      });
    } catch (err) {
      Alert.alert('Export failed', err.message);
    }
  };

  const handleGetVersion = () => {
    ble.sendCommand(protocol.buildGetVersion());
  };

  const handleGetCalib = () => {
    ble.getCalib();
  };

  return (
    <ScrollView style={styles.container}>
      <StatusBar
        connectionState={ble.connectionState}
        status={ble.status}
        deviceInfo={ble.deviceInfo}
      />

      {/* Device Info Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device Info</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Firmware</Text>
          <Text style={styles.infoValue}>
            {versionInfo
              ? `v${versionInfo.major}.${versionInfo.minor}.${versionInfo.patch}`
              : 'Unknown'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Hardware</Text>
          <Text style={styles.infoValue}>
            {versionInfo ? `Rev ${versionInfo.hardwareRev}` : 'Unknown'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Author</Text>
          <Text style={styles.infoValue}>
            {versionInfo ? versionInfo.author : 'jayis1'}
          </Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Connection</Text>
          <Text style={[styles.infoValue, {
            color: ble.connectionState === 'connected' ? '#4CAF50' : '#F44336',
          }]}>
            {ble.connectionState}
          </Text>
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleGetVersion}>
          <Text style={styles.actionButtonText}>Refresh Version</Text>
        </TouchableOpacity>
      </View>

      {/* Sample Rate Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sample Rate</Text>
        {RATE_NAMES.map((name, idx) => (
          <TouchableOpacity
            key={idx}
            style={[
              styles.rateOption,
              selectedRate === idx && styles.rateOptionActive,
            ]}
            onPress={() => handleSetRate(idx)}
          >
            <Text style={[
              styles.rateText,
              selectedRate === idx && styles.rateTextActive,
            ]}>
              {name}
            </Text>
            {selectedRate === idx && <Text style={styles.checkmark}>✓</Text>}
          </TouchableOpacity>
        ))}
      </View>

      {/* Anomaly Threshold Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Anomaly Threshold</Text>
        <View style={styles.thresholdContainer}>
          <TouchableOpacity
            style={styles.thresholdButton}
            onPress={() => handleThresholdChange(-100)}
          >
            <Text style={styles.thresholdButtonText}>−100</Text>
          </TouchableOpacity>
          <View style={styles.thresholdValueContainer}>
            <Text style={styles.thresholdValue}>
              {(threshold / 1000).toFixed(2)} µT
            </Text>
            <Text style={styles.thresholdNt}>{threshold} nT</Text>
          </View>
          <TouchableOpacity
            style={styles.thresholdButton}
            onPress={() => handleThresholdChange(100)}
          >
            <Text style={styles.thresholdButtonText}>+100</Text>
          </TouchableOpacity>
        </View>
        <Text style={styles.thresholdHint}>
          The device will alert when |B| deviates more than this from 50 µT
        </Text>
      </View>

      {/* Data Management Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data Management</Text>
        {ble.status && (
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Records</Text>
            <Text style={styles.infoValue}>{ble.status.recordCount}</Text>
          </View>
        )}
        <TouchableOpacity style={styles.actionButton} onPress={handleExportData}>
          <Text style={styles.actionButtonText}>Export Survey Data (CSV)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionButton, styles.dangerButton]} onPress={handleEraseLogs}>
          <Text style={[styles.actionButtonText, { color: '#F44336' }]}>Erase All Logs</Text>
        </TouchableOpacity>
      </View>

      {/* Calibration Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Calibration</Text>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Status</Text>
          <Text style={[styles.infoValue, {
            color: ble.status?.calibValid ? '#4CAF50' : '#FFC107',
          }]}>
            {ble.status?.calibValid ? 'Valid' : 'Not calibrated'}
          </Text>
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleGetCalib}>
          <Text style={styles.actionButtonText}>Get Calibration Parameters</Text>
        </TouchableOpacity>
        {ble.calibData && (
          <View style={styles.calibDisplay}>
            <Text style={styles.calibHeader}>Current Calibration:</Text>
            <Text style={styles.calibLine}>
              Offset: [{ble.calibData.offset[0].toFixed(1)}, {' '}
              {ble.calibData.offset[1].toFixed(1)}, {' '}
              {ble.calibData.offset[2].toFixed(1)}] nT
            </Text>
            <Text style={styles.calibLine}>
              Scale: [{ble.calibData.scale[0].toFixed(4)}, {' '}
              {ble.calibData.scale[1].toFixed(4)}, {' '}
              {ble.calibData.scale[2].toFixed(4)}]
            </Text>
          </View>
        )}
      </View>

      {/* About Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          FerroProbe is an open-source 3-axis vector fluxgate magnetometer
          for geophysical surveying, archaeological prospecting, UXO detection,
          and space science.
        </Text>
        <Text style={styles.aboutText}>
          Author: jayis1{'\n'}
          Copyright © 2026 jayis1{'\n'}
          License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f23' },
  section: {
    backgroundColor: '#1a1a2e',
    marginHorizontal: 15,
    marginTop: 15,
    borderRadius: 12,
    padding: 15,
  },
  sectionTitle: { color: '#2196F3', fontSize: 16, fontWeight: 'bold', marginBottom: 12 },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  infoLabel: { color: '#888', fontSize: 14 },
  infoValue: { color: '#fff', fontSize: 14, fontFamily: 'monospace' },
  rateOption: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 12,
    paddingHorizontal: 15,
    borderRadius: 8,
    marginBottom: 5,
    backgroundColor: '#2a2a3e',
  },
  rateOptionActive: { backgroundColor: '#2196F3' },
  rateText: { color: '#ccc', fontSize: 14 },
  rateTextActive: { color: '#fff', fontWeight: 'bold' },
  checkmark: { color: '#fff', fontSize: 18 },
  thresholdContainer: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginVertical: 10,
  },
  thresholdButton: {
    backgroundColor: '#2a2a3e',
    width: 60,
    height: 40,
    borderRadius: 8,
    justifyContent: 'center',
    alignItems: 'center',
  },
  thresholdButtonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  thresholdValueContainer: { alignItems: 'center' },
  thresholdValue: { color: '#fff', fontSize: 20, fontWeight: 'bold', fontFamily: 'monospace' },
  thresholdNt: { color: '#888', fontSize: 12 },
  thresholdHint: { color: '#666', fontSize: 11, marginTop: 8, textAlign: 'center' },
  actionButton: {
    backgroundColor: '#2a2a3e',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 8,
  },
  actionButtonText: { color: '#2196F3', fontSize: 14 },
  dangerButton: { backgroundColor: '#3a1a1a' },
  calibDisplay: {
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    padding: 12,
    marginTop: 10,
  },
  calibHeader: { color: '#888', fontSize: 12, marginBottom: 6 },
  calibLine: { color: '#ccc', fontSize: 12, fontFamily: 'monospace', marginBottom: 4 },
  aboutText: { color: '#ccc', fontSize: 13, lineHeight: 20, marginBottom: 10 },
});