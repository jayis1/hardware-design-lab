/**
 * SettingsScreen — App and device configuration
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Switch, Alert } from 'react-native';
import { useDeviceStore } from '../store/deviceStore';

export const SettingsScreen: React.FC = () => {
  const {
    settings, updateSettings, config, updateConfig,
    connected, sendSOS,
  } = useDeviceStore();

  const handleUpdateConfig = async (key: string, value: number) => {
    await updateConfig({ [key]: value } as any);
  };

  const handleOTAUpdate = () => {
    Alert.alert(
      'OTA Firmware Update',
      'This will download and install the latest firmware over BLE. The device will restart. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Update', onPress: () => Alert.alert('OTA', 'Firmware update would begin here. In production, this uses nRF Connect Device Firmware Update.') },
      ]
    );
  };

  return (
    <ScrollView style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>Settings</Text>
        <Text style={styles.author}>ThermoGlyph by jayis1</Text>
      </View>

      {/* App Settings */}
      <Text style={styles.sectionTitle}>App Settings</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-reconnect</Text>
          <Switch
            value={settings.autoReconnect}
            onValueChange={(v) => updateSettings({ autoReconnect: v })}
            trackColor={{ false: '#333', true: '#2a6' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-stream navigation</Text>
          <Switch
            value={settings.navigationAutoStream}
            onValueChange={(v) => updateSettings({ navigationAutoStream: v })}
            trackColor={{ false: '#333', true: '#2a6' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Dark mode</Text>
          <Switch
            value={settings.darkMode}
            onValueChange={(v) => updateSettings({ darkMode: v })}
            trackColor={{ false: '#333', true: '#2a6' }}
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>SOS gesture</Text>
          <TouchableOpacity
            style={styles.picker}
            onPress={() => updateSettings({
              emergencySosGesture: settings.emergencySosGesture === 'shake' ? 'double_tap' : 'shake',
            })}
          >
            <Text style={styles.pickerText}>{settings.emergencySosGesture}</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Device Configuration */}
      <Text style={styles.sectionTitle}>Device Configuration</Text>
      <View style={styles.card}>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Intensity Scale</Text>
          <View style={styles.stepper}>
            <TouchableOpacity style={styles.stepBtn} onPress={() => handleUpdateConfig('intensityScale', Math.max(0, config.intensityScale - 16))}>
              <Text style={styles.stepBtnText}>−</Text>
            </TouchableOpacity>
            <Text style={styles.stepValue}>{config.intensityScale}</Text>
            <TouchableOpacity style={styles.stepBtn} onPress={() => handleUpdateConfig('intensityScale', Math.min(255, config.intensityScale + 16))}>
              <Text style={styles.stepBtnText}>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Max Temp ({config.maxTempC}°C)</Text>
          <View style={styles.stepper}>
            <TouchableOpacity style={styles.stepBtn} onPress={() => handleUpdateConfig('maxTempC', Math.max(35, config.maxTempC - 1))}>
              <Text style={styles.stepBtnText}>−</Text>
            </TouchableOpacity>
            <Text style={styles.stepValue}>{config.maxTempC}°</Text>
            <TouchableOpacity style={styles.stepBtn} onPress={() => handleUpdateConfig('maxTempC', Math.min(44, config.maxTempC + 1))}>
              <Text style={styles.stepBtnText}>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>PID Kp</Text>
          <Text style={styles.settingValue}>{config.pidKp}</Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>PID Ki</Text>
          <Text style={styles.settingValue}>{config.pidKi}</Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>PID Kd</Text>
          <Text style={styles.settingValue}>{config.pidKd}</Text>
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Power Mode</Text>
          <TouchableOpacity
            style={styles.picker}
            onPress={() => handleUpdateConfig('powerMode',
              config.powerMode === 'auto' ? 'active' :
              config.powerMode === 'active' ? 'idle' :
              config.powerMode === 'idle' ? 'sleep' : 'auto')}
          >
            <Text style={styles.pickerText}>{config.powerMode}</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Firmware */}
      <Text style={styles.sectionTitle}>Firmware</Text>
      <View style={styles.card}>
        <TouchableOpacity style={styles.fwRow} onPress={handleOTAUpdate}>
          <Text style={styles.fwLabel}>Check for Updates</Text>
          <Text style={styles.fwArrow}>›</Text>
        </TouchableOpacity>
        <View style={styles.fwInfoRow}>
          <Text style={styles.fwInfoLabel}>Version</Text>
          <Text style={styles.fwInfoValue}>1.0.0</Text>
        </View>
        <View style={styles.fwInfoRow}>
          <Text style={styles.fwInfoLabel}>Author</Text>
          <Text style={styles.fwInfoValue}>jayis1</Text>
        </View>
        <View style={styles.fwInfoRow}>
          <Text style={styles.fwInfoLabel}>License</Text>
          <Text style={styles.fwInfoValue}>GPL-3.0</Text>
        </View>
      </View>

      {/* About */}
      <Text style={styles.sectionTitle}>About</Text>
      <View style={styles.card}>
        <Text style={styles.aboutText}>
          ThermoGlyph is a wearable thermal-tactile communication and navigation
          device that renders information through localized temperature changes
          on the skin. Designed for accessibility, emergency services, diving,
          and silent tactical communication.
        </Text>
        <Text style={styles.aboutAuthor}>Author: jayis1</Text>
        <Text style={styles.aboutCopyright}>Copyright (c) 2026 jayis1</Text>
        <Text style={styles.aboutLicense}>Hardware: CERN-OHL-S v2</Text>
        <Text style={styles.aboutLicense}>Firmware: GPL-3.0</Text>
        <Text style={styles.aboutLicense}>App: MIT</Text>
      </View>

      {/* Emergency */}
      <TouchableOpacity style={styles.sosBtn} onPress={() => {
        Alert.alert('SOS', 'Send emergency SOS?', [
          { text: 'Cancel', style: 'cancel' },
          { text: 'Send', style: 'destructive', onPress: () => sendSOS() },
        ]);
      }}>
        <Text style={styles.sosText}>EMERGENCY SOS</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>ThermoGlyph v1.0 · jayis1</Text>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  author: { color: '#888', fontSize: 13, marginTop: 4 },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 8 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 12, padding: 14, marginHorizontal: 8 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: '#222' },
  settingLabel: { color: '#ccc', fontSize: 14 },
  settingValue: { color: '#888', fontSize: 14, fontFamily: 'monospace' },
  stepper: { flexDirection: 'row', alignItems: 'center', gap: 8 },
  stepBtn: { width: 28, height: 28, borderRadius: 6, backgroundColor: '#333', justifyContent: 'center', alignItems: 'center' },
  stepBtnText: { color: '#fff', fontSize: 16 },
  stepValue: { color: '#fff', fontSize: 14, fontFamily: 'monospace', width: 40, textAlign: 'center' },
  picker: { backgroundColor: '#333', paddingHorizontal: 12, paddingVertical: 6, borderRadius: 6 },
  pickerText: { color: '#fff', fontSize: 13, textTransform: 'capitalize' },
  fwRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 12, borderBottomWidth: 1, borderBottomColor: '#222' },
  fwLabel: { color: '#ccc', fontSize: 14 },
  fwArrow: { color: '#666', fontSize: 20 },
  fwInfoRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8 },
  fwInfoLabel: { color: '#888', fontSize: 13 },
  fwInfoValue: { color: '#ccc', fontSize: 13, fontFamily: 'monospace' },
  aboutText: { color: '#aaa', fontSize: 13, lineHeight: 20, marginBottom: 12 },
  aboutAuthor: { color: '#ccc', fontSize: 13, marginTop: 4 },
  aboutCopyright: { color: '#888', fontSize: 12, marginTop: 2 },
  aboutLicense: { color: '#888', fontSize: 12 },
  sosBtn: { backgroundColor: '#a33', paddingVertical: 16, borderRadius: 12, marginHorizontal: 16, marginTop: 16, alignItems: 'center' },
  sosText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  footer: { color: '#555', fontSize: 12, textAlign: 'center', paddingVertical: 24 },
});