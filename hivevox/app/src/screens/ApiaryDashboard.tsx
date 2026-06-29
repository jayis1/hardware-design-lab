/*
 * screens/ApiaryDashboard.tsx — Overview of all hives in the apiary
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet, FlatList, RefreshControl,
         ScrollView } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import type { HiveData } from '../App';
import HiveCard from '../components/HiveCard';
import { hivesNeedingAttention, totalHoneyWeight,
         averageBroodTemp, stateName } from '../components/StateUtils';

type Props = {
  hives: HiveData[];
  onSelectHive: (deveui: string) => void;
};

export default function ApiaryDashboard({ hives, onSelectHive }: Props) {
  const [refreshing, setRefreshing] = useState(false);
  const attentionHives = hivesNeedingAttention(hives);
  const honey = totalHoneyWeight(hives);
  const avgTemp = averageBroodTemp(hives);

  const onRefresh = async () => {
    setRefreshing(true);
    // The parent App component polls every 30s; this is a manual trigger
    setTimeout(() => setRefreshing(false), 1000);
  };

  const renderSummary = () => (
    <View style={styles.summaryContainer}>
      <View style={styles.summaryCard}>
        <Ionicons name="cube" size={24} color="#e8a800" />
        <Text style={styles.summaryValue}>{hives.length}</Text>
        <Text style={styles.summaryLabel}>Total Hives</Text>
      </View>
      <View style={styles.summaryCard}>
        <Ionicons name="warning" size={24} color="#f44336" />
        <Text style={styles.summaryValue}>{attentionHives.length}</Text>
        <Text style={styles.summaryLabel}>Need Attention</Text>
      </View>
      <View style={styles.summaryCard}>
        <Ionicons name="nutrition" size={24} color="#e8a800" />
        <Text style={styles.summaryValue}>{(honey / 1000).toFixed(1)}</Text>
        <Text style={styles.summaryLabel}>Honey (kg est.)</Text>
      </View>
      <View style={styles.summaryCard}>
        <Ionicons name="thermometer" size={24} color="#4caf50" />
        <Text style={styles.summaryValue}>{avgTemp.toFixed(1)}</Text>
        <Text style={styles.summaryLabel}>Avg Brood °C</Text>
      </View>
    </View>
  );

  const renderAttention = () => {
    if (attentionHives.length === 0) return null;
    return (
      <View style={styles.attentionBox}>
        <Text style={styles.attentionTitle}>
          ⚠ {attentionHives.length} hive(s) need attention:
        </Text>
        {attentionHives.map((h) => (
          <Text key={h.deveui} style={styles.attentionItem}>
            • {h.name}: {stateName(h.state)}
          </Text>
        ))}
      </View>
    );
  };

  if (hives.length === 0) {
    return (
      <View style={styles.empty}>
        <Ionicons name="add-circle" size={64} color="#ccc" />
        <Text style={styles.emptyText}>No hives added yet</Text>
        <Text style={styles.emptySub}>
          Go to Settings → Add Hive and scan the QR code on your HiveVox device.
        </Text>
      </View>
    );
  }

  return (
    <FlatList
      data={hives}
      keyExtractor={(item) => item.deveui}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
      }
      ListHeaderComponent={
        <View>
          <Text style={styles.title}>Apiary Dashboard</Text>
          {renderSummary()}
          {renderAttention()}
          <Text style={styles.sectionTitle}>Hives</Text>
        </View>
      }
      renderItem={({ item }) => (
        <HiveCard hive={item} onPress={onSelectHive} />
      )}
      contentContainerStyle={styles.list}
    />
  );
}

const styles = StyleSheet.create({
  title: {
    fontSize: 24, fontWeight: '800', color: '#222',
    margin: 16, marginBottom: 8,
  },
  summaryContainer: {
    flexDirection: 'row', justifyContent: 'space-around',
    paddingHorizontal: 12, marginBottom: 12,
  },
  summaryCard: {
    alignItems: 'center', backgroundColor: '#fff',
    borderRadius: 10, padding: 10, flex: 1, marginHorizontal: 4,
    elevation: 1, shadowColor: '#000', shadowOpacity: 0.05,
    shadowRadius: 2, shadowOffset: { width: 0, height: 1 },
  },
  summaryValue: { fontSize: 22, fontWeight: '700', color: '#222', marginTop: 4 },
  summaryLabel: { fontSize: 10, color: '#888', marginTop: 2 },
  attentionBox: {
    backgroundColor: '#fff3e0', borderRadius: 8,
    marginHorizontal: 16, marginBottom: 12, padding: 12,
    borderLeftWidth: 4, borderLeftColor: '#ff9800',
  },
  attentionTitle: { fontSize: 14, fontWeight: '600', color: '#e65100' },
  attentionItem: { fontSize: 13, color: '#bf360c', marginTop: 4 },
  sectionTitle: {
    fontSize: 16, fontWeight: '700', color: '#555',
    marginLeft: 16, marginBottom: 4,
  },
  list: { paddingBottom: 20 },
  empty: {
    flex: 1, justifyContent: 'center', alignItems: 'center', padding: 40,
  },
  emptyText: { fontSize: 18, fontWeight: '600', color: '#888', marginTop: 12 },
  emptySub: { fontSize: 14, color: '#aaa', textAlign: 'center', marginTop: 8 },
});