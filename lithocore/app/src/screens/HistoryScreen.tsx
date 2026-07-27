/**
 * HistoryScreen.tsx — List of all tested cells with sortable history.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  RefreshControl,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { useDatabase, CellRecord } from '../db/database';
import {
  DegradationMode,
  DEGRADATION_NAMES,
  VERDICT_COLORS,
  CHEMISTRY_NAMES,
} from '../ble/protocol';

type SortField = 'date' | 'soh' | 'chemistry';

export default function HistoryScreen() {
  const [records, setRecords] = useState<CellRecord[]>([]);
  const [refreshing, setRefreshing] = useState(false);
  const [sortBy, setSortBy] = useState<SortField>('date');
  const db = useDatabase();
  const navigation = useNavigation();

  const loadHistory = useCallback(async () => {
    const history = await db.getHistory();
    let sorted = [...history];
    if (sortBy === 'soh') {
      sorted.sort((a, b) => a.sohScore - b.sohScore);
    } else if (sortBy === 'chemistry') {
      sorted.sort((a, b) => a.chemistryIdx - b.chemistryIdx);
    }
    setRecords(sorted);
  }, [db, sortBy]);

  useEffect(() => {
    loadHistory();
  }, [loadHistory]);

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    await loadHistory();
    setRefreshing(false);
  }, [loadHistory]);

  const renderItem = ({ item }: { item: CellRecord }) => {
    const verdictColor = VERDICT_COLORS[item.verdict as keyof typeof VERDICT_COLORS];
    const date = new Date(item.timestamp);
    return (
      <TouchableOpacity
        style={styles.recordCard}
        onPress={() => navigation.navigate('CellReport', { cellId: String(item.id) })}
      >
        <View style={styles.recordHeader}>
          <Text style={styles.recordDate}>
            {date.toLocaleDateString()} {date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
          </Text>
          <View style={[styles.sohBadge, { backgroundColor: verdictColor }]}>
            <Text style={styles.sohBadgeText}>{item.sohScore}%</Text>
          </View>
        </View>
        <View style={styles.recordDetails}>
          <Text style={styles.recordDetail}>
            {CHEMISTRY_NAMES[item.chemistryIdx] || 'Unknown'}
          </Text>
          <Text style={styles.recordDetail}>
            {DEGRADATION_NAMES[item.degradation as DegradationMode] || 'Unknown'}
          </Text>
          <Text style={styles.recordDetail}>DCIR: {item.dcirMohm} mΩ</Text>
          <Text style={styles.recordDetail}>OCV: {(item.ocvMv / 1000).toFixed(2)} V</Text>
        </View>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      {/* Sort controls */}
      <View style={styles.sortBar}>
        <Text style={styles.sortLabel}>Sort by:</Text>
        {(['date', 'soh', 'chemistry'] as SortField[]).map((field) => (
          <TouchableOpacity
            key={field}
            style={[styles.sortButton, sortBy === field && styles.sortButtonActive]}
            onPress={() => setSortBy(field)}
          >
            <Text style={[styles.sortButtonText, sortBy === field && styles.sortButtonTextActive]}>
              {field.charAt(0).toUpperCase() + field.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <FlatList
        data={records}
        keyExtractor={(item) => String(item.id)}
        renderItem={renderItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No cell tests yet.</Text>
            <Text style={styles.emptySubtext}>Run a sweep from the Live Sweep screen.</Text>
          </View>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#12122a' },
  sortBar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    backgroundColor: '#1a1a2e',
    gap: 8,
  },
  sortLabel: { color: '#8888aa', fontSize: 13 },
  sortButton: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 6,
    backgroundColor: '#222244',
  },
  sortButtonActive: { backgroundColor: '#0066cc' },
  sortButtonText: { color: '#8888aa', fontSize: 12 },
  sortButtonTextActive: { color: '#fff', fontWeight: '600' },
  list: { padding: 12 },
  recordCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 16,
    marginBottom: 8,
  },
  recordHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 8,
  },
  recordDate: { color: '#e0e0e0', fontSize: 13, fontWeight: '600' },
  sohBadge: {
    paddingHorizontal: 12,
    paddingVertical: 4,
    borderRadius: 12,
  },
  sohBadgeText: { color: '#000', fontSize: 14, fontWeight: 'bold' },
  recordDetails: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 12,
  },
  recordDetail: { color: '#8888aa', fontSize: 12 },
  empty: { alignItems: 'center', marginTop: 60 },
  emptyText: { color: '#666688', fontSize: 16 },
  emptySubtext: { color: '#444466', fontSize: 13, marginTop: 4 },
});