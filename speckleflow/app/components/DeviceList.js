/**
 * DeviceList.js — BLE device list component
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, TouchableOpacity, FlatList, StyleSheet } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';

export default function DeviceList({ devices, onConnect, connectedId }) {
  const renderItem = ({ item }) => {
    const isConnected = item.id === connectedId;
    const name = item.name || item.advertising?.localName || 'Unknown Device';
    const rssi = item.rssi || 0;

    return (
      <TouchableOpacity
        style={[styles.deviceItem, isConnected && styles.connectedItem]}
        onPress={() => onConnect(item.id)}
        disabled={isConnected}
      >
        <Icon
          name={isConnected ? 'bluetooth-connected' : 'bluetooth'}
          size={24}
          color={isConnected ? '#4caf50' : '#00d4ff'}
        />
        <View style={styles.deviceInfo}>
          <Text style={styles.deviceName}>{name}</Text>
          <Text style={styles.deviceId}>{item.id}</Text>
        </View>
        <Text style={styles.rssi}>{rssi} dBm</Text>
      </TouchableOpacity>
    );
  };

  if (devices.length === 0) {
    return (
      <View style={styles.emptyContainer}>
        <Text style={styles.emptyText}>No devices found</Text>
        <Text style={styles.emptySubtext}>Make sure SpeckleFlow is powered on</Text>
      </View>
    );
  }

  return (
    <FlatList
      data={devices}
      keyExtractor={item => item.id}
      renderItem={renderItem}
      style={styles.list}
    />
  );
}

const styles = StyleSheet.create({
  list: { flex: 1 },
  deviceItem: {
    flexDirection: 'row', alignItems: 'center', padding: 12,
    backgroundColor: '#1a1a2e', borderRadius: 8, marginBottom: 4,
  },
  connectedItem: { backgroundColor: '#1b3a1b' },
  deviceInfo: { flex: 1, marginLeft: 12 },
  deviceName: { color: '#e0e0e0', fontSize: 14, fontWeight: '600' },
  deviceId: { color: '#666', fontSize: 11, marginTop: 2 },
  rssi: { color: '#888', fontSize: 12 },
  emptyContainer: { padding: 40, alignItems: 'center' },
  emptyText: { color: '#666', fontSize: 16 },
  emptySubtext: { color: '#444', fontSize: 12, marginTop: 4 },
});