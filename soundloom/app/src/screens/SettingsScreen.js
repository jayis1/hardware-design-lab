/**
 * SettingsScreen.js — App settings and device configuration
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Switch, ScrollView, Alert, Share } from 'react-native';
import { useSelector, useDispatch } from 'react-redux';
import BluetoothService from '../services/BluetoothService';

export default function SettingsScreen({ navigation }) {
  const settings = useSelector(state => state.settings);
  const dispatch = useDispatch();
  const [scanning, setScanning] = useState(false);

  const updateSetting = (key, value) => {
    dispatch({ type: 'SETTINGS_UPDATE', payload: { [key]: value } });
  };

  const handleExport = async () => {
    try {
      const data = JSON.stringify({ pods: [], events: [], settings }, null, 2);
      await Share.share({ message: data, title: 'SoundLoom Data Export' });
    } catch (e) {
      Alert.alert('Export failed', e.message);
    }
  };

  const handleFirmwareUpdate = async () => {
    Alert.alert(
      'Firmware Update',
      'This will put the connected pod into OTA mode and update the classifier model via BLE. Continue?',
      [
        { text: 'Cancel' },
        { text: 'OK', onPress: async () => {
          try {
            await BluetoothService.sendCommand(0x07); // CMD_START_OTA
            Alert.alert('OTA Mode', 'Pod is now in OTA mode. Transfer the model file via BLE.');
          } catch (e) {
            Alert.alert('Error', e.message);
          }
        }}
      ]
    );
  };

  const handleReboot = async () => {
    Alert.alert('Reboot Pod', 'Are you sure?', [
      { text: 'Cancel' },
      { text: 'Reboot', onPress: async () => {
        try { await BluetoothService.reboot(); } catch (e) { /* ignore */ }
      }}
    ]);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Settings</Text>
        <Text style={styles.headerSub}>SoundLoom v1.0 — Author: jayis1</Text>
      </View>

      {/* Measurement settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Measurement</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Cadence (minutes)</Text>
          <TextInput
            style={styles.settingInput}
            value={String(settings.cadence)}
            onChangeText={v => updateSetting('cadence', parseInt(v) || 30)}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Listen window (seconds)</Text>
          <TextInput
            style={styles.settingInput}
            value={String(settings.listenWindow)}
            onChangeText={v => updateSetting('listenWindow', parseInt(v) || 30)}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Sensitivity (σ)</Text>
          <TextInput
            style={styles.settingInput}
            value={String(settings.sensitivity)}
            onChangeText={v => updateSetting('sensitivity', parseFloat(v) || 3.0)}
            keyboardType="numeric"
          />
        </View>
      </View>

      {/* LoRaWAN settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>LoRaWAN</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Region</Text>
          <View style={styles.regionPicker}>
            {['EU868', 'US915', 'AU915', 'AS923'].map(r => (
              <TouchableOpacity
                key={r}
                style={[styles.regionBtn, settings.loraRegion === r && styles.regionBtnActive]}
                onPress={() => updateSetting('loraRegion', r)}
              >
                <Text style={[styles.regionBtnText, settings.loraRegion === r && styles.regionBtnTextActive]}>{r}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>DevEUI</Text>
          <TextInput
            style={styles.settingInput}
            value={settings.loraDevEUI}
            onChangeText={v => updateSetting('loraDevEUI', v)}
            placeholder="0000000000000000"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>AppKey</Text>
          <TextInput
            style={styles.settingInput}
            value={settings.loraAppKey}
            onChangeText={v => updateSetting('loraAppKey', v)}
            placeholder="00000000000000000000000000000000"
            secureTextEntry
          />
        </View>
      </View>

      {/* Display settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Units</Text>
          <View style={styles.regionPicker}>
            {['metric', 'imperial'].map(u => (
              <TouchableOpacity
                key={u}
                style={[styles.regionBtn, settings.units === u && styles.regionBtnActive]}
                onPress={() => updateSetting('units', u)}
              >
                <Text style={[styles.regionBtnText, settings.units === u && styles.regionBtnTextActive]}>{u}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Theme</Text>
          <View style={styles.regionPicker}>
            {['light', 'dark'].map(t => (
              <TouchableOpacity
                key={t}
                style={[styles.regionBtn, settings.theme === t && styles.regionBtnActive]}
                onPress={() => updateSetting('theme', t)}
              >
                <Text style={[styles.regionBtnText, settings.theme === t && styles.regionBtnTextActive]}>{t}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      {/* Device actions */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Device</Text>
        <TouchableOpacity style={styles.actionButton} onPress={handleFirmwareUpdate}>
          <Text style={styles.actionButtonText}>Update Classifier Model (OTA)</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionButton, { backgroundColor: '#FF9800' }]} onPress={handleReboot}>
          <Text style={styles.actionButtonText}>Reboot Connected Pod</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionButton, { backgroundColor: '#757575' }]} onPress={handleExport}>
          <Text style={styles.actionButtonText}>Export Data (CSV/JSON)</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>SoundLoom — Bioacoustic Soil Health Monitor</Text>
        <Text style={styles.aboutText}>Version: 1.0.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright © 2026 jayis1</Text>
        <Text style={styles.aboutText}>License: MIT</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 20, backgroundColor: '#2E7D32', alignItems: 'center' },
  headerTitle: { fontSize: 22, fontWeight: 'bold', color: '#FFFFFF' },
  headerSub: { fontSize: 12, color: '#E8F5E9', marginTop: 4 },
  section: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12, elevation: 1 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#2E7D32', marginBottom: 10 },
  settingRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', paddingVertical: 8, borderBottomWidth: 0.5, borderBottomColor: '#F0F0F0' },
  settingLabel: { fontSize: 14, color: '#424242', flex: 1 },
  settingInput: { borderWidth: 1, borderColor: '#E0E0E0', borderRadius: 6, padding: 8, fontSize: 14, width: 100, textAlign: 'center' },
  regionPicker: { flexDirection: 'row', flexWrap: 'wrap' },
  regionBtn: { paddingHorizontal: 8, paddingVertical: 5, margin: 2, borderRadius: 6, backgroundColor: '#F0F0F0' },
  regionBtnActive: { backgroundColor: '#2E7D32' },
  regionBtnText: { fontSize: 11, color: '#757575' },
  regionBtnTextActive: { color: '#FFFFFF', fontWeight: 'bold' },
  actionButton: { backgroundColor: '#2E7D32', padding: 12, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  actionButtonText: { color: '#FFFFFF', fontSize: 14, fontWeight: 'bold' },
  aboutText: { fontSize: 12, color: '#616161', marginBottom: 3 },
});