// ============================================================================
// screens/SettingsScreen.js — App & controller settings
// ============================================================================
import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Switch } from 'react-native';

export default function SettingsScreen() {
  const [canNodeId, setCanNodeId] = useState(1);
  const [autoConnect, setAutoConnect] = useState(true);
  const [darkMode, setDarkMode] = useState(true);
  const [telemetryRate, setTelemetryRate] = useState(10);
  const [units, setUnits] = useState('metric');
  const [fwVersion, setFwVersion] = useState('1.0.0');

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Settings</Text>
      </View>

      {/* BLE Settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Bluetooth</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Auto-Connect</Text>
          <Switch value={autoConnect} onValueChange={setAutoConnect} trackColor={{ true: '#FF6B35' }} />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Telemetry Rate (Hz)</Text>
          <View style={styles.rateButtons}>
            {[1, 5, 10, 50].map(rate => (
              <TouchableOpacity
                key={rate}
                style={[styles.rateBtn, telemetryRate === rate && styles.rateBtnActive]}
                onPress={() => setTelemetryRate(rate)}
              >
                <Text style={styles.rateBtnText}>{rate}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      {/* CAN Settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>CAN Bus</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Node ID</Text>
          <View style={styles.nodeIdButtons}>
            {[1, 2, 3, 4].map(id => (
              <TouchableOpacity
                key={id}
                style={[styles.nodeBtn, canNodeId === id && styles.nodeBtnActive]}
                onPress={() => setCanNodeId(id)}
              >
                <Text style={styles.nodeBtnText}>{id}</Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>

      {/* Display Settings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Display</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Dark Mode</Text>
          <Switch value={darkMode} onValueChange={setDarkMode} trackColor={{ true: '#FF6B35' }} />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Units</Text>
          <View style={styles.unitButtons}>
            <TouchableOpacity
              style={[styles.unitBtn, units === 'metric' && styles.unitBtnActive]}
              onPress={() => setUnits('metric')}
            >
              <Text style={styles.unitBtnText}>Metric</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.unitBtn, units === 'imperial' && styles.unitBtnActive]}
              onPress={() => setUnits('imperial')}
            >
              <Text style={styles.unitBtnText}>Imperial</Text>
            </TouchableOpacity>
          </View>
        </View>
      </View>

      {/* About */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>About</Text>
        <Text style={styles.aboutText}>RideCore App v1.0.0</Text>
        <Text style={styles.aboutText}>Controller FW: v{fwVersion}</Text>
        <Text style={styles.aboutText}>Protocol: v1 (CRC16-CCITT)</Text>
        <Text style={styles.aboutText}>License: MIT</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F23' },
  header: { padding: 20, alignItems: 'center' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#FF6B35' },
  card: { margin: 10, padding: 16, backgroundColor: '#1A1A2E', borderRadius: 12 },
  cardTitle: { fontSize: 16, fontWeight: '600', color: '#FFF', marginBottom: 12 },
  settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginVertical: 8 },
  settingLabel: { color: '#CCC', fontSize: 14 },
  rateButtons: { flexDirection: 'row' },
  rateBtn: { padding: 8, marginHorizontal: 4, backgroundColor: '#2A2A4A', borderRadius: 6 },
  rateBtnActive: { backgroundColor: '#FF6B35' },
  rateBtnText: { color: '#FFF', fontSize: 12, fontWeight: '600' },
  nodeIdButtons: { flexDirection: 'row' },
  nodeBtn: { padding: 10, marginHorizontal: 4, backgroundColor: '#2A2A4A', borderRadius: 6, width: 40, alignItems: 'center' },
  nodeBtnActive: { backgroundColor: '#2196F3' },
  nodeBtnText: { color: '#FFF', fontWeight: '700' },
  unitButtons: { flexDirection: 'row' },
  unitBtn: { padding: 10, marginHorizontal: 4, backgroundColor: '#2A2A4A', borderRadius: 6 },
  unitBtnActive: { backgroundColor: '#FF6B35' },
  unitBtnText: { color: '#FFF', fontSize: 12, fontWeight: '600' },
  aboutText: { color: '#888', fontSize: 13, marginVertical: 4 },
});