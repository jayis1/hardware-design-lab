/**
 * Settings.tsx — pen-up dwell, vibration, hand mode, calibration, OTA
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Switch, Slider, TouchableOpacity, Alert } from 'react-native';

export function SettingsScreen() {
  const [dwell, setDwell] = useState(180);
  const [vibration, setVibration] = useState(60);
  const [leftHand, setLeftHand] = useState(false);
  const [autoSleep, setAutoSleep] = useState(true);

  const calibrate = () => {
    Alert.alert(
      'Calibration',
      'Place the band flat and still for 5 seconds to zero the gyro. Tap OK when ready.',
      [{ text: 'OK', onPress: () => console.log('calibration started') }]
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Pen-up dwell (ms)</Text>
        <Slider minimumValue={100} maximumValue={400} step={10} value={dwell} onSlidingComplete={setDwell} />
        <Text style={styles.value}>{dwell} ms</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Vibration strength</Text>
        <Slider minimumValue={0} maximumValue={100} step={5} value={vibration} onSlidingComplete={setVibration} />
        <Text style={styles.value}>{vibration}%</Text>
      </View>

      <View style={styles.row}>
        <Text style={styles.label}>Left-hand mode</Text>
        <Switch value={leftHand} onValueChange={setLeftHand} trackColor={{ false: '#333', true: '#2563eb' }} thumbColor="#fff" />
      </View>

      <View style={styles.row}>
        <Text style={styles.label}>Auto-sleep after 2 s idle</Text>
        <Switch value={autoSleep} onValueChange={setAutoSleep} trackColor={{ false: '#333', true: '#2563eb' }} thumbColor="#fff" />
      </View>

      <TouchableOpacity style={styles.btn} onPress={calibrate}>
        <Text style={styles.btnText}>Calibrate Gyro</Text>
      </TouchableOpacity>

      <TouchableOpacity style={styles.btn} onPress={() => Alert.alert('OTA', 'Firmware update via BLE DFU (placeholder).')}>
        <Text style={styles.btnText}>Check for Firmware Update</Text>
      </TouchableOpacity>

      <Text style={styles.foot}>GlyphFlow · © 2026 jayis1 · MIT License</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#fff', fontSize: 20, fontWeight: '600', marginBottom: 16 },
  section: { marginBottom: 18 },
  sectionTitle: { color: '#aaa', fontSize: 13, marginBottom: 6 },
  value: { color: '#7cc4ff', fontSize: 12, fontFamily: 'monospace', marginTop: 2 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 12, borderBottomColor: '#1a1a26', borderBottomWidth: 1 },
  label: { color: '#e8e8f0', fontSize: 15 },
  btn: { backgroundColor: '#1a1a26', padding: 14, borderRadius: 10, alignItems: 'center', marginTop: 10 },
  btnText: { color: '#7cc4ff', fontSize: 15 },
  foot: { color: '#444', fontSize: 11, textAlign: 'center', marginTop: 24 },
});