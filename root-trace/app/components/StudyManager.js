/**
 * StudyManager.js — Study and plant management screen
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, FlatList, Modal,
  TextInput, Alert,
} from 'react-native';

const Colors = {
  bg: '#0a1628', card: '#162640', accent: '#3b82f6', text: '#e2e8f0',
  textDim: '#94a3b8', success: '#10b981',
};

export function StudyManager({ navigation }) {
  const [studies, setStudies] = useState([
    { id: 1, name: 'Drought Resistance Trial 2026', plantCount: 12, date: '2026-06-01' },
    { id: 2, name: 'Vineyard Root Depth Study', plantCount: 48, date: '2026-06-15' },
    { id: 3, name: 'Cover Crop Carbon Sequestration', plantCount: 24, date: '2026-07-01' },
  ]);
  const [modalVisible, setModalVisible] = useState(false);
  const [newStudyName, setNewStudyName] = useState('');

  const addStudy = useCallback(() => {
    if (!newStudyName.trim()) return;
    const newStudy = {
      id: Date.now(),
      name: newStudyName.trim(),
      plantCount: 0,
      date: new Date().toISOString().split('T')[0],
    };
    setStudies([...studies, newStudy]);
    setNewStudyName('');
    setModalVisible(false);
  }, [newStudyName]);

  const renderStudy = ({ item }) => (
    <TouchableOpacity
      style={styles.studyItem}
      onPress={() => Alert.alert('Study', `Opening ${item.name} (not implemented in demo)`)}>
      <Text style={styles.studyName}>{item.name}</Text>
      <View style={styles.studyMeta}>
        <Text style={styles.metaText}>{item.plantCount} plants</Text>
        <Text style={styles.metaText}>Started: {item.date}</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Studies</Text>
      <Text style={styles.subtitle}>Manage plant studies and interventions</Text>

      <FlatList
        data={studies}
        renderItem={renderStudy}
        keyExtractor={item => item.id.toString()}
        style={styles.list}
      />

      <TouchableOpacity
        style={styles.addButton}
        onPress={() => setModalVisible(true)}>
        <Text style={styles.addButtonText}>+ New Study</Text>
      </TouchableOpacity>

      <Modal visible={modalVisible} animationType="slide" transparent={true}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalCard}>
            <Text style={styles.modalTitle}>New Study</Text>
            <TextInput
              style={styles.input}
              placeholder="Study name"
              placeholderTextColor={Colors.textDim}
              value={newStudyName}
              onChangeText={setNewStudyName}
            />
            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={[styles.button, styles.cancelButton]}
                onPress={() => setModalVisible(false)}>
                <Text style={styles.buttonText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.button, styles.confirmButton]}
                onPress={addStudy}>
                <Text style={styles.buttonText}>Create</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.bg, padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: Colors.text },
  subtitle: { fontSize: 14, color: Colors.textDim, marginBottom: 16 },
  list: { flex: 1 },
  studyItem: {
    backgroundColor: Colors.card, borderRadius: 10, padding: 16, marginBottom: 8,
  },
  studyName: { fontSize: 16, color: Colors.text, fontWeight: '600' },
  studyMeta: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 8 },
  metaText: { fontSize: 12, color: Colors.textDim },
  addButton: {
    backgroundColor: Colors.accent, borderRadius: 10, padding: 14,
    alignItems: 'center', marginTop: 12,
  },
  addButtonText: { color: Colors.text, fontSize: 16, fontWeight: '600' },
  modalOverlay: { flex: 1, justifyContent: 'center', backgroundColor: 'rgba(0,0,0,0.5)' },
  modalCard: { backgroundColor: Colors.card, margin: 24, borderRadius: 12, padding: 20 },
  modalTitle: { fontSize: 18, color: Colors.text, fontWeight: 'bold', marginBottom: 12 },
  input: {
    backgroundColor: Colors.bg, borderRadius: 8, padding: 12, color: Colors.text,
    marginBottom: 16, borderWidth: 1, borderColor: Colors.textDim,
  },
  modalButtons: { flexDirection: 'row', justifyContent: 'flex-end' },
  button: { padding: 10, borderRadius: 8, marginLeft: 8 },
  cancelButton: { backgroundColor: Colors.bg },
  confirmButton: { backgroundColor: Colors.accent },
  buttonText: { color: Colors.text, fontSize: 14, fontWeight: '600' },
});