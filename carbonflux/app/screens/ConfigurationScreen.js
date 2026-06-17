/**
 * ConfigurationScreen.js — Device settings, calibration, and LoRaWAN config.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {useState} from 'react';
import {
  View, ScrollView, Text, StyleSheet, TextInput, TouchableOpacity, Alert, Switch,
} from 'react-native';

const ConfigurationScreen = () => {
  const [interval, setInterval] = useState('1800');
  const [accumDuration, setAccumDuration] = useState('180');
  const [chamberVolume, setChamberVolume] = useState('4.5');
  const [collarDiameter, setCollarDiameter] = useState('20');
  const [loraRegion, setLoraRegion] = useState('EU868');
  const [ascEnabled, setAscEnabled] = useState(true);
  const [bleEnabled, setBleEnabled] = useState(true);

  const saveConfig = () => {
    Alert.alert('Configuration Saved', `Interval: ${interval}s\nAccum: ${accumDuration}s\nVolume: ${chamberVolume}L`);
  };

  const startCalZero = () => {
    Alert.alert('Zero Calibration', 'Flush chamber with CO₂-free air.\nClick OK when ready.', [
      {text: 'Cancel', style: 'cancel'},
      {text: 'Start', onPress: () => Alert.alert('Calibrating', 'Zero calibration in progress...')},
    ]);
  };

  const startCalSpan = () => {
    Alert.alert('Span Calibration', 'Connect 5000 ppm calibration gas.\nClick OK when ready.', [
      {text: 'Cancel', style: 'cancel'},
      {text: 'Start', onPress: () => Alert.alert('Calibrating', 'Span calibration in progress...')},
    ]);
  };

  const ConfigRow = ({label, children}) => (
    <View style={styles.configRow}>
      <Text style={styles.configLabel}>{label}</Text>
      {children}
    </View>
  );

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Measurement Settings</Text>

      <ConfigRow label="Interval (s)">
        <TextInput style={styles.input} value={interval} onChangeText={setInterval} keyboardType="numeric" />
      </ConfigRow>
      <ConfigRow label="Accumulation (s)">
        <TextInput style={styles.input} value={accumDuration} onChangeText={setAccumDuration} keyboardType="numeric" />
      </ConfigRow>
      <ConfigRow label="Chamber Vol (L)">
        <TextInput style={styles.input} value={chamberVolume} onChangeText={setChamberVolume} keyboardType="decimal-pad" />
      </ConfigRow>
      <ConfigRow label="Collar Dia (cm)">
        <TextInput style={styles.input} value={collarDiameter} onChangeText={setCollarDiameter} keyboardType="numeric" />
      </ConfigRow>

      <Text style={styles.sectionTitle}>Radio & Connectivity</Text>

      <ConfigRow label="LoRaWAN Region">
        <View style={styles.regionRow}>
          {['EU868', 'US915', 'AU915'].map(r => (
            <TouchableOpacity
              key={r}
              style={[styles.regionBtn, loraRegion === r && styles.regionBtnActive]}
              onPress={() => setLoraRegion(r)}>
              <Text style={[styles.regionText, loraRegion === r && styles.regionTextActive]}>
                {r}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </ConfigRow>

      <ConfigRow label="BLE Enabled">
        <Switch value={bleEnabled} onValueChange={setBleEnabled} trackColor={{false: '#555', true: '#4CAF50'}} />
      </ConfigRow>

      <Text style={styles.sectionTitle}>Calibration</Text>

      <View style={styles.calRow}>
        <TouchableOpacity style={styles.calButton} onPress={startCalZero}>
          <Text style={styles.calButtonText}>Start Zero Cal</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.calButton} onPress={startCalSpan}>
          <Text style={styles.calButtonText}>Start Span Cal</Text>
        </TouchableOpacity>
      </View>

      <ConfigRow label="Auto Self-Cal">
        <Switch value={ascEnabled} onValueChange={setAscEnabled} trackColor={{false: '#555', true: '#4CAF50'}} />
      </ConfigRow>

      {/* Save button */}
      <TouchableOpacity style={styles.saveButton} onPress={saveConfig}>
        <Text style={styles.saveButtonText}>Save Configuration</Text>
      </TouchableOpacity>

      <View style={styles.infoBox}>
        <Text style={styles.infoTitle}>Device Info</Text>
        <Text style={styles.infoText}>Model: CarbonFlux v1.0</Text>
        <Text style={styles.infoText}>Serial: 2401A00001</Text>
        <Text style={styles.infoText}>Firmware: 1.0.0</Text>
        <Text style={styles.infoText}>Author: jayis1</Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {flex: 1, backgroundColor: '#0f0f23', padding: 8},
  sectionTitle: {
    color: '#4CAF50', fontSize: 14, fontWeight: '600',
    textTransform: 'uppercase', letterSpacing: 1.5,
    marginTop: 16, marginBottom: 8, marginLeft: 4,
  },
  configRow: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginVertical: 2,
    borderWidth: 1, borderColor: '#252540',
  },
  configLabel: {color: '#aaa', fontSize: 14, flex: 1},
  input: {
    backgroundColor: '#252540', color: '#e0e0e0', borderRadius: 6,
    padding: 8, width: 80, textAlign: 'center', fontFamily: 'monospace',
    fontSize: 14,
  },
  regionRow: {flexDirection: 'row'},
  regionBtn: {
    paddingHorizontal: 12, paddingVertical: 6, marginHorizontal: 2,
    borderRadius: 12, backgroundColor: '#252540',
  },
  regionBtnActive: {backgroundColor: '#4CAF50'},
  regionText: {color: '#888', fontSize: 12},
  regionTextActive: {color: '#fff', fontWeight: '700'},
  calRow: {flexDirection: 'row', justifyContent: 'space-around', marginVertical: 8},
  calButton: {
    backgroundColor: '#1a1a2e', padding: 16, borderRadius: 12,
    borderWidth: 1, borderColor: '#FF9800', width: '47%', alignItems: 'center',
  },
  calButtonText: {color: '#FF9800', fontWeight: '600'},
  saveButton: {
    backgroundColor: '#4CAF50', padding: 16, borderRadius: 12,
    alignItems: 'center', marginVertical: 16,
  },
  saveButtonText: {color: '#fff', fontSize: 16, fontWeight: '700'},
  infoBox: {
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginVertical: 8,
    borderWidth: 1, borderColor: '#333',
  },
  infoTitle: {color: '#888', fontSize: 11, textTransform: 'uppercase', letterSpacing: 1, marginBottom: 8},
  infoText: {color: '#e0e0e0', fontSize: 13, fontFamily: 'monospace', marginVertical: 2},
});

export default ConfigurationScreen;