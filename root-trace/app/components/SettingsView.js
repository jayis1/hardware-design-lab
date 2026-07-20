/**
 * SettingsView.js — App settings
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Switch, TextInput, TouchableOpacity, Alert } from 'react-native';

const Colors = {
  bg: '#0a1628', card: '#162640', accent: '#3b82f6', text: '#e2e8f0', textDim: '#94a3b8',
};

export function SettingsView({ navigation }) {
  const [autoConnect, setAutoConnect] = useState(true);
  const [darkMode, setDarkMode] = useState(true);
  const [units, setUnits] = useState('metric');
  const [meshRes, setMeshRes] = useState('medium');
  const [lambda, setLambda] = useState('0.01');
  const [iterations, setIterations] = useState('3');
  const [operatorName, setOperatorName] = useState('');
  const [gpsLogging, setGpsLogging] = useState(true);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Connection</Text>
        <SettingToggle label="Auto-connect on launch" value={autoConnect} onChange={setAutoConnect} />
        <SettingToggle label="GPS location logging" value={gpsLogging} onChange={setGpsLogging} />
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Reconstruction</Text>
        <SettingInput label="Regularization λ" value={lambda} onChange={setLambda} keyboard="numeric" />
        <SettingInput label="Gauss-Newton iterations" value={iterations} onChange={setIterations} keyboard="numeric" />
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Mesh resolution</Text>
          <View style={styles.segmentedRow}>
            {['coarse', 'medium', 'fine'].map(r => (
              <TouchableOpacity
                key={r}
                style={[styles.segment, meshRes === r && styles.segmentActive]}
                onPress={() => setMeshRes(r)}>
                <Text style={[styles.segmentText, meshRes === r && styles.segmentTextActive]}>
                  {r.charAt(0).toUpperCase() + r.slice(1)}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Display</Text>
        <SettingToggle label="Dark mode" value={darkMode} onChange={setDarkMode} />
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Units</Text>
          <View style={styles.segmentedRow}>
            {['metric', 'imperial'].map(u => (
              <TouchableOpacity
                key={u}
                style={[styles.segment, units === u && styles.segmentActive]}
                onPress={() => setUnits(u)}>
                <Text style={[styles.segmentText, units === u && styles.segmentTextActive]}>
                  {u.charAt(0).toUpperCase() + u.slice(1)}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Operator</Text>
        <SettingInput label="Operator name" value={operatorName} onChange={setOperatorName} placeholder="Your name" />
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.aboutText}>RootTrace v1.0</Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <Text style={styles.aboutText}>Copyright © 2026 jayis1</Text>
        <Text style={styles.aboutText}>License: MIT (app) · GPL-3.0 (firmware) · CERN-OHL-S v2 (hardware)</Text>
        <TouchableOpacity onPress={() => Alert.alert('Firmware Update', 'Enter DFU mode? This will reset the device.')}>
          <Text style={styles.link}>Firmware Update (DFU mode)</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

function SettingToggle({ label, value, onChange }) {
  return (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      <Switch value={value} onValueChange={onChange} trackColor={{ false: Colors.card, true: Colors.accent }} />
    </View>
  );
}

function SettingInput({ label, value, onChange, keyboard, placeholder }) {
  return (
    <View style={styles.settingRow}>
      <Text style={styles.settingLabel}>{label}</Text>
      <TextInput
        style={styles.input}
        value={value}
        onChangeText={onChange}
        keyboardType={keyboard || 'default'}
        placeholder={placeholder}
        placeholderTextColor={Colors.textDim}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.bg, padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: Colors.text, marginBottom: 16 },
  card: { backgroundColor: Colors.card, borderRadius: 12, padding: 16, marginBottom: 12 },
  cardTitle: { fontSize: 16, color: Colors.text, fontWeight: 'bold', marginBottom: 12 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  settingLabel: { fontSize: 14, color: Colors.text, flex: 1 },
  input: { backgroundColor: Colors.bg, borderWidth: 1, borderColor: Colors.textDim, borderRadius: 6, padding: 8, color: Colors.text, width: 100, fontSize: 13 },
  segmentedRow: { flexDirection: 'row' },
  segment: { backgroundColor: Colors.bg, padding: 6, borderRadius: 6, marginHorizontal: 2 },
  segmentActive: { backgroundColor: Colors.accent },
  segmentText: { fontSize: 11, color: Colors.textDim },
  segmentTextActive: { color: Colors.text, fontWeight: 'bold' },
  aboutText: { fontSize: 12, color: Colors.textDim, lineHeight: 18 },
  link: { fontSize: 13, color: Colors.accent, marginTop: 8 },
});