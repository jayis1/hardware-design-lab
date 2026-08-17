// ============================================================
// LignoScan App — Settings Screen
// Configuration for scan parameters, thresholds, units
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { useState, useEffect } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView, Switch, AsyncStorage, Alert,
} from 'react-native';
import { useBle } from '../utils/BleContext';

export default function SettingsScreen({ navigation }) {
  const { sensorCount, setSensorCount, connected, disconnect, batteryLevel } = useBle();

  const [settings, setSettings] = useState({
    treeId: '',
    treeSpecies: '',
    trunkDiameter: '',
    scanHeight: '130',
    units: 'metric',
    tomoIterations: '50',
    soundWoodThreshold: '2500',
    moderateDecayThreshold: '1500',
    autoSave: true,
    autoGps: true,
    rawDataLogging: true,
  });

  useEffect(() => {
    loadSettings();
  }, []);

  const loadSettings = async () => {
    try {
      const stored = await AsyncStorage.getItem('lignoscan_settings');
      if (stored) {
        setSettings({ ...settings, ...JSON.parse(stored) });
      }
    } catch (e) {
      console.error('Failed to load settings', e);
    }
  };

  const saveSettings = async () => {
    try {
      await AsyncStorage.setItem('lignoscan_settings', JSON.stringify(settings));
      Alert.alert('Saved', 'Settings have been saved.');
    } catch (e) {
      Alert.alert('Error', 'Could not save settings.');
    }
  };

  const updateSetting = (key, value) => {
    setSettings({ ...settings, [key]: value });
  };

  const handleSensorCountChange = (delta) => {
    const newCount = Math.max(8, Math.min(16, sensorCount + delta));
    setSensorCount(newCount);
  };

  const handleDisconnect = () => {
    Alert.alert(
      'Disconnect',
      'Disconnect from LignoScan device?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Disconnect', onPress: () => disconnect() },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* Device Status */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Device</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Connection</Text>
          <Text style={[styles.settingValue, { color: connected ? '#2d8a2d' : '#999' }]}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Battery</Text>
          <Text style={styles.settingValue}>{batteryLevel}%</Text>
        </View>
        {connected && (
          <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
            <Text style={styles.disconnectText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Tree Information */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Tree Information</Text>
        <SettingInput
          label="Tree ID"
          value={settings.treeId}
          onChangeText={(v) => updateSetting('treeId', v)}
          placeholder="e.g., OAK-042"
        />
        <SettingInput
          label="Species"
          value={settings.treeSpecies}
          onChangeText={(v) => updateSetting('treeSpecies', v)}
          placeholder="e.g., Quercus robur"
        />
        <SettingInput
          label="Trunk Diameter (cm)"
          value={settings.trunkDiameter}
          onChangeText={(v) => updateSetting('trunkDiameter', v)}
          placeholder="e.g., 45.0"
          keyboardType="numeric"
        />
        <SettingInput
          label="Scan Height (cm)"
          value={settings.scanHeight}
          onChangeText={(v) => updateSetting('scanHeight', v)}
          placeholder="130"
          keyboardType="numeric"
        />
      </View>

      {/* Sensor Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Sensor Configuration</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Sensor Count</Text>
          <View style={styles.stepperRow}>
            <TouchableOpacity
              style={styles.stepperButton}
              onPress={() => handleSensorCountChange(-2)}
            >
              <Text style={styles.stepperText}>−2</Text>
            </TouchableOpacity>
            <Text style={styles.stepperValue}>{sensorCount}</Text>
            <TouchableOpacity
              style={styles.stepperButton}
              onPress={() => handleSensorCountChange(2)}
            >
              <Text style={styles.stepperText}>+2</Text>
            </TouchableOpacity>
          </View>
        </View>
        <Text style={styles.hint}>
          LignoScan supports 8-16 ultrasonic sensors. More sensors = higher resolution.
        </Text>
      </View>

      {/* Reconstruction Settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Reconstruction</Text>
        <SettingInput
          label="SART Iterations"
          value={settings.tomoIterations}
          onChangeText={(v) => updateSetting('tomoIterations', v)}
          placeholder="50"
          keyboardType="numeric"
        />
        <SettingInput
          label="Sound Wood Threshold (m/s)"
          value={settings.soundWoodThreshold}
          onChangeText={(v) => updateSetting('soundWoodThreshold', v)}
          placeholder="2500"
          keyboardType="numeric"
        />
        <SettingInput
          label="Moderate Decay Threshold (m/s)"
          value={settings.moderateDecayThreshold}
          onChangeText={(v) => updateSetting('moderateDecayThreshold', v)}
          placeholder="1500"
          keyboardType="numeric"
        />
        <Text style={styles.hint}>
          Lower thresholds = more sensitive decay detection. Adjust based on wood species.
        </Text>
      </View>

      {/* Data & Privacy */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Data & Storage</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-save scans</Text>
          <Switch
            value={settings.autoSave}
            onValueChange={(v) => updateSetting('autoSave', v)}
            trackColor={{ false: '#ccc', true: '#2d8a2d' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto GPS tagging</Text>
          <Switch
            value={settings.autoGps}
            onValueChange={(v) => updateSetting('autoGps', v)}
            trackColor={{ false: '#ccc', true: '#2d8a2d' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Raw data logging (SD)</Text>
          <Switch
            value={settings.rawDataLogging}
            onValueChange={(v) => updateSetting('rawDataLogging', v)}
            trackColor={{ false: '#ccc', true: '#2d8a2d' }}
          />
        </View>
      </View>

      {/* Units */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Units</Text>
        <View style={styles.unitRow}>
          <TouchableOpacity
            style={[styles.unitButton, settings.units === 'metric' && styles.unitButtonActive]}
            onPress={() => updateSetting('units', 'metric')}
          >
            <Text style={[styles.unitText, settings.units === 'metric' && styles.unitTextActive]}>
              Metric (cm, m/s)
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.unitButton, settings.units === 'imperial' && styles.unitButtonActive]}
            onPress={() => updateSetting('units', 'imperial')}
          >
            <Text style={[styles.unitText, settings.units === 'imperial' && styles.unitTextActive]}>
              Imperial (in, ft/s)
            </Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* About */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.aboutText}>
          LignoScan v1.0{'\n'}
          Portable Acoustic Tomography Scanner{'\n'}
          {'\n'}
          Author: jayis1{'\n'}
          Copyright © 2026 jayis1{'\n'}
          {'\n'}
          Hardware License: CERN-OHL-S v2{'\n'}
          Firmware License: GPL-2.0{'\n'}
          App License: MIT{'\n'}
          {'\n'}
          LignoScan is an open-source tool for non-destructive{'\n'}
          tree decay detection using ultrasonic stress-wave{'\n'}
          tomography. Designed for arborists, urban foresters,{'\n'}
          and tree-risk assessors.
        </Text>
      </View>

      <TouchableOpacity style={styles.saveButton} onPress={saveSettings}>
        <Text style={styles.saveButtonText}>Save Settings</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>
        Author: jayis1 — Copyright © 2026 — MIT License
      </Text>
    </ScrollView>
  );
}

// Reusable setting input component
function SettingInput({ label, value, onChangeText, placeholder, keyboardType }) {
  return (
    <View style={styles.inputRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      <TextInput
        style={styles.textInput}
        value={value}
        onChangeText={onChangeText}
        placeholder={placeholder}
        keyboardType={keyboardType || 'default'}
      />
    </View>
  );
}

// Need to import TextInput
import { TextInput } from 'react-native';

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f0' },
  card: {
    backgroundColor: '#fff',
    margin: 12,
    padding: 16,
    borderRadius: 10,
    elevation: 2,
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#1a3a1a',
    marginBottom: 10,
  },
  settingRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  settingLabel: { fontSize: 14, color: '#333' },
  settingValue: { fontSize: 14, fontWeight: 'bold' },
  disconnectButton: {
    marginTop: 10,
    padding: 10,
    backgroundColor: '#cc0000',
    borderRadius: 6,
    alignItems: 'center',
  },
  disconnectText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  inputRow: {
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  textInput: {
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 6,
    padding: 8,
    marginTop: 4,
    fontSize: 14,
  },
  stepperRow: { flexDirection: 'row', alignItems: 'center' },
  stepperButton: {
    width: 36,
    height: 36,
    borderRadius: 18,
    backgroundColor: '#1a3a1a',
    alignItems: 'center',
    justifyContent: 'center',
  },
  stepperText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  stepperValue: {
    fontSize: 18,
    fontWeight: 'bold',
    marginHorizontal: 16,
    color: '#1a3a1a',
  },
  hint: { fontSize: 12, color: '#888', marginTop: 6, fontStyle: 'italic' },
  unitRow: { flexDirection: 'row', gap: 8 },
  unitButton: {
    flex: 1,
    padding: 10,
    borderRadius: 6,
    backgroundColor: '#e0e0d0',
    alignItems: 'center',
  },
  unitButtonActive: { backgroundColor: '#1a3a1a' },
  unitText: { fontSize: 13, color: '#555' },
  unitTextActive: { color: '#fff', fontWeight: 'bold' },
  aboutText: { fontSize: 13, color: '#555', lineHeight: 20 },
  saveButton: {
    backgroundColor: '#2d8a2d',
    margin: 12,
    padding: 14,
    borderRadius: 8,
    alignItems: 'center',
  },
  saveButtonText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  footer: { textAlign: 'center', fontSize: 11, color: '#aaa', paddingVertical: 12 },
});

// EOF — SettingsScreen.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT