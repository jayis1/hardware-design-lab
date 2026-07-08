/*
 * WalkBySyncScreen.js — BLE scan, connect, and bulk download of raw data.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useContext } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet, ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import ConnectionStatus from '../components/ConnectionStatus';
import {
  BleContext, scanForNodes, readNodeStatus,
  bulkDownload, saveRecords,
} from '../utils/ble';

export default function WalkBySyncScreen() {
  const { bleState, setBleState } = useContext(BleContext);
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [downloading, setDownloading] = useState(false);
  const [progress, setProgress] = useState(0);

  const handleScan = async () => {
    setScanning(true);
    setDevices([]);
    const found = await scanForNodes(8000);
    setDevices(found);
    setScanning(false);
  };

  const handleConnect = async (deviceId) => {
    setBleState({
      ...bleState,
      connectedNodeId: deviceId,
      status: 'Connected',
    });
    const s = await readNodeStatus(deviceId);
    setBleState({
      ...bleState,
      connectedNodeId: deviceId,
      status: 'Connected',
      latest: s,
    });
  };

  const handleDownload = async () => {
    if (!bleState.connectedNodeId) return;
    setDownloading(true);
    setProgress(0);
    const records = [];
    await bulkDownload(
      bleState.connectedNodeId,
      (rec, count) => {
        records.push(rec);
        setProgress(count);
      },
      async (allRecords) => {
        await saveRecords(bleState.connectedNodeId, allRecords);
        setBleState({
          ...bleState,
          history: allRecords,
        });
        setDownloading(false);
      },
    );
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={styles.deviceItem}
      onPress={() => handleConnect(item.id)}
    >
      <Icon name="access-point" size={24} color="#2e7d32" />
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
      </View>
      <Text style={styles.rssi}>{item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <ConnectionStatus />

      <View style={styles.actions}>
        <TouchableOpacity
          style={styles.button}
          onPress={handleScan}
          disabled={scanning}
        >
          {scanning ? (
            <ActivityIndicator color="#fff" />
          ) : (
            <Icon name="radar" size={20} color="#fff" />
          )}
          <Text style={styles.buttonText}>
            {scanning ? 'Scanning…' : 'Scan for nodes'}
          </Text>
        </TouchableOpacity>

        {bleState.connectedNodeId && (
          <TouchableOpacity
            style={[styles.button, styles.downloadBtn]}
            onPress={handleDownload}
            disabled={downloading}
          >
            <Icon name="download" size={20} color="#fff" />
            <Text style={styles.buttonText}>
              {downloading ? `Downloading ${progress}…` : 'Download raw data'}
            </Text>
          </TouchableOpacity>
        )}
      </View>

      <Text style={styles.sectionTitle}>
        {devices.length > 0
          ? `${devices.length} node(s) found`
          : scanning
          ? 'Searching…'
          : 'No nodes found yet. Tap Scan.'}
      </Text>

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={renderItem}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#fafafa' },
  actions: { flexDirection: 'row', padding: 12 },
  button: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1b3a1b',
    padding: 12,
    borderRadius: 8,
    marginRight: 8,
  },
  downloadBtn: { backgroundColor: '#1565c0' },
  buttonText: { color: '#fff', marginLeft: 6, fontSize: 14 },
  sectionTitle: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#1b3a1b',
    padding: 12,
  },
  deviceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 14,
    backgroundColor: '#fff',
    borderBottomWidth: 1,
    borderBottomColor: '#eee',
  },
  deviceInfo: { flex: 1, marginLeft: 10 },
  deviceName: { fontSize: 15, fontWeight: 'bold', color: '#1b3a1b' },
  deviceId: { fontSize: 11, color: '#888' },
  rssi: { fontSize: 12, color: '#666' },
});