// ============================================================
// LignoScan App — Scan List Screen
// History of all scans with date, GPS, tree ID, severity
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { useState, useEffect } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, StyleSheet,
  AsyncStorage, Alert,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { severityFromTDI } from '../utils/protocol';

export default function ScanListScreen({ navigation }) {
  const { tomogram, gpsData } = useBle();
  const [scans, setScans] = useState([]);

  // Load saved scans from storage
  useEffect(() => {
    loadScans();
  }, []);

  const loadScans = async () => {
    try {
      const stored = await AsyncStorage.getItem('lignoscan_scans');
      if (stored) {
        setScans(JSON.parse(stored));
      }
    } catch (e) {
      console.error('Failed to load scans', e);
    }
  };

  // Save current scan when a new tomogram arrives
  useEffect(() => {
    if (tomogram && gpsData) {
      saveCurrentScan();
    }
  }, [tomogram, gpsData]);

  const saveCurrentScan = async () => {
    const newScan = {
      id: Date.now().toString(),
      timestamp: gpsData?.timestamp || new Date().toISOString(),
      treeId: `TREE-${scans.length + 1}`,
      tdi: tomogram.tdi,
      severity: severityFromTDI(tomogram.tdi),
      gps: {
        lat: gpsData?.latitude || 0,
        lon: gpsData?.longitude || 0,
      },
      nCells: tomogram.nCells,
    };

    const updated = [newScan, ...scans];
    setScans(updated);

    try {
      await AsyncStorage.setItem('lignoscan_scans', JSON.stringify(updated));
    } catch (e) {
      Alert.alert('Save Error', 'Could not save scan to device storage.');
    }
  };

  const deleteScan = (id) => {
    Alert.alert(
      'Delete Scan',
      'Are you sure you want to delete this scan?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            const updated = scans.filter(s => s.id !== id);
            setScans(updated);
            await AsyncStorage.setItem('lignoscan_scans', JSON.stringify(updated));
          },
        },
      ]
    );
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={styles.scanItem}
      onPress={() => navigation.navigate('Report', { scanId: item.id })}
      onLongPress={() => deleteScan(item.id)}
    >
      <View style={styles.scanHeader}>
        <Text style={styles.treeId}>{item.treeId}</Text>
        <View style={[styles.severityBadge, { backgroundColor: item.severity.color }]}>
          <Text style={styles.severityText}>{item.severity.label}</Text>
        </View>
      </View>
      <Text style={styles.scanDate}>{item.timestamp}</Text>
      <View style={styles.scanDetails}>
        <Text style={styles.detailText}>TDI: {(item.tdi * 100).toFixed(1)}%</Text>
        <Text style={styles.detailText}>
          GPS: {item.gps.lat.toFixed(4)}, {item.gps.lon.toFixed(4)}
        </Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <FlatList
        data={scans}
        renderItem={renderItem}
        keyExtractor={item => item.id}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No scans yet</Text>
            <Text style={styles.emptySubtext}>
              Perform a scan to see it listed here.
            </Text>
          </View>
        }
      />
      <Text style={styles.footer}>
        Author: jayis1 — Copyright © 2026 — MIT License
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f0' },
  list: { padding: 12 },
  scanItem: {
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 10,
    marginBottom: 10,
    elevation: 1,
  },
  scanHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  treeId: { fontSize: 17, fontWeight: 'bold', color: '#1a3a1a' },
  severityBadge: {
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 12,
  },
  severityText: { color: '#fff', fontSize: 12, fontWeight: 'bold' },
  scanDate: { fontSize: 13, color: '#888', marginBottom: 6 },
  scanDetails: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  detailText: { fontSize: 13, color: '#555' },
  empty: { alignItems: 'center', paddingTop: 60 },
  emptyText: { fontSize: 18, color: '#999', marginBottom: 8 },
  emptySubtext: { fontSize: 14, color: '#aaa', textAlign: 'center' },
  footer: { textAlign: 'center', fontSize: 11, color: '#aaa', paddingVertical: 12 },
});

// EOF — ScanListScreen.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT