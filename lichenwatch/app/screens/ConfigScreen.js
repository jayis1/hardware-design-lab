/*
 * ConfigScreen.js — Node configuration: measurement cadence, LoRa duty cycle,
 * classifier sensitivity.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useContext } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput, Slider,
  Platform,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import ConnectionStatus from '../components/ConnectionStatus';
import { BleContext, writeNodeConfig } from '../utils/ble';

export default function ConfigScreen() {
  const { bleState } = useContext(BleContext);
  const [measureMin, setMeasureMin] = useState(15);
  const [uplinkMin, setUplinkMin] = useState(60);
  const [sensitivity, setSensitivity] = useState(1);
  const [nodeId, setNodeId] = useState(bleState.connectedNodeId || '');
  const [saving, setSaving] = useState(false);
  const [savedMsg, setSavedMsg] = useState('');

  const handleSave = async () => {
    if (!bleState.connectedNodeId) {
      setSavedMsg('Not connected to a node.');
      return;
    }
    setSaving(true);
    try {
      await writeNodeConfig(bleState.connectedNodeId, measureMin, uplinkMin);
      setSavedMsg('Configuration sent to node.');
    } catch (e) {
      setSavedMsg(`Error: ${e.message}`);
    }
    setSaving(false);
    setTimeout(() => setSavedMsg(''), 4000);
  };

  return (
    <View style={styles.container}>
      <ConnectionStatus />

      <Text style={styles.sectionTitle}>Measurement Cadence</Text>
      <Text style={styles.help}>
        How often the node wakes to run a full sensor burst (PAM fluorometry,
        spectral read, microclimate, acoustic capture). Shorter intervals give
        higher temporal resolution at the cost of battery life.
      </Text>
      <View style={styles.row}>
        <Text style={styles.label}>Interval: {measureMin} min</Text>
        <Slider
          style={styles.slider}
          minimumValue={5}
          maximumValue={240}
          step={5}
          value={measureMin}
          onValueChange={setMeasureMin}
          minimumTrackTintColor="#2e7d32"
        />
      </View>

      <Text style={styles.sectionTitle}>LoRa Uplink Cadence</Text>
      <Text style={styles.help}>
        How often the node transmits its latest state to the gateway. Raw data
        is always stored in on-board flash for walk-by BLE download regardless
        of this setting.
      </Text>
      <View style={styles.row}>
        <Text style={styles.label}>Uplink: every {uplinkMin} min</Text>
        <Slider
          style={styles.slider}
          minimumValue={15}
          maximumValue={1440}
          step={15}
          value={uplinkMin}
          onValueChange={setUplinkMin}
          minimumTrackTintColor="#1565c0"
        />
      </View>

      <Text style={styles.sectionTitle}>Classifier Sensitivity</Text>
      <Text style={styles.help}>
        Adjusts how aggressively the on-device edge-ML model flags stress. At
        high sensitivity, borderline-healthy readings are downgraded to
        stressed. Useful for protected or sensitive colonies.
      </Text>
      <View style={styles.sensRow}>
        {['Low', 'Medium', 'High', 'Max'].map((label, idx) => (
          <TouchableOpacity
            key={label}
            style={[
              styles.sensPill,
              sensitivity === idx && styles.sensPillActive,
            ]}
            onPress={() => setSensitivity(idx)}
          >
            <Text
              style={[
                styles.sensText,
                sensitivity === idx && styles.sensTextActive,
              ]}
            >
              {label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.sectionTitle}>Node Identity</Text>
      <TextInput
        style={styles.input}
        value={nodeId}
        onChangeText={setNodeId}
        placeholder="e.g. LW-0001"
        editable={!bleState.connectedNodeId}
      />

      <TouchableOpacity
        style={[styles.saveButton, saving && styles.saveButtonDisabled]}
        onPress={handleSave}
        disabled={saving}
      >
        <Icon name="content-save" size={20} color="#fff" />
        <Text style={styles.saveButtonText}>
          {saving ? 'Sending…' : 'Send configuration'}
        </Text>
      </TouchableOpacity>

      {savedMsg ? <Text style={styles.savedMsg}>{savedMsg}</Text> : null}

      <Text style={styles.footer}>
        Lichenwatch v1.0 · Author: jayis1 · MIT License
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#fafafa', padding: 14 },
  sectionTitle: {
    fontSize: 15,
    fontWeight: 'bold',
    color: '#1b3a1b',
    marginTop: 14,
    marginBottom: 4,
  },
  help: { fontSize: 11, color: '#666', marginBottom: 6, lineHeight: 16 },
  row: { flexDirection: 'row', alignItems: 'center', marginVertical: 6 },
  label: { fontSize: 13, color: '#333', width: 140 },
  slider: { flex: 1, height: 40 },
  sensRow: { flexDirection: 'row', marginTop: 6 },
  sensPill: {
    paddingVertical: 8,
    paddingHorizontal: 14,
    borderRadius: 20,
    backgroundColor: '#e0e0e0',
    marginRight: 8,
  },
  sensPillActive: { backgroundColor: '#2e7d32' },
  sensText: { fontSize: 12, color: '#333' },
  sensTextActive: { color: '#fff', fontWeight: 'bold' },
  input: {
    borderWidth: 1,
    borderColor: '#ccc',
    borderRadius: 8,
    padding: 10,
    fontSize: 14,
    marginTop: 4,
    backgroundColor: '#fff',
  },
  saveButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#1b3a1b',
    padding: 14,
    borderRadius: 8,
    marginTop: 18,
  },
  saveButtonDisabled: { backgroundColor: '#555' },
  saveButtonText: { color: '#fff', fontSize: 15, fontWeight: 'bold', marginLeft: 8 },
  savedMsg: { color: '#2e7d32', fontSize: 13, textAlign: 'center', marginTop: 10 },
  footer: { fontSize: 10, color: '#aaa', textAlign: 'center', marginTop: 24 },
});