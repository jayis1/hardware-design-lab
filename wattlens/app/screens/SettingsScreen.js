/**
 * SettingsScreen.js — Device configuration and calibration
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, ScrollView, StyleSheet,
  TouchableOpacity, TextInput, Alert, Picker,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { Commands, APPLIANCE_NAMES } from '../utils/protocol';

export default function SettingsScreen() {
  const { status, sendCommand, connectionState, reconnect } = useBle();
  const [ctRatio, setCtRatio] = useState('100');
  const [ctChannel, setCtChannel] = useState(0);
  const [gridFreq, setGridFreq] = useState(50);
  const [selectedAppliance, setSelectedAppliance] = useState(0);
  const [applianceName, setApplianceName] = useState('');

  const handleSetCt = () => {
    const ratio = parseFloat(ctRatio);
    if (isNaN(ratio) || ratio <= 0) {
      Alert.alert('Invalid CT Ratio', 'Please enter a valid numeric ratio (e.g., 100)');
      return;
    }
    if (sendCommand) sendCommand(Commands.SET_CT(ctChannel, ratio));
    Alert.alert('CT Ratio Set', `Channel ${ctChannel + 1}: ${ratio} A`);
  };

  const handleSetGrid = () => {
    if (sendCommand) sendCommand(Commands.SET_GRID(gridFreq));
    Alert.alert('Grid Frequency Set', `${gridFreq} Hz`);
  };

  const handleCalibrate = () => {
    Alert.alert(
      'Calibration',
      'Apply a known reference voltage (e.g., 230 V) and a known resistive load (e.g., 1000 W).\n\nContinue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Start', onPress: () => sendCommand && sendCommand(Commands.CALIBRATE()) },
      ]
    );
  };

  const handleSetName = () => {
    if (!applianceName.trim()) {
      Alert.alert('Enter Name', 'Please enter a name for the appliance');
      return;
    }
    if (sendCommand) sendCommand(Commands.SET_NAME(selectedAppliance, applianceName));
    Alert.alert('Name Set', `Class ${selectedAppliance}: ${applianceName}`);
    setApplianceName('');
  };

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Settings</Text>

      {/* Connection status */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <Text style={styles.statusText}>
          State: {connectionState}
        </Text>
        {status && (
          <>
            <Text style={styles.detailText}>Uptime: {formatUptime(status.uptime)}</Text>
            <Text style={styles.detailText}>Battery: {status.battery}%</Text>
            <Text style={styles.detailText}>SD Card: {status.sdPresent ? 'Present ✓' : 'Not detected ✗'}</Text>
            <Text style={styles.detailText}>LoRa Mesh: {status.loraJoined ? 'Joined ✓' : 'Not joined'}</Text>
            <Text style={styles.detailText}>Samples: {status.sampleCount.toLocaleString()}</Text>
            <Text style={styles.detailText}>NILM Classes: {status.numClasses}</Text>
          </>
        )}
        <TouchableOpacity style={styles.button} onPress={reconnect}>
          <Text style={styles.buttonText}>Reconnect</Text>
        </TouchableOpacity>
      </View>

      {/* CT Configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>CT Configuration</Text>
        <Text style={styles.hint}>Set the turns ratio for each current transformer channel</Text>

        <Text style={styles.label}>Channel</Text>
        <View style={styles.pickerRow}>
          {[0, 1, 2, 3].map((ch) => (
            <TouchableOpacity
              key={ch}
              style={[styles.chBtn, ctChannel === ch && styles.chBtnActive]}
              onPress={() => setCtChannel(ch)}
            >
              <Text style={[styles.chBtnText, ctChannel === ch && styles.chBtnTextActive]}>
                {ch < 3 ? `L${ch + 1}` : 'N'}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.label}>CT Ratio (A)</Text>
        <TextInput
          style={styles.input}
          value={ctRatio}
          onChangeText={setCtRatio}
          keyboardType="numeric"
          placeholder="e.g., 100"
          placeholderTextColor="#555"
        />
        <TouchableOpacity style={styles.button} onPress={handleSetCt}>
          <Text style={styles.buttonText}>Apply CT Ratio</Text>
        </TouchableOpacity>
      </View>

      {/* Grid Frequency */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Grid Configuration</Text>
        <Text style={styles.label}>Nominal Frequency</Text>
        <View style={styles.pickerRow}>
          {[50, 60].map((f) => (
            <TouchableOpacity
              key={f}
              style={[styles.chBtn, gridFreq === f && styles.chBtnActive]}
              onPress={() => setGridFreq(f)}
            >
              <Text style={[styles.chBtnText, gridFreq === f && styles.chBtnTextActive]}>
                {f} Hz
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <TouchableOpacity style={styles.button} onPress={handleSetGrid}>
          <Text style={styles.buttonText}>Apply Grid Setting</Text>
        </TouchableOpacity>
      </View>

      {/* Calibration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Calibration</Text>
        <Text style={styles.hint}>
          Field-calibrate voltage and current channels using a known reference.
          Ensure the device is connected to a stable reference source before
          starting calibration.
        </Text>
        <TouchableOpacity style={[styles.button, styles.calButton]} onPress={handleCalibrate}>
          <Text style={styles.buttonText}>Start Calibration Wizard</Text>
        </TouchableOpacity>
      </View>

      {/* NILM Appliance Names */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>NILM Appliance Names</Text>
        <Text style={styles.hint}>Customize appliance class names for your installation</Text>

        <Text style={styles.label}>Appliance Class</Text>
        <Picker
          selectedValue={selectedAppliance}
          style={styles.picker}
          onValueChange={(v) => setSelectedAppliance(v)}
          itemStyle={{ color: '#FFF' }}
        >
          {APPLIANCE_NAMES.map((name, i) => (
            <Picker.Item key={i} label={`Class ${i}: ${name}`} value={i} />
          ))}
        </Picker>

        <Text style={styles.label}>New Name</Text>
        <TextInput
          style={styles.input}
          value={applianceName}
          onChangeText={setApplianceName}
          placeholder="e.g., Heat Pump"
          placeholderTextColor="#555"
          maxLength={22}
        />
        <TouchableOpacity style={styles.button} onPress={handleSetName}>
          <Text style={styles.buttonText}>Set Appliance Name</Text>
        </TouchableOpacity>
      </View>

      {/* About */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.aboutText}>WattLens v1.0</Text>
        <Text style={styles.aboutText}>3-Phase Power Quality Analyzer & NILM</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright © 2026 jayis1</Text>
        <Text style={styles.aboutText}>MIT License</Text>
        <Text style={styles.aboutHint}>
          Open-source hardware + firmware + app.
          Schematics, PCB, and full firmware source available.
        </Text>
      </View>
    </ScrollView>
  );
}

function formatUptime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return `${h}h ${m}m ${s}s`;
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: '#0a0a14' },
  content: { padding: 16, paddingBottom: 40 },
  title: { color: '#FFF', fontSize: 24, fontWeight: '700', textAlign: 'center', marginBottom: 16 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 16, marginBottom: 12 },
  cardTitle: { color: '#0080FF', fontSize: 16, fontWeight: '700', marginBottom: 8 },
  hint: { color: '#777', fontSize: 12, marginBottom: 12, lineHeight: 18 },
  label: { color: '#AAA', fontSize: 13, fontWeight: '600', marginBottom: 6, marginTop: 8 },
  input: {
    backgroundColor: '#0a0a14', borderWidth: 1, borderColor: '#333', borderRadius: 6,
    paddingHorizontal: 12, paddingVertical: 8, color: '#FFF', fontSize: 15, marginBottom: 12,
  },
  picker: { backgroundColor: '#0a0a14', color: '#FFF', marginBottom: 12, height: 44 },
  pickerRow: { flexDirection: 'row', marginBottom: 12 },
  chBtn: { flex: 1, paddingVertical: 8, marginHorizontal: 4, borderRadius: 6, backgroundColor: '#222', alignItems: 'center' },
  chBtnActive: { backgroundColor: '#0080FF' },
  chBtnText: { color: '#888', fontWeight: '600' },
  chBtnTextActive: { color: '#FFF' },
  button: {
    backgroundColor: '#0080FF', paddingVertical: 12, borderRadius: 8, alignItems: 'center',
  },
  calButton: { backgroundColor: '#AA6600' },
  buttonText: { color: '#FFF', fontWeight: '700', fontSize: 15 },
  statusText: { color: '#CCC', fontSize: 14, marginBottom: 4 },
  detailText: { color: '#999', fontSize: 13, marginBottom: 2 },
  aboutText: { color: '#CCC', fontSize: 13, marginBottom: 2 },
  aboutHint: { color: '#666', fontSize: 11, marginTop: 8, lineHeight: 16 },
});