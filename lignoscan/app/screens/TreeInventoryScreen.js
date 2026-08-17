// ============================================================
// LignoScan App — Tree Inventory Screen
// Map view of all scanned trees with severity color coding
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { useState, useEffect } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView, AsyncStorage, FlatList,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { severityFromTDI } from '../utils/protocol';

export default function TreeInventoryScreen({ navigation }) {
  const [scans, setScans] = useState([]);
  const [filter, setFilter] = useState('all'); // all, low, moderate, high, critical

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

  const filteredScans = scans.filter(s => {
    if (filter === 'all') return true;
    return s.severity.label.toLowerCase().includes(filter);
  });

  // Statistics
  const stats = {
    total: scans.length,
    low: scans.filter(s => s.tdi < 0.1).length,
    moderate: scans.filter(s => s.tdi >= 0.1 && s.tdi < 0.25).length,
    high: scans.filter(s => s.tdi >= 0.25 && s.tdi < 0.5).length,
    critical: scans.filter(s => s.tdi >= 0.5).length,
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={styles.treeItem}
      onPress={() => navigation.navigate('Report', { scanId: item.id })}
    >
      <View style={[styles.treePin, { backgroundColor: item.severity.color }]} />
      <View style={styles.treeInfo}>
        <Text style={styles.treeId}>{item.treeId}</Text>
        <Text style={styles.treeGps}>
          {item.gps.lat.toFixed(4)}, {item.gps.lon.toFixed(4)}
        </Text>
      </View>
      <View style={styles.treeRisk}>
        <Text style={[styles.riskLabel, { color: item.severity.color }]}>
          {item.severity.label}
        </Text>
        <Text style={styles.tdiText}>TDI: {(item.tdi * 100).toFixed(1)}%</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <ScrollView style={styles.container}>
      {/* Statistics Summary */}
      <View style={styles.statsCard}>
        <Text style={styles.statsTitle}>Inventory Summary</Text>
        <View style={styles.statsGrid}>
          <View style={styles.statItem}>
            <Text style={styles.statValue}>{stats.total}</Text>
            <Text style={styles.statLabel}>Total Trees</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={[styles.statValue, { color: '#2d8a2d' }]}>{stats.low}</Text>
            <Text style={styles.statLabel}>Low Risk</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={[styles.statValue, { color: '#e6c200' }]}>{stats.moderate}</Text>
            <Text style={styles.statLabel}>Moderate</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={[styles.statValue, { color: '#e65c00' }]}>{stats.high}</Text>
            <Text style={styles.statLabel}>High Risk</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={[styles.statValue, { color: '#cc0000' }]}>{stats.critical}</Text>
            <Text style={styles.statLabel}>Critical</Text>
          </View>
        </View>
      </View>

      {/* Filter Buttons */}
      <View style={styles.filterRow}>
        {['all', 'low', 'moderate', 'high', 'critical'].map(f => (
          <TouchableOpacity
            key={f}
            style={[styles.filterButton, filter === f && styles.filterButtonActive]}
            onPress={() => setFilter(f)}
          >
            <Text style={[styles.filterText, filter === f && styles.filterTextActive]}>
              {f.charAt(0).toUpperCase() + f.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Tree List (simulated map) */}
      <Text style={styles.sectionTitle}>
        Trees ({filteredScans.length})
      </Text>

      {filteredScans.length > 0 ? (
        <FlatList
          data={filteredScans}
          renderItem={renderItem}
          keyExtractor={item => item.id}
          scrollEnabled={false}
          contentContainerStyle={styles.list}
        />
      ) : (
        <View style={styles.empty}>
          <Text style={styles.emptyText}>No trees in inventory</Text>
          <Text style={styles.emptySubtext}>
            Perform scans to build your tree inventory.{'\n'}
            Each scan is automatically geotagged and added here.
          </Text>
        </View>
      )}

      <Text style={styles.footer}>
        Author: jayis1 — Copyright © 2026 — MIT License
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f0' },
  statsCard: {
    backgroundColor: '#fff',
    margin: 12,
    padding: 16,
    borderRadius: 10,
    elevation: 2,
  },
  statsTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#1a3a1a',
    marginBottom: 12,
  },
  statsGrid: {
    flexDirection: 'row',
    justifyContent: 'space-around',
  },
  statItem: { alignItems: 'center' },
  statValue: { fontSize: 24, fontWeight: 'bold', color: '#333' },
  statLabel: { fontSize: 11, color: '#888', marginTop: 4 },
  filterRow: {
    flexDirection: 'row',
    paddingHorizontal: 12,
    marginBottom: 8,
    gap: 6,
  },
  filterButton: {
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderRadius: 16,
    backgroundColor: '#e0e0d0',
  },
  filterButtonActive: {
    backgroundColor: '#1a3a1a',
  },
  filterText: { fontSize: 12, color: '#555' },
  filterTextActive: { color: '#fff', fontWeight: 'bold' },
  sectionTitle: {
    fontSize: 15,
    fontWeight: 'bold',
    color: '#333',
    paddingHorizontal: 16,
    marginBottom: 8,
  },
  list: { paddingHorizontal: 12 },
  treeItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#fff',
    padding: 12,
    borderRadius: 8,
    marginBottom: 6,
    elevation: 1,
  },
  treePin: {
    width: 14,
    height: 14,
    borderRadius: 7,
    marginRight: 12,
  },
  treeInfo: { flex: 1 },
  treeId: { fontSize: 15, fontWeight: 'bold', color: '#1a3a1a' },
  treeGps: { fontSize: 12, color: '#888', marginTop: 2 },
  treeRisk: { alignItems: 'flex-end' },
  riskLabel: { fontSize: 13, fontWeight: 'bold' },
  tdiText: { fontSize: 12, color: '#888', marginTop: 2 },
  empty: { alignItems: 'center', paddingTop: 40, paddingBottom: 60 },
  emptyText: { fontSize: 18, color: '#999', marginBottom: 8 },
  emptySubtext: { fontSize: 14, color: '#aaa', textAlign: 'center' },
  footer: { textAlign: 'center', fontSize: 11, color: '#aaa', paddingVertical: 12 },
});

// EOF — TreeInventoryScreen.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT