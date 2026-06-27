/*
 * MedicationScreen.js — Medication dose logging and correlation
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  Modal, TextInput, Alert,
} from 'react-native';
import BleManager from '../ble/BleManager';
import AsyncStorage from '@react-native-async-storage/async-storage';

export default function MedicationScreen() {
  const [doses, setDoses] = useState([]);
  const [showAddModal, setShowAddModal] = useState(false);
  const [medName, setMedName] = useState('');
  const [medDose, setMedDose] = useState('');
  const [tremorCorrelation, setTremorCorrelation] = useState([]);

  useEffect(() => {
    loadDoses();
  }, []);

  const loadDoses = async () => {
    try {
      const stored = await AsyncStorage.getItem('medication_doses');
      if (stored) {
        setDoses(JSON.parse(stored));
      }
    } catch (e) {
      console.error('Failed to load doses:', e);
    }
  };

  const saveDoses = async (newDoses) => {
    try {
      await AsyncStorage.setItem('medication_doses', JSON.stringify(newDoses));
      setDoses(newDoses);
    } catch (e) {
      console.error('Failed to save doses:', e);
    }
  };

  const logDose = useCallback(() => {
    const newDose = {
      id: Date.now(),
      timestamp: Date.now(),
      name: medName || 'Levodopa',
      dose: medDose || '100mg',
    };
    const newDoses = [newDose, ...doses];
    saveDoses(newDoses);
    setMedName('');
    setMedDose('');
    setShowAddModal(false);
  }, [medName, medDose, doses]);

  const logDoseViaNFC = useCallback(async () => {
    // Trigger NFC tag simulation on the device via BLE
    const result = await BleManager.sendCommand('LOG_DOSE', { timestamp: Date.now() });
    if (result) {
      Alert.alert('NFC Dose Logged', 'Medication dose timestamp recorded via NFC tap.');
    } else {
      Alert.alert('Error', 'Could not log dose. Device not connected.');
    }
  }, []);

  const deleteDose = (id) => {
    Alert.alert(
      'Delete Dose',
      'Remove this medication dose record?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Delete', style: 'destructive', onPress: () => {
          const newDoses = doses.filter(d => d.id !== id);
          saveDoses(newDoses);
        }},
      ]
    );
  };

  const renderDoseItem = ({ item }) => {
    const time = new Date(item.timestamp).toLocaleString();
    const hoursAgo = ((Date.now() - item.timestamp) / 3600000).toFixed(1);

    // Find tremor correlation: look at tremor activity 30 min before and after dose
    const beforeDose = tremorCorrelation.find(c => c.doseId === item.id && c.window === 'before');
    const afterDose = tremorCorrelation.find(c => c.doseId === item.id && c.window === 'after');

    return (
      <TouchableOpacity
        style={styles.doseItem}
        onLongPress={() => deleteDose(item.id)}
      >
        <View style={styles.doseHeader}>
          <Text style={styles.doseName}>{item.name}</Text>
          <Text style={styles.doseTime}>{time}</Text>
        </View>
        <Text style={styles.doseAmount}>{item.dose}</Text>
        <Text style={styles.doseAgo}>{hoursAgo}h ago</Text>

        {beforeDose && afterDose && (
          <View style={styles.correlationBox}>
            <Text style={styles.correlationTitle}>Tremor Response</Text>
            <View style={styles.correlationRow}>
              <Text style={styles.correlationLabel}>30 min before:</Text>
              <Text style={styles.correlationValue}>
                {beforeDose.tremorMinutes.toFixed(1)} min
              </Text>
            </View>
            <View style={styles.correlationRow}>
              <Text style={styles.correlationLabel}>30 min after:</Text>
              <Text style={[styles.correlationValue,
                { color: afterDose.tremorMinutes < beforeDose.tremorMinutes ? '#34C759' : '#FF3B30' }]}>
                {afterDose.tremorMinutes.toFixed(1)} min
              </Text>
            </View>
            {beforeDose.tremorMinutes > 0 && (
              <Text style={styles.improvementText}>
                Improvement: {(((beforeDose.tremorMinutes - afterDose.tremorMinutes) /
                beforeDose.tremorMinutes) * 100).toFixed(0)}%
              </Text>
            )}
          </View>
        )}
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Medication Log</Text>
      <Text style={styles.subtitle}>Track doses and correlate with tremor suppression</Text>

      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.logButton} onPress={() => setShowAddModal(true)}>
          <Text style={styles.logButtonText}>+ Log Dose</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.nfcButton} onPress={logDoseViaNFC}>
          <Text style={styles.nfcButtonText}>📱 NFC Tap</Text>
        </TouchableOpacity>
      </View>

      <Text style={styles.sectionLabel}>Recent Doses ({doses.length})</Text>
      <FlatList
        data={doses}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderDoseItem}
        style={styles.doseList}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No doses logged. Tap "Log Dose" to start.</Text>
        }
      />

      {/* Add dose modal */}
      <Modal visible={showAddModal} animationType="slide" transparent={true}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Log Medication Dose</Text>
            <TextInput
              style={styles.input}
              placeholder="Medication name (e.g., Levodopa)"
              placeholderTextColor="#8E8E93"
              value={medName}
              onChangeText={setMedName}
            />
            <TextInput
              style={styles.input}
              placeholder="Dose (e.g., 100mg)"
              placeholderTextColor="#8E8E93"
              value={medDose}
              onChangeText={setMedDose}
            />
            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={styles.cancelButton}
                onPress={() => setShowAddModal(false)}
              >
                <Text style={styles.cancelButtonText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity style={styles.saveButton} onPress={logDose}>
                <Text style={styles.saveButtonText}>Save</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#8E8E93', fontSize: 14, marginBottom: 16 },
  buttonRow: { flexDirection: 'row', marginBottom: 16 },
  logButton: {
    backgroundColor: '#0A84FF',
    borderRadius: 10,
    padding: 14,
    marginRight: 8,
    flex: 1,
    alignItems: 'center',
  },
  logButtonText: { color: '#FFFFFF', fontSize: 16, fontWeight: '600' },
  nfcButton: {
    backgroundColor: '#34C759',
    borderRadius: 10,
    padding: 14,
    flex: 1,
    alignItems: 'center',
  },
  nfcButtonText: { color: '#FFFFFF', fontSize: 16, fontWeight: '600' },
  sectionLabel: { color: '#8E8E93', fontSize: 14, marginBottom: 8 },
  doseList: { flex: 1 },
  doseItem: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 8,
  },
  doseHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  doseName: { color: '#FFFFFF', fontSize: 16, fontWeight: '600' },
  doseTime: { color: '#8E8E93', fontSize: 12 },
  doseAmount: { color: '#0A84FF', fontSize: 14, marginBottom: 2 },
  doseAgo: { color: '#8E8E93', fontSize: 11, marginBottom: 8 },
  correlationBox: {
    backgroundColor: '#1C1C1E',
    borderRadius: 8,
    padding: 10,
    marginTop: 8,
  },
  correlationTitle: { color: '#8E8E93', fontSize: 12, marginBottom: 6 },
  correlationRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  correlationLabel: { color: '#8E8E93', fontSize: 12 },
  correlationValue: { color: '#FFFFFF', fontSize: 12, fontWeight: '600' },
  improvementText: { color: '#34C759', fontSize: 12, marginTop: 4, fontWeight: '600' },
  emptyText: { color: '#8E8E93', fontSize: 14, textAlign: 'center', marginTop: 40 },
  modalOverlay: {
    flex: 1,
    justifyContent: 'center',
    backgroundColor: 'rgba(0,0,0,0.5)',
  },
  modalContent: {
    backgroundColor: '#2C2C2E',
    borderRadius: 16,
    padding: 20,
    margin: 20,
  },
  modalTitle: { color: '#FFFFFF', fontSize: 18, fontWeight: 'bold', marginBottom: 16 },
  input: {
    backgroundColor: '#1C1C1E',
    borderRadius: 8,
    padding: 12,
    color: '#FFFFFF',
    marginBottom: 12,
    fontSize: 16,
  },
  modalButtons: { flexDirection: 'row', justifyContent: 'flex-end' },
  cancelButton: { padding: 12, marginRight: 12 },
  cancelButtonText: { color: '#8E8E93', fontSize: 16 },
  saveButton: {
    backgroundColor: '#0A84FF',
    borderRadius: 8,
    padding: 12,
  },
  saveButtonText: { color: '#FFFFFF', fontSize: 16, fontWeight: '600' },
});