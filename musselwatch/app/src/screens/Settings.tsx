/*
 * screens/Settings.tsx — app configuration
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import React from 'react';
import {
  View, Text, StyleSheet, TextInput, Switch, TouchableOpacity, Alert,
} from 'react-native';

import { AppConfig } from '../types';

export function SettingsScreen({
  config,
  setConfig,
}: {
  config: AppConfig;
  setConfig: (c: AppConfig) => void;
}) {
  const update = (patch: Partial<AppConfig>) => setConfig({ ...config, ...patch });

  const resetToDefaults = () => {
    Alert.alert('Reset settings', 'Restore all defaults?', [
      { text: 'Cancel', style: 'cancel' },
      { text: 'Reset', onPress: () =>
        setConfig({
          gatewayUrl: 'mock://musselwatch',
          pollIntervalS: 30,
          alertThreshold: 50,
          temperatureUnitC: true,
          darkMode: true,
        })},
    ]);
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Settings</Text>
        <Text style={styles.headerSub}>MusselWatch v1.0 · jayis1</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Gateway</Text>
        <Text style={styles.label}>Gateway URL</Text>
        <TextInput
          style={styles.input}
          value={config.gatewayUrl}
          onChangeText={(v) => update({ gatewayUrl: v })}
          placeholder="https://musselwatch.jayis1.dev/api"
          placeholderTextColor="#5a7a8a"
          autoCapitalize="none"
          autoCorrect={false}
        />
        <Text style={styles.hint}>
          Use “mock://…” to run with built-in demo data (no gateway required).
        </Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Polling</Text>
        <Text style={styles.label}>Poll interval (seconds)</Text>
        <TextInput
          style={styles.input}
          value={String(config.pollIntervalS)}
          onChangeText={(v) => update({ pollIntervalS: Math.max(5, parseInt(v) || 30) })}
          keyboardType="numeric"
        />
        <Text style={styles.label}>Alert threshold (anomaly score 0–100)</Text>
        <TextInput
          style={styles.input}
          value={String(config.alertThreshold)}
          onChangeText={(v) => update({ alertThreshold: Math.max(0, Math.min(100, parseInt(v) || 0)) })}
          keyboardType="numeric"
        />
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Display</Text>
        <View style={styles.toggleRow}>
          <Text style={styles.label}>Temperature in Celsius</Text>
          <Switch
            value={config.temperatureUnitC}
            onValueChange={(v) => update({ temperatureUnitC: v })}
            trackColor={{ true: '#0a4d6e' }}
          />
        </View>
        <View style={styles.toggleRow}>
          <Text style={styles.label}>Dark mode</Text>
          <Switch
            value={config.darkMode}
            onValueChange={(v) => update({ darkMode: v })}
            trackColor={{ true: '#0a4d6e' }}
          />
        </View>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About</Text>
        <Text style={styles.aboutText}>
          MusselWatch is an open-source bivalve valvometric biosensor network.
          Freshwater mussels are sentinel organisms: they filter-feed
          continuously and clamp shut within minutes of detecting toxins,
          heavy metals, or pH shifts.  This app displays shell-gape
          telemetry relayed via LoRaWAN from solar-powered sensor nodes
          deployed along rivers, lakes, and industrial discharge points.
        </Text>
        <Text style={styles.aboutText}>
          Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT
        </Text>
        <Text style={styles.aboutText}>Author: jayis1</Text>
        <TouchableOpacity style={styles.resetBtn} onPress={resetToDefaults}>
          <Text style={styles.resetBtnText}>Reset to defaults</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f2c' },
  header: { padding: 16, backgroundColor: '#0a4d6e' },
  headerTitle: { color: '#e0f0f5', fontSize: 18, fontWeight: '700' },
  headerSub: { color: '#a0c5d5', fontSize: 12, marginTop: 2 },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#1c4a66' },
  sectionTitle: { color: '#7fcfff', fontSize: 14, fontWeight: '600', marginBottom: 10 },
  label: { color: '#9fb8c7', fontSize: 13, marginBottom: 4, marginTop: 8 },
  input: {
    backgroundColor: '#0d2b3d', color: '#e0f0f5', fontSize: 14,
    borderRadius: 6, padding: 10, borderWidth: 1, borderColor: '#1c4a66',
  },
  hint: { color: '#6a8a9a', fontSize: 11, marginTop: 4, fontStyle: 'italic' },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginTop: 8 },
  aboutText: { color: '#9fb8c7', fontSize: 12, marginTop: 6, lineHeight: 18 },
  resetBtn: {
    marginTop: 14, padding: 10, borderRadius: 6, backgroundColor: '#3a1208', alignItems: 'center',
  },
  resetBtnText: { color: '#f5b8a0', fontSize: 13, fontWeight: '600' },
});