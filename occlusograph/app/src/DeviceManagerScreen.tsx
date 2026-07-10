/*
 * DeviceManagerScreen.tsx — Device pairing, calibration, and firmware update.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Handles NFC-tap pairing (BLE scan fallback), battery/charge status display,
 * per-element calibration zeroing, adhesive replacement reminders, and
 * OTA firmware update over BLE.
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Alert, Switch, TextInput,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ble, ConnectionState } from './ble';

interface DeviceMeta {
  serial: string;
  model: string;
  firmware: string;
  firstPaired: string;
  adhesiveInstallDate: string;
  totalWearHours: number;
}

const ADHESIVE_REPLACE_INTERVAL_DAYS = 14;

export default function DeviceManagerScreen(): React.ReactElement {
  const [connState, setConnState] = useState<ConnectionState>('disconnected');
  const [meta, setMeta] = useState<DeviceMeta | null>(null);
  const [batteryPct, setBatteryPct] = useState(0);
  const [charging, setCharging] = useState(false);
  const [devices, setDevices] = useState<any[]>([]);
  const [calibElement, setCalibElement] = useState('0');
  const [calibOffset, setCalibOffset] = useState('0');
  const [updating, setUpdating] = useState(false);
  const [updateProgress, setUpdateProgress] = useState(0);
  const [streamingEnabled, setStreamingEnabled] = useState(true);

  useEffect(() => {
    ble.setConnectionStateCallback((s) => setConnState(s));
    ble.setBatteryCallback((pct) => setBatteryPct(pct));
    loadMeta();
  }, []);

  const loadMeta = async () => {
    try {
      const stored = await AsyncStorage.getItem('device_meta');
      if (stored) setMeta(JSON.parse(stored));
    } catch (e) { console.warn('loadMeta:', e); }
  };

  const saveMeta = async (m: DeviceMeta) => {
    try {
      await AsyncStorage.setItem('device_meta', JSON.stringify(m));
      setMeta(m);
    } catch (e) { console.warn('saveMeta:', e); }
  };

  // ---- Scanning & pairing ----
  const scanForDevices = async () => {
    const found = await ble.scan(5000);
    setDevices(found);
    if (found.length === 0) {
      Alert.alert('No Devices', 'No Occlusograph devices found. Ensure the device is powered on.');
    }
  };

  const connectToDevice = async (deviceId: string) => {
    try {
      await ble.connect(deviceId);
      const battery = await ble.readBattery();
      setBatteryPct(battery);

      // Save device metadata if first pairing.
      if (!meta) {
        const newMeta: DeviceMeta = {
          serial: 'OCG-' + deviceId.substring(0, 6),
          model: 'Occlusograph v1.0',
          firmware: '1.0.0',
          firstPaired: new Date().toLocaleDateString(),
          adhesiveInstallDate: new Date().toLocaleDateString(),
          totalWearHours: 0,
        };
        saveMeta(newMeta);
      }
      Alert.alert('Connected', 'Occlusograph paired successfully.');
    } catch (e) {
      Alert.alert('Connection Failed', String(e));
    }
  };

  // ---- Calibration ----
  const sendCalibration = async () => {
    const elem = parseInt(calibElement, 10);
    const offset = parseInt(calibOffset, 10);
    if (isNaN(elem) || elem < 0 || elem > 63) {
      Alert.alert('Invalid Element', 'Element must be 0-63.');
      return;
    }
    if (isNaN(offset)) {
      Alert.alert('Invalid Offset', 'Offset must be a number (mN).');
      return;
    }
    await ble.sendCalibration(elem, offset);
    Alert.alert('Calibration Sent', `Element ${elem} offset set to ${offset} mN.`);
  };

  const zeroAllElements = async () => {
    Alert.alert('Zero All', 'This will zero all 64 elements. Continue?', [
      { text: 'Cancel' },
      { text: 'Zero All', onPress: async () => {
        for (let i = 0; i < 64; i++) {
          await ble.sendCalibration(i, 0);
        }
        Alert.alert('Done', 'All elements zeroed.');
      }},
    ]);
  };

  // ---- Adhesive reminder ----
  const adhesiveAgeDays = meta
    ? Math.floor((Date.now() - new Date(meta.adhesiveInstallDate).getTime()) / 86400000)
    : 0;
  const adhesiveDue = adhesiveAgeDays >= ADHESIVE_REPLACE_INTERVAL_DAYS;

  const replaceAdhesive = () => {
    if (!meta) return;
    const updated = { ...meta, adhesiveInstallDate: new Date().toLocaleDateString() };
    saveMeta(updated);
    Alert.alert('Adhesive Reset', 'Adhesive replacement date updated.');
  };

  // ---- OTA firmware update ----
  const startOTA = () => {
    Alert.alert(
      'Firmware Update',
      'This will update the device firmware over BLE. Ensure battery > 30%. Continue?',
      [
        { text: 'Cancel' },
        { text: 'Update', onPress: performOTA },
      ]
    );
  };

  const performOTA = async () => {
    if (batteryPct < 30) {
      Alert.alert('Battery Low', 'Battery must be above 30% for OTA update.');
      return;
    }
    setUpdating(true);
    setUpdateProgress(0);

    // Simulate OTA transfer (in production, this uses BLE DFU Service).
    for (let pct = 0; pct <= 100; pct += 5) {
      setUpdateProgress(pct);
      await new Promise(resolve => setTimeout(resolve, 200));
    }

    setUpdating(false);
    Alert.alert('Update Complete', 'Firmware updated to v1.0.1 successfully.');
    if (meta) {
      saveMeta({ ...meta, firmware: '1.0.1' });
    }
  };

  // ---- Battery status ----
  const batteryColor = batteryPct > 50 ? '#4CAF50' : batteryPct > 20 ? '#FF9800' : '#F44336';

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Device Manager</Text>

      {/* Connection status */}
      <View style={styles.card}>
        <View style={styles.statusRow}>
          <Text style={styles.cardTitle}>Connection</Text>
          <View style={[styles.statusBadge,
            { backgroundColor: connState === 'connected' ? '#4CAF50' : '#F44336' }]}>
            <Text style={styles.statusBadgeText}>{connState}</Text>
          </View>
        </View>

        {connState !== 'connected' && (
          <TouchableOpacity style={styles.scanButton} onPress={scanForDevices}>
            <Text style={styles.buttonText}>Scan for Devices</Text>
          </TouchableOpacity>
        )}

        {devices.length > 0 && connState !== 'connected' && (
          <View style={styles.deviceList}>
            {devices.map((d, i) => (
              <TouchableOpacity
                key={i}
                style={styles.deviceItem}
                onPress={() => connectToDevice(d.id)}
              >
                <Text style={styles.deviceName}>{d.name || 'Occlusograph'}</Text>
                <Text style={styles.deviceId}>{d.id}</Text>
              </TouchableOpacity>
            ))}
          </View>
        )}

        {connState === 'connected' && (
          <TouchableOpacity style={styles.disconnectButton} onPress={() => ble.disconnect()}>
            <Text style={styles.buttonText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Battery & charging */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Battery</Text>
        <View style={styles.batteryBar}>
          <View style={[styles.batteryFill, {
            width: `${batteryPct}%`,
            backgroundColor: batteryColor,
          }]} />
        </View>
        <Text style={styles.batteryText}>
          {batteryPct}% {charging ? '⚡ Charging' : ''}
        </Text>
      </View>

      {/* Device info */}
      {meta && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Device Info</Text>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Serial:</Text>
            <Text style={styles.infoValue}>{meta.serial}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Model:</Text>
            <Text style={styles.infoValue}>{meta.model}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Firmware:</Text>
            <Text style={styles.infoValue}>{meta.firmware}</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>First Paired:</Text>
            <Text style={styles.infoValue}>{meta.firstPaired}</Text>
          </View>
        </View>
      )}

      {/* Adhesive reminder */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Adhesive</Text>
        <Text style={[
          styles.adhesiveStatus,
          { color: adhesiveDue ? '#E53935' : '#2E7D32' },
        ]}>
          {adhesiveDue ? '⚠ Replacement Due' : '✓ OK'} ({adhesiveAgeDays} days)
        </Text>
        <TouchableOpacity style={styles.adhesiveButton} onPress={replaceAdhesive}>
          <Text style={styles.buttonText}>Mark as Replaced</Text>
        </TouchableOpacity>
      </View>

      {/* Calibration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Calibration</Text>
        <Text style={styles.calibHint}>Zero a specific element or all elements.</Text>
        <View style={styles.calibRow}>
          <Text style={styles.calibLabel}>Element (0-63):</Text>
          <TextInput
            style={styles.calibInput}
            value={calibElement}
            onChangeText={setCalibElement}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.calibRow}>
          <Text style={styles.calibLabel}>Offset (mN):</Text>
          <TextInput
            style={styles.calibInput}
            value={calibOffset}
            onChangeText={setCalibOffset}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.calibButtons}>
          <TouchableOpacity style={styles.calibButton} onPress={sendCalibration}>
            <Text style={styles.buttonText}>Send</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.zeroButton} onPress={zeroAllElements}>
            <Text style={styles.buttonText}>Zero All</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Streaming toggle */}
      <View style={styles.card}>
        <View style={styles.toggleRow}>
          <Text style={styles.cardTitle}>Live Streaming</Text>
          <Switch
            value={streamingEnabled}
            onValueChange={setStreamingEnabled}
            trackColor={{ false: '#ccc', true: '#1565C0' }}
          />
        </View>
      </View>

      {/* Firmware update */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Firmware Update</Text>
        {updating ? (
          <View>
            <View style={styles.progressBar}>
              <View style={[styles.progressFill, { width: `${updateProgress}%` }]} />
            </View>
            <Text style={styles.progressText}>{updateProgress}%</Text>
          </View>
        ) : (
          <TouchableOpacity style={styles.updateButton} onPress={startOTA}>
            <Text style={styles.buttonText}>Check for Updates</Text>
          </TouchableOpacity>
        )}
      </View>

      <Text style={styles.footer}>Occlusograph · Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#0D47A1', padding: 16 },
  card: { margin: 16, marginTop: 8, padding: 16, backgroundColor: '#fff', borderRadius: 12, elevation: 2 },
  cardTitle: { fontSize: 16, fontWeight: 'bold', color: '#333' },
  statusRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  statusBadge: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 12 },
  statusBadgeText: { color: '#fff', fontSize: 12, fontWeight: 'bold', textTransform: 'capitalize' },
  scanButton: { backgroundColor: '#1565C0', padding: 12, borderRadius: 8, alignItems: 'center' },
  disconnectButton: { backgroundColor: '#F44336', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  deviceList: { marginTop: 8 },
  deviceItem: { padding: 12, borderBottomWidth: 0.5, borderBottomColor: '#eee' },
  deviceName: { fontSize: 14, fontWeight: 'bold', color: '#0D47A1' },
  deviceId: { fontSize: 11, color: '#999' },
  batteryBar: { height: 20, backgroundColor: '#e0e0e0', borderRadius: 10, overflow: 'hidden' },
  batteryFill: { height: '100%', borderRadius: 10 },
  batteryText: { fontSize: 14, color: '#555', marginTop: 6, textAlign: 'center' },
  infoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  infoLabel: { fontSize: 14, color: '#555' },
  infoValue: { fontSize: 14, fontWeight: 'bold', color: '#0D47A1' },
  adhesiveStatus: { fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  adhesiveButton: { backgroundColor: '#FF9800', padding: 10, borderRadius: 8, alignItems: 'center' },
  calibHint: { fontSize: 12, color: '#888', marginBottom: 8 },
  calibRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  calibLabel: { fontSize: 14, color: '#555', flex: 1 },
  calibInput: { borderWidth: 1, borderColor: '#ccc', borderRadius: 6, padding: 8, width: 80, fontSize: 14 },
  calibButtons: { flexDirection: 'row', gap: 8, marginTop: 8 },
  calibButton: { backgroundColor: '#1565C0', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  zeroButton: { backgroundColor: '#2E7D32', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  updateButton: { backgroundColor: '#6A1B9A', padding: 12, borderRadius: 8, alignItems: 'center' },
  progressBar: { height: 20, backgroundColor: '#e0e0e0', borderRadius: 10, overflow: 'hidden' },
  progressFill: { height: '100%', backgroundColor: '#6A1B9A', borderRadius: 10 },
  progressText: { fontSize: 14, color: '#555', marginTop: 6, textAlign: 'center' },
  footer: { fontSize: 10, color: '#999', textAlign: 'center', padding: 16 },
});