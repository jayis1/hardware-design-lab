// src/screens/HistoryScreen.tsx — Measurement history list with filters
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useMemo, useCallback } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, RefreshControl,
} from 'react-native';
import { Card, Title, Paragraph, Chip, Searchbar, IconButton } from 'react-native-paper';
import { useNavigation } from '@react-navigation/native';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';
import type { RootStackParamList } from '../../App';
import { useBle } from '../ble/BleManager';
import { Measurement, interpretSpad } from '../ble/protocol';

type FilterType = 'all' | 'low_n' | 'moderate' | 'sufficient';

export default function HistoryScreen() {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();
  const { measurements, clearMeasurements } = useBle();
  const [search, setSearch] = useState('');
  const [filter, setFilter] = useState<FilterType>('all');
  const [refreshing, setRefreshing] = useState(false);

  const filteredMeasurements = useMemo(() => {
    let result = measurements.slice().reverse();
    if (filter === 'low_n') result = result.filter((m) => m.spad < 35);
    else if (filter === 'moderate') result = result.filter((m) => m.spad >= 35 && m.spad < 50);
    else if (filter === 'sufficient') result = result.filter((m) => m.spad >= 50);
    return result;
  }, [measurements, filter]);

  const refresh = useCallback(() => {
    setRefreshing(true);
    setTimeout(() => setRefreshing(false), 400);
  }, []);

  const renderItem = ({ item }: { item: Measurement }) => {
    const interp = interpretSpad(item.spad);
    const color = item.spad < 20 ? '#d32f2f' : item.spad < 35 ? '#f57c00' :
      item.spad < 50 ? '#fbc02d' : item.spad < 65 ? '#689f38' : '#2e7d32';

    return (
      <TouchableOpacity
        onPress={() => navigation.navigate('Measurement', { measurementId: String(item.timestampMs) })}
      >
        <Card style={styles.card}>
          <Card.Content style={styles.cardContent}>
            <View style={styles.leftSection}>
              <Text style={styles.spadText}>SPAD {item.spad}</Text>
              <Text style={styles.metaText}>
                NDVI {item.ndvi.toFixed(3)} · LWBI {item.lwbi.toFixed(3)}
              </Text>
              <Text style={styles.metaText}>
                {item.lat.toFixed(4)}, {item.lon.toFixed(4)}
              </Text>
              <Text style={styles.timeText}>
                {new Date(item.timestampMs).toLocaleString()}
              </Text>
            </View>
            <View style={styles.rightSection}>
              <Chip style={[styles.chip, { backgroundColor: color }]}>{interp.label}</Chip>
              <Text style={styles.tempText}>{item.tempC.toFixed(1)}°C</Text>
            </View>
          </Card.Content>
        </Card>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      {/* Filter chips */}
      <View style={styles.filterRow}>
        <Chip
          selected={filter === 'all'}
          onPress={() => setFilter('all')}
          style={[styles.filterChip, filter === 'all' && styles.filterActive]}
        >All ({measurements.length})</Chip>
        <Chip
          selected={filter === 'low_n'}
          onPress={() => setFilter('low_n')}
          style={[styles.filterChip, filter === 'low_n' && styles.filterActive]}
        >Low N</Chip>
        <Chip
          selected={filter === 'moderate'}
          onPress={() => setFilter('moderate')}
          style={[styles.filterChip, filter === 'moderate' && styles.filterActive]}
        >Moderate</Chip>
        <Chip
          selected={filter === 'sufficient'}
          onPress={() => setFilter('sufficient')}
          style={[styles.filterChip, filter === 'sufficient' && styles.filterActive]}
        >Sufficient</Chip>
      </View>

      <View style={styles.actionBar}>
        <Text style={styles.countText}>{filteredMeasurements.length} measurements</Text>
        <IconButton icon="delete-outline" color="#d32f2f" size={20} onPress={clearMeasurements} />
      </View>

      <FlatList
        data={filteredMeasurements}
        keyExtractor={(item, idx) => String(item.timestampMs) + '_' + idx}
        renderItem={renderItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No measurements yet</Text>
            <Text style={styles.emptySub}>Connect to device and take measurements</Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f0a', paddingTop: 8 },
  filterRow: { flexDirection: 'row', paddingHorizontal: 8, marginBottom: 8, flexWrap: 'wrap', gap: 4 },
  filterChip: { backgroundColor: '#152815', marginRight: 4 },
  filterActive: { backgroundColor: '#2e7d32' },
  actionBar: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingHorizontal: 12, marginBottom: 4 },
  countText: { color: '#81c784', fontSize: 13 },
  list: { paddingHorizontal: 8, paddingBottom: 16 },
  card: { backgroundColor: '#152815', marginBottom: 8 },
  cardContent: { flexDirection: 'row', justifyContent: 'space-between' },
  leftSection: { flex: 1 },
  rightSection: { alignItems: 'flex-end' },
  spadText: { color: '#e8f5e9', fontSize: 16, fontWeight: '600' },
  metaText: { color: '#81c784', fontSize: 12, marginTop: 2 },
  timeText: { color: '#b0bec5', fontSize: 11, marginTop: 4 },
  chip: { borderRadius: 12 },
  tempText: { color: '#b0bec5', fontSize: 12, marginTop: 4 },
  empty: { alignItems: 'center', paddingTop: 60 },
  emptyText: { color: '#e8f5e9', fontSize: 18 },
  emptySub: { color: '#81c784', fontSize: 13, marginTop: 8 },
});