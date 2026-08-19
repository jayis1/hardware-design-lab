/**
 * ConnectScreen.js — BLE device scanning and connection
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet,
  ActivityIndicator, Alert,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { useDevice, scanForDevices, requestBlePermissions } from '../utils/protocol';
import DeviceList from '../components/DeviceList';

export default function ConnectScreen() {
  const { connected, deviceId, connect, disconnect, status } = useDevice();
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);

  const handleScan = useCallback(async () => {
    setScanning(true);
    const ok = await requestBlePermissions();
    if (!ok) {
      Alert.alert('Permission Denied', 'BLE permissions are required.');
      setScanning(false);
      return;
    }
    try {
      const found = await scanForDevices(5000);
      setDevices(found);
    } catch (e) {
      Alert.alert('Scan Error', e.message);
    }
    setScanning(false);
  }, []);

  const handleConnect = useCallback(async (id) => {
    await connect(id);
  }, [connect]);

  const handleDisconnect = useCallback(async () => {
    await disconnect();
  }, [disconnect]);

  useEffect(() => {
    handleScan();
  }, []);

  return (
    <View style={styles.container}>
      {/* Connection status banner */}
      <View style={[styles.banner, connected ? styles.connected : styles.disconnected]}>
        <Icon
          name={connected ? 'bluetooth-connected' : 'bluetooth-disabled'}
          size={24}
          color="#fff"
        />
        <Text style={styles.bannerText}>
          {connected ? `Connected: ${deviceId?.substring(0, 12)}...` : 'Not Connected'}
        </Text>
        {connected && (
          <TouchableOpacity onPress={handleDisconnect} style={styles.disconnectBtn}>
            <Text style={styles.disconnectText}>Disconnect</Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Device status (when connected) */}
      {connected && (
        <View style={styles.statusCard}>
          <View style={styles.statusRow}>
            <Icon name="battery-full" size={20} color={status.battery < 20 ? '#f44336' : '#4caf50'} />
            <Text style={styles.statusText}>Battery: {status.battery}%</Text>
          </View>
          <View style={styles.statusRow}>
            <Icon name="wb-iridescent" size={20} color={status.laserOn ? '#f44336' : '#666'} />
            <Text style={styles.statusText}>Laser: {status.laserOn ? 'ON' : 'OFF'}</Text>
          </View>
          <View style={styles.statusRow}>
            <Icon name="speed" size={20} color="#00d4ff" />
            <Text style={styles.statusText}>FPS: {status.fps}</Text>
          </View>
          <View style={styles.statusRow}>
            <Icon name="thermostat" size={20} color="#ff9800" />
            <Text style={styles.statusText}>Temp: {status.tempC}°C</Text>
          </View>
          <View style={styles.statusRow}>
            <Icon name="memory" size={20} color="#9c27b0" />
            <Text style={styles.statusText}>Frames: {status.frameCount}</Text>
          </View>
        </View>
      )}

      {/* Scan button */}
      <TouchableOpacity style={styles.scanButton} onPress={handleScan} disabled={scanning}>
        {scanning ? (
          <ActivityIndicator color="#fff" />
        ) : (
          <Icon name="search" size={24} color="#fff" />
        )}
        <Text style={styles.scanButtonText}>
          {scanning ? 'Scanning...' : 'Scan for Devices'}
        </Text>
      </TouchableOpacity>

      {/* Device list */}
      <Text style={styles.sectionTitle}>Available Devices</Text>
      <DeviceList devices={devices} onConnect={handleConnect} connectedId={deviceId} />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 16 },
  banner: {
    flexDirection: 'row', alignItems: 'center', padding: 12,
    borderRadius: 8, marginBottom: 12,
  },
  connected: { backgroundColor: '#1b5e20' },
  disconnected: { backgroundColor: '#424242' },
  bannerText: { color: '#fff', fontSize: 14, marginLeft: 8, flex: 1 },
  disconnectBtn: { paddingHorizontal: 12, paddingVertical: 4, backgroundColor: '#c62828', borderRadius: 4 },
  disconnectText: { color: '#fff', fontSize: 12 },
  statusCard: {
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12,
  },
  statusRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 4 },
  statusText: { color: '#e0e0e0', fontSize: 14, marginLeft: 8 },
  scanButton: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'center',
    backgroundColor: '#0066cc', padding: 14, borderRadius: 8, marginBottom: 16,
  },
  scanButtonText: { color: '#fff', fontSize: 16, marginLeft: 8, fontWeight: '600' },
  sectionTitle: { color: '#888', fontSize: 12, marginBottom: 8, textTransform: 'uppercase' },
});