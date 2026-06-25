/**
 * DashboardScreen.js — Main dashboard showing SVI for all pods
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, RefreshControl } from 'react-native';
import { useSelector, useDispatch } from 'react-redux';
import { SVIGauge } from '../components/SVIGauge';
import { PodCard } from '../components/PodCard';
import { TrendsChart } from '../components/TrendsChart';
import LoRaService from '../services/LoRaService';

export default function DashboardScreen({ navigation }) {
  const pods = useSelector(state => state.pods.pods);
  const dispatch = useDispatch();
  const [refreshing, setRefreshing] = useState(false);

  // Generate demo data if no pods exist
  useEffect(() => {
    if (pods.length === 0) {
      // Create demo pods
      const demoPods = [
        { id: 'pod-001', name: 'North Field A', svi: 72, batteryPct: 87, location: { lat: 48.1, lon: 11.5 } },
        { id: 'pod-002', name: 'Vineyard Row 3', svi: 64, batteryPct: 92, location: { lat: 48.101, lon: 11.501 } },
        { id: 'pod-003', name: 'Greenhouse Bed', svi: 88, batteryPct: 76, location: { lat: 48.102, lon: 11.502 } },
      ];
      demoPods.forEach(p => dispatch({ type: 'PODS_UPDATE_ONE', payload: p }));
    }
  }, []);

  const onRefresh = async () => {
    setRefreshing(true);
    // In production: fetch latest uplinks from LoRa gateway
    // For demo: simulate SVI drift
    pods.forEach(pod => {
      const newSvi = Math.max(0, Math.min(100, pod.svi + Math.floor(Math.random() * 10 - 5)));
      dispatch({ type: 'PODS_UPDATE_SVI', payload: { podId: pod.id, svi: newSvi } });
    });
    setRefreshing(false);
  };

  const avgSvi = pods.length > 0 ? Math.round(pods.reduce((s, p) => s + (p.svi || 0), 0) / pods.length) : 0;

  return (
    <ScrollView
      style={styles.container}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
    >
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>SoundLoom</Text>
        <Text style={styles.subtitle}>Bioacoustic Soil Health Monitor</Text>
        <Text style={styles.author}>Author: jayis1</Text>
      </View>

      {/* Overall SVI gauge */}
      <View style={styles.overallSection}>
        <Text style={styles.sectionTitle}>Field Average Vitality Index</Text>
        <SVIGauge value={avgSvi} size={180} />
        <Text style={styles.sviLabel}>{avgSvi}/100</Text>
      </View>

      {/* Trend chart */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>7-Day SVI Trend</Text>
        <TrendsChart pods={pods} days={7} />
      </View>

      {/* Pod list */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Active Pods ({pods.length})</Text>
        {pods.map(pod => (
          <PodCard
            key={pod.id}
            pod={pod}
            onPress={() => navigation.navigate('PodDetail', { podId: pod.id })}
          />
        ))}
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 20, backgroundColor: '#2E7D32', alignItems: 'center' },
  title: { fontSize: 28, fontWeight: 'bold', color: '#FFFFFF' },
  subtitle: { fontSize: 14, color: '#E8F5E9', marginTop: 4 },
  author: { fontSize: 10, color: '#C8E6C9', marginTop: 2 },
  overallSection: { alignItems: 'center', padding: 20, backgroundColor: '#FFFFFF', margin: 10, borderRadius: 12, elevation: 2 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#424242', marginBottom: 10 },
  sviLabel: { fontSize: 24, fontWeight: 'bold', color: '#2E7D32', marginTop: 8 },
  section: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12, elevation: 1 },
});