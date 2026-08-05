// src/screens/ProvisioningScreen.tsx — Mesh node provisioning
//
// Scan for FrostSentinel nodes, assign node IDs, set mesh roles,
// configure the network AES key, and set sample intervals.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, TextInput,
  Alert, FlatList,
} from 'react-native';
import bleManager from '../ble/BleManager';

export default function ProvisioningScreen() {
  const [scanning, setScanning] = useState(false);
  const [foundDevices, setFoundDevices] = useState<string[]>([]);
  const [nodeId, setNodeId] = useState('1');
  const [meshRole, setMeshRole] = useState(0); // 0=leaf, 1=relay, 2=root
  const [networkKey, setNetworkKey] = useState(
    'F5051283A19E4BC7D23F68117AB4E69D'
  );
  const [sampleInterval, setSampleInterval] = useState('5');

  const scan = async () => {
    setScanning(true);
    setFoundDevices([]);
    try {
      // In production, this would use the BLE manager's scan function
      // For now, we simulate finding devices
      setTimeout(() => {
        setFoundDevices(['Node A1B2', 'Node C3D4', 'Node E5F6']);
        setScanning(false);
      }, 2000);
    } catch (e) {
      Alert.alert('Error', 'Scan failed. Check BLE permissions.');
      setScanning(false);
    }
  };

  const provision = async () => {
    const id = parseInt(nodeId, 10);
    if (isNaN(id) || id < 1 || id > 32) {
      Alert.alert('Invalid Node ID', 'Node ID must be 1–32.');
      return;
    }

    // Parse hex network key
    const keyHex = networkKey.replace(/[^0-9A-Fa-f]/g, '');
    if (keyHex.length !== 32) {
      Alert.alert('Invalid Key', 'Network key must be 32 hex characters (16 bytes).');
      return;
    }
    const keyBytes = new Uint8Array(16);
    for (let i = 0; i < 16; i++) {
      keyBytes[i] = parseInt(keyHex.substr(i * 2, 2), 16);
    }

    try {
      await bleManager.provision(id, meshRole, keyBytes);
      const roleLabel = ['Leaf', 'Relay', 'Root'][meshRole];
      Alert.alert(
        'Provisioned',
        `Node ${id} configured as ${roleLabel}.\nSample interval: ${sampleInterval} min.`
      );
    } catch (e) {
      Alert.alert('Error', 'Provisioning failed. Is BLE connected?');
    }
  };

  const setTimeInterval = async () => {
    const interval = parseInt(sampleInterval, 10);
    if (isNaN(interval) || interval < 1 || interval > 60) {
      Alert.alert('Invalid Interval', 'Sample interval must be 1–60 minutes.');
      return;
    }
    try {
      await bleManager.setSampleInterval(interval);
      Alert.alert('Success', `Sample interval set to ${interval} minutes.`);
    } catch (e) {
      Alert.alert('Error', 'Could not set interval.');
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Provisioning</Text>

      {/* Scan section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Scan for Nodes</Text>
        <TouchableOpacity style={styles.scanButton} onPress={scan} disabled={scanning}>
          <Text style={styles.buttonText}>
            {scanning ? 'Scanning...' : 'Scan for Nodes'}
          </Text>
        </TouchableOpacity>
        <FlatList
          data={foundDevices}
          renderItem={({ item }) => (
            <View style={styles.deviceItem}>
              <Text style={styles.deviceName}>{item}</Text>
              <Text style={styles.deviceStatus}>Not provisioned</Text>
            </View>
          )}
          keyExtractor={(item, idx) => idx.toString()}
          ListEmptyComponent={
            <Text style={styles.emptyText}>No devices found. Tap scan.</Text>
          }
          style={styles.deviceList}
        />
      </View>

      {/* Node configuration */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Node Configuration</Text>

        <Text style={styles.inputLabel}>Node ID (1–32)</Text>
        <TextInput
          style={styles.input}
          value={nodeId}
          onChangeText={setNodeId}
          keyboardType="numeric"
          placeholder="1"
          placeholderTextColor="#555"
        />

        <Text style={styles.inputLabel}>Mesh Role</Text>
        <View style={styles.roleRow}>
          {['Leaf', 'Relay', 'Root'].map((role, i) => (
            <TouchableOpacity
              key={i}
              style={[styles.roleButton, meshRole === i ? styles.roleActive : null]}
              onPress={() => setMeshRole(i)}
            >
              <Text style={[styles.roleText, meshRole === i ? styles.roleTextActive : null]}>
                {role}
              </Text>
            </TouchableOpacity>
          ))}
        </View>

        <Text style={styles.inputLabel}>Network AES Key (32 hex chars)</Text>
        <TextInput
          style={[styles.input, styles.keyInput]}
          value={networkKey}
          onChangeText={setNetworkKey}
          placeholder="F5051283A19E4BC7D23F68117AB4E69D"
          placeholderTextColor="#555"
          autoCapitalize="characters"
          autoCorrect={false}
        />

        <Text style={styles.inputLabel}>Sample Interval (minutes, 1–60)</Text>
        <View style={styles.intervalRow}>
          <TextInput
            style={[styles.input, { flex: 1, marginRight: 8 }]}
            value={sampleInterval}
            onChangeText={setSampleInterval}
            keyboardType="numeric"
            placeholder="5"
            placeholderTextColor="#555"
          />
          <TouchableOpacity style={styles.smallButton} onPress={setTimeInterval}>
            <Text style={styles.buttonText}>Set</Text>
          </TouchableOpacity>
        </View>

        <TouchableOpacity style={styles.provisionButton} onPress={provision}>
          <Text style={styles.buttonText}>Provision Node</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a', padding: 20, paddingTop: 40 },
  title: { fontSize: 22, fontWeight: 'bold', color: '#fff', marginBottom: 15 },
  section: { backgroundColor: '#1b263b', borderRadius: 12, padding: 15, marginBottom: 15 },
  sectionTitle: { fontSize: 15, fontWeight: 'bold', color: '#e0e1dd', marginBottom: 10 },
  scanButton: {
    backgroundColor: '#2196F3', paddingHorizontal: 20, paddingVertical: 10,
    borderRadius: 8, alignItems: 'center', marginBottom: 10,
  },
  buttonText: { color: '#fff', fontSize: 14, fontWeight: '600' },
  deviceList: { maxHeight: 120 },
  deviceItem: {
    flexDirection: 'row', justifyContent: 'space-between',
    paddingVertical: 8, borderBottomWidth: 1, borderBottomColor: '#333',
  },
  deviceName: { fontSize: 13, color: '#e0e1dd' },
  deviceStatus: { fontSize: 11, color: '#FF9800' },
  emptyText: { fontSize: 13, color: '#778da9', textAlign: 'center', padding: 10 },
  inputLabel: { fontSize: 12, color: '#778da9', marginBottom: 4, marginTop: 10 },
  input: {
    backgroundColor: '#0d1b2a', borderRadius: 6, paddingHorizontal: 12,
    paddingVertical: 8, color: '#fff', fontSize: 14,
  },
  keyInput: { fontFamily: 'monospace' },
  roleRow: { flexDirection: 'row', justifyContent: 'space-between' },
  roleButton: {
    flex: 1, backgroundColor: '#0d1b2a', borderRadius: 6, paddingVertical: 8,
    alignItems: 'center', marginHorizontal: 4,
  },
  roleActive: { backgroundColor: '#2196F3' },
  roleText: { fontSize: 12, color: '#778da9' },
  roleTextActive: { color: '#fff', fontWeight: '600' },
  intervalRow: { flexDirection: 'row', alignItems: 'center' },
  smallButton: {
    backgroundColor: '#2196F3', paddingHorizontal: 16, paddingVertical: 10,
    borderRadius: 8,
  },
  provisionButton: {
    backgroundColor: '#4CAF50', paddingHorizontal: 20, paddingVertical: 12,
    borderRadius: 8, alignItems: 'center', marginTop: 15,
  },
});