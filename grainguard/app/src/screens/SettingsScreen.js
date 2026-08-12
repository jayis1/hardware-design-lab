/**
 * @file    SettingsScreen.js
 * @brief   Probe configuration: grain type, thresholds, intervals, OTA.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState } from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity, TextInput, Switch, Alert } from 'react-native';
import { useGrainGuard } from '../services/GrainGuardContext';

const GRAIN_TYPES = [
  { id: 1, name: 'Wheat',   safeMc: 13.5 },
  { id: 2, name: 'Corn',    safeMc: 15.5 },
  { id: 3, name: 'Barley',  safeMc: 14.0 },
  { id: 4, name: 'Rice',    safeMc: 13.0 },
  { id: 5, name: 'Oats',    safeMc: 14.0 },
  { id: 6, name: 'Soybean', safeMc: 13.0 },
];

export default function SettingsScreen() {
  const { probeList, configureProbe, connected } = useGrainGuard();
  const [selectedSerial, setSelectedSerial] = useState(null);
  const [grainType, setGrainType] = useState(1);
  const [cautionThresh, setCautionThresh] = useState('40');
  const [criticalThresh, setCriticalThresh] = useState('70');
  const [measInterval, setMeasInterval] = useState('15');
  const [acousticInterval, setAcousticInterval] = useState('360');
  const [autoAlerts, setAutoAlerts] = useState(true);
  const [pushNotifs, setPushNotifs] = useState(true);

  const selectedProbe = probeList.find(p => p.serial === selectedSerial);

  const handleApply = () => {
    if (!selectedSerial) {
      Alert.alert('No probe selected', 'Please select a probe to configure.');
      return;
    }
    if (!connected) {
      Alert.alert('Not connected', 'Cannot configure — gateway offline.');
      return;
    }
    configureProbe(selectedSerial, {
      grainType,
      cautionThreshold: parseInt(cautionThresh, 10),
      criticalThreshold: parseInt(criticalThresh, 10),
      measIntervalMin: parseInt(measInterval, 10),
      acousticIntervalMin: parseInt(acousticInterval, 10),
    });
    Alert.alert('Configuration sent', `Settings pushed to probe #${selectedSerial}`);
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Settings</Text>
      </View>

      {/* Probe selector */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Select Probe</Text>
        {probeList.length === 0 ? (
          <Text style={styles.hintText}>No probes available. Commission via NFC first.</Text>
        ) : (
          <ScrollView horizontal style={styles.probeScroll} showsHorizontalScrollIndicator={false}>
            {probeList.map(p => (
              <TouchableOpacity
                key={p.serial}
                style={[
                  styles.probeChip,
                  selectedSerial === p.serial && styles.probeChipActive,
                ]}
                onPress={() => {
                  setSelectedSerial(p.serial);
                  setGrainType(p.grainType || 1);
                  setCautionThresh(String(p.cautionThreshold || 40));
                  setCriticalThresh(String(p.criticalThreshold || 70));
                }}
              >
                <Text style={selectedSerial === p.serial ? styles.chipTextActive : styles.chipText}>
                  #{p.serial} · {p.siloName || `Silo ${p.serial}`}
                </Text>
              </TouchableOpacity>
            ))}
          </ScrollView>
        )}
      </View>

      {/* Grain type selector */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Grain Type</Text>
        <View style={styles.grainGrid}>
          {GRAIN_TYPES.map(g => (
            <TouchableOpacity
              key={g.id}
              style={[
                styles.grainChip,
                grainType === g.id && styles.grainChipActive,
              ]}
              onPress={() => setGrainType(g.id)}
            >
              <Text style={grainType === g.id ? styles.grainTextActive : styles.grainText}>
                {g.name}
              </Text>
              <Text style={styles.grainSub}>Safe ≤ {g.safeMc}%</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Thresholds */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Alert Thresholds</Text>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Caution SRI</Text>
          <TextInput
            style={styles.input}
            value={cautionThresh}
            onChangeText={setCautionThresh}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Critical SRI</Text>
          <TextInput
            style={styles.input}
            value={criticalThresh}
            onChangeText={setCriticalThresh}
            keyboardType="numeric"
          />
        </View>
      </View>

      {/* Measurement intervals */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Measurement Intervals (minutes)</Text>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>T/RH/CO₂</Text>
          <TextInput
            style={styles.input}
            value={measInterval}
            onChangeText={setMeasInterval}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.inputRow}>
          <Text style={styles.inputLabel}>Acoustic Scan</Text>
          <TextInput
            style={styles.input}
            value={acousticInterval}
            onChangeText={setAcousticInterval}
            keyboardType="numeric"
          />
        </View>
        <Text style={styles.hintText}>
          Acoustic scan runs for 5 minutes at the specified interval.
          Lower intervals = shorter battery life.
        </Text>
      </View>

      {/* Notification settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Notifications</Text>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Auto-generate alerts</Text>
          <Switch value={autoAlerts} onValueChange={setAutoAlerts} />
        </View>
        <View style={styles.toggleRow}>
          <Text style={styles.toggleLabel}>Push notifications</Text>
          <Switch value={pushNotifs} onValueChange={setPushNotifs} />
        </View>
      </View>

      {/* Battery info for selected probe */}
      {selectedProbe && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Probe Status</Text>
          <Text style={styles.statusLine}>Serial: {selectedProbe.serial}</Text>
          <Text style={styles.statusLine}>
            Battery: {selectedProbe.batteryMv || '—'} mV
          </Text>
          <Text style={styles.statusLine}>
            Last update: {selectedProbe.lastUpdate ? new Date(selectedProbe.lastUpdate).toLocaleString() : 'never'}
          </Text>
          <Text style={styles.statusLine}>Firmware: v1.0.0</Text>
          <Text style={styles.statusLine}>Author: jayis1</Text>
        </View>
      )}

      {/* Apply button */}
      <TouchableOpacity style={styles.applyBtn} onPress={handleApply}>
        <Text style={styles.applyText}>Apply Configuration</Text>
      </TouchableOpacity>

      <View style={styles.footer}>
        <Text style={styles.footerText}>GrainGuard v1.0.0</Text>
        <Text style={styles.footerText}>Author: jayis1 · © 2026</Text>
        <Text style={styles.footerText}>License: MIT</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { paddingHorizontal: 16, paddingVertical: 12, backgroundColor: '#1B5E20' },
  headerTitle: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  section: { backgroundColor: '#fff', padding: 16, marginTop: 8 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 12 },
  probeScroll: { flexDirection: 'row' },
  probeChip: {
    paddingHorizontal: 16, paddingVertical: 10, marginRight: 8,
    borderRadius: 20, borderWidth: 1, borderColor: '#E0E0E0',
    backgroundColor: '#FAFAFA',
  },
  probeChipActive: { backgroundColor: '#1B5E20', borderColor: '#1B5E20' },
  chipText: { fontSize: 13, color: '#616161' },
  chipTextActive: { fontSize: 13, color: '#fff', fontWeight: 'bold' },
  grainGrid: { flexDirection: 'row', flexWrap: 'wrap' },
  grainChip: {
    width: '31%', margin: '1%', padding: 12, borderRadius: 8,
    borderWidth: 1, borderColor: '#E0E0E0', alignItems: 'center',
  },
  grainChipActive: { backgroundColor: '#E8F5E9', borderColor: '#2E7D32' },
  grainText: { fontSize: 14, color: '#616161', fontWeight: '600' },
  grainTextActive: { fontSize: 14, color: '#2E7D32', fontWeight: 'bold' },
  grainSub: { fontSize: 10, color: '#9E9E9E', marginTop: 4 },
  inputRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 10 },
  inputLabel: { width: 120, fontSize: 14, color: '#616161' },
  input: {
    flex: 1, borderWidth: 1, borderColor: '#E0E0E0', borderRadius: 6,
    paddingHorizontal: 10, paddingVertical: 8, fontSize: 14,
  },
  toggleRow: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    paddingVertical: 8,
  },
  toggleLabel: { fontSize: 14, color: '#616161' },
  statusLine: { fontSize: 13, color: '#757575', marginVertical: 2 },
  hintText: { fontSize: 12, color: '#9E9E9E', marginTop: 8 },
  applyBtn: {
    margin: 16, paddingVertical: 14, borderRadius: 8,
    backgroundColor: '#2E7D32', alignItems: 'center',
  },
  applyText: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  footer: { alignItems: 'center', padding: 20 },
  footerText: { fontSize: 11, color: '#BDBDBD' },
});