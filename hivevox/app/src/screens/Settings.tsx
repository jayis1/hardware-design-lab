/*
 * screens/Settings.tsx — Add hives, configure sample intervals, QR scan
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TextInput, TouchableOpacity,
  ScrollView, Alert as RNAlert, Modal
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import type { HiveData } from '../App';

type Props = {
  onAddHive: (deveui: string, name: string) => void;
  onSetInterval: (deveui: string, minutes: number) => void;
  hives: HiveData[];
};

export default function Settings({ onAddHive, onSetInterval, hives }: Props) {
  const [showAdd, setShowAdd] = useState(false);
  const [deveui, setDeveui] = useState('');
  const [hiveName, setHiveName] = useState('');
  const [intervalHive, setIntervalHive] = useState('');
  const [intervalMin, setIntervalMin] = useState('10');

  const handleAdd = () => {
    if (!deveui || deveui.length < 8) {
      RNAlert.alert('Error', 'Enter a valid DevEUI (at least 8 hex chars)');
      return;
    }
    if (!hiveName.trim()) {
      RNAlert.alert('Error', 'Enter a name for this hive');
      return;
    }
    onAddHive(deveui.toUpperCase(), hiveName.trim());
    setDeveui('');
    setHiveName('');
    setShowAdd(false);
    RNAlert.alert('Added', `"${hiveName}" added to your apiary.`);
  };

  const handleIntervalChange = () => {
    const min = parseInt(intervalMin, 10);
    if (!intervalHive || isNaN(min) || min < 2 || min > 60) {
      RNAlert.alert('Error', 'Interval must be 2–60 minutes. Select a hive first.');
      return;
    }
    onSetInterval(intervalHive, min);
    RNAlert.alert('Queued', `Sample interval set to ${min} min. ` +
      'The change will apply after the next downlink window.');
    setIntervalHive('');
  };

  const handleScanQR = () => {
    // In a full build, expo-camera BarCodeScanner would read the DevEUI
    // from a QR printed on the HiveVox enclosure. Here we simulate with
    // a random DevEUI for the demo flow.
    const randomEui = Array.from({ length: 8 }, () =>
      Math.floor(Math.random() * 256).toString(16).padStart(2, '0')
    ).join('').toUpperCase();
    setDeveui(randomEui);
    RNAlert.alert('Scanned', `DevEUI detected: ${randomEui}`);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      {/* Add hive section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Add a Hive</Text>
        <Text style={styles.sectionDesc}>
          Scan the QR code on your HiveVox device or enter the DevEUI manually.
        </Text>
        <TouchableOpacity style={styles.scanButton} onPress={handleScanQR}>
          <Ionicons name="qr-code" size={20} color="white" />
          <Text style={styles.scanButtonText}>Scan QR Code</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.addButton} onPress={() => setShowAdd(true)}>
          <Ionicons name="add-circle" size={20} color="#e8a800" />
          <Text style={styles.addButtonText}>Enter Manually</Text>
        </TouchableOpacity>
      </View>

      {/* Sample interval section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Sample Interval</Text>
        <Text style={styles.sectionDesc}>
          Set how often each hive reports. Lower intervals drain battery faster.
          The change is sent via LoRa downlink on the next cycle.
        </Text>
        <Text style={styles.inputLabel}>Select Hive</Text>
        <View style={styles.hiveSelector}>
          {hives.map((h) => (
            <TouchableOpacity
              key={h.deveui}
              style={[
                styles.hivePill,
                intervalHive === h.deveui && styles.hivePillActive,
              ]}
              onPress={() => setIntervalHive(h.deveui)}
            >
              <Text style={[
                styles.hivePillText,
                intervalHive === h.deveui && styles.hivePillTextActive,
              ]}>{h.name}</Text>
            </TouchableOpacity>
          ))}
          {hives.length === 0 && (
            <Text style={styles.noHives}>No hives added yet.</Text>
          )}
        </View>
        <Text style={styles.inputLabel}>Interval (minutes: 2–60)</Text>
        <TextInput
          style={styles.input}
          value={intervalMin}
          onChangeText={setIntervalMin}
          keyboardType="numeric"
          placeholder="10"
        />
        <TouchableOpacity style={styles.applyButton} onPress={handleIntervalChange}>
          <Text style={styles.applyButtonText}>Apply via Downlink</Text>
        </TouchableOpacity>
      </View>

      {/* Hive list with current settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Registered Hives ({hives.length})</Text>
        {hives.length === 0 ? (
          <Text style={styles.noHives}>No hives registered.</Text>
        ) : (
          hives.map((h) => (
            <View key={h.deveui} style={styles.hiveRow}>
              <Ionicons name="cube" size={16} color="#e8a800" />
              <View style={styles.hiveRowInfo}>
                <Text style={styles.hiveRowName}>{h.name}</Text>
                <Text style={styles.hiveRowEui}>{h.deveui}</Text>
              </View>
              <Text style={styles.hiveRowBat}>{Math.round((h.batteryMv - 2400) / 12)}%</Text>
            </View>
          ))
        )}
      </View>

      {/* About */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>About HiveVox</Text>
        <Text style={styles.aboutText}>
          HiveVox v1.0.0{"\n"}
          Author: jayis1{"\n"}
          License: MIT{"\n\n"}
          HiveVox is an acoustic beehive health monitor that detects queen
          loss, swarming preparation, and winter cluster state from colony
          sound, temperature, and weight — reporting via LoRaWAN without
          cell service or Wi-Fi.
        </Text>
      </View>

      {/* Manual add modal */}
      <Modal visible={showAdd} animationType="slide" transparent>
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Add Hive Manually</Text>
            <Text style={styles.inputLabel}>DevEUI (hex)</Text>
            <TextInput
              style={styles.input}
              value={deveui}
              onChangeText={setDeveui}
              placeholder="ACDE480000000001"
              autoCapitalize="characters"
            />
            <Text style={styles.inputLabel}>Hive Name</Text>
            <TextInput
              style={styles.input}
              value={hiveName}
              onChangeText={setHiveName}
              placeholder="Backyard Hive #1"
            />
            <View style={styles.modalButtons}>
              <TouchableOpacity onPress={() => setShowAdd(false)}>
                <Text style={styles.cancelText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity style={styles.saveButton} onPress={handleAdd}>
                <Text style={styles.saveButtonText}>Save</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  title: {
    fontSize: 24, fontWeight: '800', color: '#222',
    margin: 16, marginBottom: 8,
  },
  section: {
    backgroundColor: '#fff', margin: 12, borderRadius: 10,
    padding: 16, elevation: 1, shadowColor: '#000',
    shadowOpacity: 0.05, shadowRadius: 2, shadowOffset: { width: 0, height: 1 },
  },
  sectionTitle: { fontSize: 16, fontWeight: '700', color: '#333', marginBottom: 6 },
  sectionDesc: { fontSize: 13, color: '#888', marginBottom: 12, lineHeight: 18 },
  scanButton: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#e8a800', borderRadius: 8, padding: 12, marginBottom: 8,
  },
  scanButtonText: { color: 'white', fontWeight: '600', marginLeft: 8 },
  addButton: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    borderWidth: 1, borderColor: '#e8a800', borderRadius: 8, padding: 12,
  },
  addButtonText: { color: '#e8a800', fontWeight: '600', marginLeft: 8 },
  inputLabel: { fontSize: 12, color: '#888', marginTop: 12, marginBottom: 4 },
  input: {
    borderWidth: 1, borderColor: '#ddd', borderRadius: 6,
    padding: 10, fontSize: 14, backgroundColor: '#fafafa',
  },
  hiveSelector: { flexDirection: 'row', flexWrap: 'wrap', marginVertical: 8 },
  hivePill: {
    borderWidth: 1, borderColor: '#ddd', borderRadius: 16,
    paddingHorizontal: 12, paddingVertical: 6, margin: 4,
  },
  hivePillActive: { backgroundColor: '#e8a800', borderColor: '#e8a800' },
  hivePillText: { fontSize: 12, color: '#666' },
  hivePillTextActive: { color: 'white', fontWeight: '600' },
  noHives: { fontSize: 13, color: '#aaa', marginVertical: 8 },
  applyButton: {
    backgroundColor: '#4caf50', borderRadius: 8, padding: 12,
    alignItems: 'center', marginTop: 12,
  },
  applyButtonText: { color: 'white', fontWeight: '600' },
  hiveRow: {
    flexDirection: 'row', alignItems: 'center', paddingVertical: 10,
    borderBottomWidth: 0.5, borderBottomColor: '#eee',
  },
  hiveRowInfo: { flex: 1, marginLeft: 8 },
  hiveRowName: { fontSize: 14, fontWeight: '600', color: '#333' },
  hiveRowEui: { fontSize: 11, color: '#999' },
  hiveRowBat: { fontSize: 13, color: '#666' },
  aboutText: { fontSize: 13, color: '#666', lineHeight: 20 },
  modalOverlay: {
    flex: 1, justifyContent: 'center', alignItems: 'center',
    backgroundColor: 'rgba(0,0,0,0.5)', padding: 20,
  },
  modalContent: {
    backgroundColor: 'white', borderRadius: 12, padding: 20, width: '100%',
  },
  modalTitle: { fontSize: 18, fontWeight: '700', color: '#333', marginBottom: 16 },
  modalButtons: {
    flexDirection: 'row', justifyContent: 'flex-end', marginTop: 16,
  },
  cancelText: { color: '#888', fontSize: 15, marginRight: 20, padding: 8 },
  saveButton: { backgroundColor: '#e8a800', borderRadius: 6, padding: 8 },
  saveButtonText: { color: 'white', fontWeight: '600' },
});