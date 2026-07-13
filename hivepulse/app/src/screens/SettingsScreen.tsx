/**
 * SettingsScreen — Device configuration and BLE pairing
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, TextInput, TouchableOpacity, StyleSheet,
  Switch, Alert, ScrollView,
} from 'react-native';

export default function SettingsScreen() {
  const [samplingInterval, setSamplingInterval] = useState('5');
  const [listenDuration, setListenDuration] = useState('30');
  const [winterMode, setWinterMode] = useState(false);
  const [varroaThreshold, setVarroaThreshold] = useState('80');
  const [swarmAlerts, setSwarmAlerts] = useState(true);
  const [queenlessAlerts, setQueenlessAlerts] = useState(true);
  const [loraDevEui, setLoraDevEui] = useState('0000000000000001');
  const [loraAppKey, setLoraAppKey] = useState('');
  const [bleScanning, setBleScanning] = useState(false);
  const [pairedDevices, setPairedDevices] = useState<string[]>([]);

  const handleScan = () => {
    setBleScanning(true);
    setTimeout(() => {
      setBleScanning(false);
      setPairedDevices(['HivePulse-0001', 'HivePulse-0002']);
    }, 3000);
  };

  const handleTare = () => {
    Alert.alert('Tare Weight', 'Send tare command to all paired devices?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Tare All', onPress: () => Alert.alert('Success', 'Tare sent') },
    ]);
  };

  const handleSaveSettings = () => {
    Alert.alert('Settings Saved', 'Configuration will be sent to devices on next sync.');
  };

  const SettingRow = ({ label, children }: { label: string; children: React.ReactNode }) => (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      {children}
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>⚙️ Settings</Text>

      {/* BLE section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>📱 BLE Pairing</Text>
        <TouchableOpacity
          style={styles.scanButton}
          onPress={handleScan}
          disabled={bleScanning}
        >
          <Text style={styles.scanButtonText}>
            {bleScanning ? '🔍 Scanning...' : '🔍 Scan for HivePulse Devices'}
          </Text>
        </TouchableOpacity>
        {pairedDevices.map((device, i) => (
          <View key={i} style={styles.deviceRow}>
            <Text style={styles.deviceIcon}>🐝</Text>
            <Text style={styles.deviceName}>{device}</Text>
            <Text style={styles.deviceStatus}>Connected</Text>
          </View>
        ))}
      </View>

      {/* Sampling section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>🎙️ Acoustic Sampling</Text>
        <SettingRow label="Sampling Interval (min)">
          <TextInput
            style={styles.input}
            value={samplingInterval}
            onChangeText={setSamplingInterval}
            keyboardType="numeric"
          />
        </SettingRow>
        <SettingRow label="Listen Duration (sec)">
          <TextInput
            style={styles.input}
            value={listenDuration}
            onChangeText={setListenDuration}
            keyboardType="numeric"
          />
        </SettingRow>
        <SettingRow label="Winter Mode Override">
          <Switch value={winterMode} onValueChange={setWinterMode}
            trackColor={{ false: '#333', true: '#03A9F4' }} />
        </SettingRow>
      </View>

      {/* Alert thresholds */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>🔔 Alert Thresholds</Text>
        <SettingRow label="Varroa Confidence Threshold (%)">
          <TextInput
            style={styles.input}
            value={varroaThreshold}
            onChangeText={setVarroaThreshold}
            keyboardType="numeric"
          />
        </SettingRow>
        <SettingRow label="Swarm Alerts">
          <Switch value={swarmAlerts} onValueChange={setSwarmAlerts}
            trackColor={{ false: '#333', true: '#FFC107' }} />
        </SettingRow>
        <SettingRow label="Queenless Alerts">
          <Switch value={queenlessAlerts} onValueChange={setQueenlessAlerts}
            trackColor={{ false: '#333', true: '#DC3545' }} />
        </SettingRow>
      </View>

      {/* LoRaWAN section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>📡 LoRaWAN Configuration</Text>
        <SettingRow label="DevEUI">
          <TextInput
            style={styles.input}
            value={loraDevEui}
            onChangeText={setLoraDevEui}
          />
        </SettingRow>
        <SettingRow label="AppKey">
          <TextInput
            style={styles.input}
            value={loraAppKey}
            onChangeText={setLoraAppKey}
            secureTextEntry
            placeholder="2B7E151628AED2A6..."
          />
        </SettingRow>
      </View>

      {/* Calibration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>⚖️ Weight Calibration</Text>
        <TouchableOpacity style={styles.calibButton} onPress={handleTare}>
          <Text style={styles.calibButtonText}>Tare All Load Cells</Text>
        </TouchableOpacity>
      </View>

      {/* OTA Update */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>📦 Firmware Update</Text>
        <TouchableOpacity
          style={styles.otaButton}
          onPress={() => Alert.alert('OTA Update', 'Check for firmware updates via BLE?')}
        >
          <Text style={styles.otaButtonText}>Check for Updates</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity style={styles.saveButton} onPress={handleSaveSettings}>
        <Text style={styles.saveButtonText}>Save Settings</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>HivePulse v1.0 · Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#E8A317', padding: 16 },
  section: { backgroundColor: '#1A1A2E', margin: 8, borderRadius: 12, padding: 14 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#E8A317', marginBottom: 10 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 10 },
  settingLabel: { fontSize: 14, color: '#CCC', flex: 1 },
  input: {
    width: 100, backgroundColor: '#0F0F1E', borderRadius: 6, paddingHorizontal: 10,
    paddingVertical: 6, color: '#FFF', fontSize: 14, borderWidth: 1, borderColor: '#333',
  },
  scanButton: { backgroundColor: '#E8A317', borderRadius: 8, padding: 12, alignItems: 'center' },
  scanButtonText: { color: '#1A1A2E', fontWeight: 'bold', fontSize: 14 },
  deviceRow: { flexDirection: 'row', alignItems: 'center', marginTop: 10 },
  deviceIcon: { fontSize: 16, marginRight: 8 },
  deviceName: { flex: 1, fontSize: 14, color: '#FFF' },
  deviceStatus: { fontSize: 12, color: '#4CAF50' },
  calibButton: { backgroundColor: '#333', borderRadius: 8, padding: 12, alignItems: 'center' },
  calibButtonText: { color: '#FFF', fontSize: 14 },
  otaButton: { backgroundColor: '#03A9F4', borderRadius: 8, padding: 12, alignItems: 'center' },
  otaButtonText: { color: '#FFF', fontWeight: 'bold', fontSize: 14 },
  saveButton: {
    backgroundColor: '#4CAF50', borderRadius: 8, padding: 14,
    margin: 16, alignItems: 'center',
  },
  saveButtonText: { color: '#FFF', fontWeight: 'bold', fontSize: 16 },
  footer: { fontSize: 11, color: '#444', textAlign: 'center', paddingBottom: 20 },
});