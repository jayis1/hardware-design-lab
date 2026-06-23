/**
 * NodeMapScreen.js — Mesh map view for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Displays all MycoMesh nodes in the mesh, color-coded by activity class,
 * with signal strength and battery status.  Also provides BLE scan and
 * connection controls.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Alert } from 'react-native';
import { useBle } from '../utils/BleContext';
import ActivityBadge from '../components/ActivityBadge';

export default function NodeMapScreen() {
  const { connected, scanning, status, epoch, startScan, disconnect, classLabels, classColors } = useBle();
  const [nodes, setNodes] = useState([]);

  // Simulated mesh nodes (in production, these come from LoRaWAN → MQTT).
  useEffect(() => {
    const mockNodes = [
      { id: 'node-01', label: 'Forest Plot A', class: 1, battery: 85, rssi: -65 },
      { id: 'node-02', label: 'Forest Plot B', class: 2, battery: 72, rssi: -78 },
      { id: 'node-03', label: 'Bioreactor 1', class: 1, battery: 91, rssi: -52 },
      { id: 'node-04', label: 'Mycelium Bed 3', class: 0, battery: 45, rssi: -89 },
    ];
    setNodes(mockNodes);
  }, []);

  const handleScan = () => {
    if (connected) {
      Alert.alert('Disconnect', 'Disconnect from current node?', [
        { text: 'Cancel' },
        { text: 'Disconnect', onPress: disconnect },
      ]);
    } else {
      startScan();
    }
  };

  const renderNode = ({ item }) => (
    <View style={styles.nodeCard}>
      <View style={styles.nodeHeader}>
        <Text style={styles.nodeId}>{item.label}</Text>
        <ActivityBadge classLabel={item.class} />
      </View>
      <View style={styles.nodeDetails}>
        <Text style={styles.detailText}>ID: {item.id}</Text>
        <Text style={styles.detailText}>Battery: {item.battery}%</Text>
        <Text style={styles.detailText}>RSSI: {item.rssi} dBm</Text>
      </View>
      <View style={[styles.classIndicator, { backgroundColor: classColors[item.class] }]} />
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>MycoMesh Nodes</Text>
        <TouchableOpacity style={styles.scanButton} onPress={handleScan}>
          <Text style={styles.scanButtonText}>
            {connected ? 'Disconnect' : scanning ? 'Scanning...' : 'Scan for Node'}
          </Text>
        </TouchableOpacity>
      </View>

      {connected && status && (
        <View style={styles.connectedInfo}>
          <Text style={styles.connectedText}>
            Connected • Class: {classLabels[status.classLabel] || 'Unknown'}
          </Text>
          <Text style={styles.connectedText}>
            Battery: {status.batteryMv} mV • Duty: {status.dutyCyclePct}%
          </Text>
        </View>
      )}

      <FlatList
        data={nodes}
        renderItem={renderNode}
        keyExtractor={(item) => item.id}
        contentContainerStyle={styles.list}
      />

      <View style={styles.footer}>
        <Text style={styles.footerText}>MycoMesh v1.0 — jayis1 — GPL-2.0</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1a0a' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between',
    alignItems: 'center', padding: 16, borderBottomWidth: 1, borderBottomColor: '#2a3a2a',
  },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  scanButton: {
    backgroundColor: '#4CAF50', paddingHorizontal: 16, paddingVertical: 8,
    borderRadius: 8,
  },
  scanButtonText: { color: '#fff', fontWeight: 'bold' },
  connectedInfo: { padding: 12, backgroundColor: '#1a2a1a', margin: 8, borderRadius: 8 },
  connectedText: { color: '#4CAF50', fontSize: 13 },
  list: { padding: 8 },
  nodeCard: {
    backgroundColor: '#1a2a1a', borderRadius: 12, padding: 16,
    marginBottom: 8, borderLeftWidth: 4, borderLeftColor: '#4CAF50',
  },
  nodeHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  nodeId: { color: '#fff', fontSize: 16, fontWeight: 'bold' },
  nodeDetails: { marginTop: 8 },
  detailText: { color: '#aaa', fontSize: 13 },
  classIndicator: {
    position: 'absolute', right: 0, top: 0, bottom: 0, width: 4, borderRadius: 2,
  },
  footer: { padding: 12, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 11 },
});