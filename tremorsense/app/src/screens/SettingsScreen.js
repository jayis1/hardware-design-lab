/*
 * SettingsScreen.js — App and device settings
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useContext } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Switch, Slider,
  Alert, Share, Linking,
} from 'react-native';
import { TremorContext } from '../models/TremorContext';
import BleManager from '../ble/BleManager';
import AsyncStorage from '@react-native-async-storage/async-storage';

export default function SettingsScreen() {
  const { bleState } = useContext(TremorContext);
  const [sampleRate, setSampleRate] = useState(1000);
  const [sensitivity, setSensitivity] = useState(2);
  const [displayOn, setDisplayOn] = useState(true);
  const [bleEnabled, setBleEnabled] = useState(true);
  const [nfcLogging, setNfcLogging] = useState(true);
  const [dataExportProgress, setDataExportProgress] = useState(0);
  const [downloading, setDownloading] = useState(false);
  const [firmwareVersion, setFirmwareVersion] = useState('1.0.0');

  const connectToDevice = async () => {
    try {
      await BleManager.scanAndConnect();
      Alert.alert('Scanning...', 'Looking for TremorSense devices nearby.');
    } catch (e) {
      Alert.alert('Error', 'Could not start BLE scan: ' + e.message);
    }
  };

  const disconnectFromDevice = async () => {
    await BleManager.disconnect();
    Alert.alert('Disconnected', 'TremorSense device disconnected.');
  };

  const updateSampleRate = async (value) => {
    setSampleRate(value);
    await BleManager.sendCommand('SET_SAMPLE_RATE', { rate: value });
    await AsyncStorage.setItem('sample_rate', String(value));
  };

  const updateSensitivity = async (value) => {
    setSensitivity(value);
    await BleManager.sendCommand('SET_SENSITIVITY', { level: value });
    await AsyncStorage.setItem('sensitivity', String(value));
  };

  const toggleDisplay = async (value) => {
    setDisplayOn(value);
    await BleManager.sendCommand('SET_DISPLAY', { on: value });
  };

  const toggleBLE = async (value) => {
    setBleEnabled(value);
    if (value) {
      await BleManager.enable();
    } else {
      await BleManager.disable();
    }
  };

  const downloadAllData = async () => {
    setDownloading(true);
    setDataExportProgress(0);

    try {
      const episodes = await BleManager.downloadAllEpisodes((progress) => {
        setDataExportProgress(progress);
      });

      // Save to AsyncStorage
      await AsyncStorage.setItem('downloaded_episodes', JSON.stringify(episodes));

      Alert.alert(
        'Download Complete',
        `Downloaded ${episodes.length} episodes from device.`
      );
    } catch (e) {
      Alert.alert('Error', 'Download failed: ' + e.message);
    } finally {
      setDownloading(false);
      setDataExportProgress(0);
    }
  };

  const exportDataCSV = async () => {
    try {
      const episodes = JSON.parse(
        await AsyncStorage.getItem('downloaded_episodes') || '[]'
      );

      if (episodes.length === 0) {
        Alert.alert('No Data', 'Download episodes from device first.');
        return;
      }

      // Build CSV
      let csv = 'Timestamp,Duration_ms,Frequency_Hz,Amplitude_g,Class,Confidence,Context\n';
      episodes.forEach(e => {
        csv += `${e.timestamp},${e.duration},${e.frequency.toFixed(2)},${e.amplitude.toFixed(4)},${e.class},${e.confidence.toFixed(3)},${e.context}\n`;
      });

      // Share via OS share sheet
      await Share.share({
        message: csv,
        title: 'TremorSense Data Export',
      });
    } catch (e) {
      Alert.alert('Error', 'Export failed: ' + e.message);
    }
  };

  const checkFirmwareUpdate = async () => {
    Alert.alert(
      'Firmware Update',
      `Current version: ${firmwareVersion}\nChecking for updates...`,
      [{ text: 'OK' }]
    );
    // In production: check Nordic DFU server for new firmware
  };

  const startDFU = async () => {
    Alert.alert(
      'Firmware Update',
      'Start OTA firmware update? Device will restart.',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Update', onPress: async () => {
          try {
            await BleManager.startDFU();
            Alert.alert('Updating', 'Firmware update in progress. Do not close the app.');
          } catch (e) {
            Alert.alert('Error', 'DFU failed: ' + e.message);
          }
        }},
      ]
    );
  };

  const factoryReset = () => {
    Alert.alert(
      'Factory Reset',
      'This will erase all data on the device. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Reset', style: 'destructive', onPress: async () => {
          await BleManager.sendCommand('FACTORY_RESET', {});
          Alert.alert('Reset Complete', 'Device will restart.');
        }},
      ]
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      {/* Connection */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Status</Text>
          <Text style={[styles.rowValue, { color: bleState === 'connected' ? '#34C759' : '#FF3B30' }]}>
            {bleState === 'connected' ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {bleState !== 'connected' ? (
          <TouchableOpacity style={styles.button} onPress={connectToDevice}>
            <Text style={styles.buttonText}>Connect to Device</Text>
          </TouchableOpacity>
        ) : (
          <TouchableOpacity style={[styles.button, styles.buttonDanger]} onPress={disconnectFromDevice}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Device settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Sample Rate: {sampleRate} Hz</Text>
          <Slider
            style={styles.slider}
            minimumValue={100}
            maximumValue={1000}
            step={100}
            value={sampleRate}
            onValueChange={updateSampleRate}
            minimumTrackTintColor="#0A84FF"
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Sensitivity: {['', 'Low', 'Medium', 'High'][sensitivity]}</Text>
          <Slider
            style={styles.slider}
            minimumValue={1}
            maximumValue={3}
            step={1}
            value={sensitivity}
            onValueChange={updateSensitivity}
            minimumTrackTintColor="#0A84FF"
          />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Display On</Text>
          <Switch value={displayOn} onValueChange={toggleDisplay}
            trackColor={{ false: '#3A3A3C', true: '#0A84FF' }} />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>BLE Enabled</Text>
          <Switch value={bleEnabled} onValueChange={toggleBLE}
            trackColor={{ false: '#3A3A3C', true: '#0A84FF' }} />
        </View>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>NFC Dose Logging</Text>
          <Switch value={nfcLogging} onValueChange={setNfcLogging}
            trackColor={{ false: '#3A3A3C', true: '#0A84FF' }} />
        </View>
      </View>

      {/* Data management */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Data</Text>
        <TouchableOpacity style={styles.button} onPress={downloadAllData}>
          <Text style={styles.buttonText}>
            {downloading ? `Downloading... ${dataExportProgress}%` : 'Download All Episodes'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.button} onPress={exportDataCSV}>
          <Text style={styles.buttonText}>Export as CSV</Text>
        </TouchableOpacity>
      </View>

      {/* Firmware */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <View style={styles.row}>
          <Text style={styles.rowLabel}>Version</Text>
          <Text style={styles.rowValue}>{firmwareVersion}</Text>
        </View>
        <TouchableOpacity style={styles.button} onPress={checkFirmwareUpdate}>
          <Text style={styles.buttonText}>Check for Updates</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.button} onPress={startDFU}>
          <Text style={styles.buttonText}>Start OTA Update</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          TremorSense Connect v1.0.0{'\n'}
          Author: jayis1{'\n'}
          Copyright © 2026 jayis1{'\n'}
          MIT License
        </Text>
      </View>

      {/* Danger zone */}
      <View style={styles.section}>
        <Text style={[styles.sectionTitle, { color: '#FF3B30' }]}>Danger Zone</Text>
        <TouchableOpacity style={[styles.button, styles.buttonDanger]} onPress={factoryReset}>
          <Text style={styles.buttonText}>Factory Reset Device</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold', marginBottom: 16 },
  section: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  sectionTitle: { color: '#8E8E93', fontSize: 14, marginBottom: 12, fontWeight: '600' },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 10,
  },
  rowLabel: { color: '#FFFFFF', fontSize: 16, flex: 1 },
  rowValue: { color: '#8E8E93', fontSize: 14 },
  slider: { flex: 1, height: 40, width: 120 },
  button: {
    backgroundColor: '#0A84FF',
    borderRadius: 8,
    padding: 14,
    alignItems: 'center',
    marginTop: 8,
  },
  buttonDanger: { backgroundColor: '#FF3B30' },
  buttonText: { color: '#FFFFFF', fontSize: 16, fontWeight: '600' },
  aboutText: { color: '#8E8E93', fontSize: 13, lineHeight: 20 },
});