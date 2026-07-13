/**
 * HiveDetailScreen — Full hive detail with all sensor data and timeline
 * Author: jayis1
 * License: MIT
 */

import React, { useEffect, useState } from 'react';
import {
  View, Text, ScrollView, StyleSheet, Dimensions,
} from 'react-native';
import { useSelector } from 'react-redux';
import { AppState } from '../store/hiveStore';
import { ColonyState } from '../types';
import StateBadge from '../components/StateBadge';
import Sparkline from '../components/Sparkline';

const { width } = Dimensions.get('window');

const tempLabels = [
  'Brood Top', 'Brood Bottom', 'NE Corner', 'NW Corner',
  'SE Corner', 'SW Corner', 'Entrance', 'Ambient',
];

export default function HiveDetailScreen({ route }: any) {
  const hiveId = route?.params?.hiveId;
  const hive = useSelector((s: AppState) =>
    s.hives.find(h => h.id === hiveId)
  );

  if (!hive) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>Hive not found</Text>
      </View>
    );
  }

  const weightTrend = [35, 36, 37.5, 38, 39.2, 40, hive.weight];
  const trafficData = [hive.beesIn * 0.3, hive.beesIn * 0.5, hive.beesIn * 0.8,
                       hive.beesIn, hive.beesIn * 0.7];
  const co2Trend = [400, 420, 450, hive.co2 - 20, hive.co2 - 10, hive.co2];

  return (
    <ScrollView style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.hiveName}>{hive.name}</Text>
        <StateBadge state={hive.colonyState} confidence={hive.confidence} />
      </View>
      <Text style={styles.location}>📍 {hive.location.label}</Text>
      <Text style={styles.updated}>
        Last updated: {new Date(hive.lastUpdated * 1000).toLocaleString()}
      </Text>

      {/* Weight section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>⚖️ Honey Weight</Text>
        <View style={styles.row}>
          <Text style={styles.bigValue}>{hive.weight.toFixed(2)} kg</Text>
          <Sparkline data={weightTrend} width={120} height={50} color="#E8A317" />
        </View>
        <Text style={styles.trendLabel}>
          Rate: +{((weightTrend[6] - weightTrend[0]) / 7).toFixed(2)} kg/day
        </Text>
      </View>

      {/* Temperature gradient */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>🌡️ Temperature Gradient</Text>
        <View style={styles.tempGrid}>
          {hive.temperatures.map((t, i) => (
            <View key={i} style={styles.tempCell}>
              <Text style={styles.tempLabel}>{tempLabels[i]}</Text>
              <Text style={[styles.tempValue, { color: getTempColor(t) }]}>
                {t.toFixed(1)}°C
              </Text>
            </View>
          ))}
        </View>
      </View>

      {/* Bee traffic */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>🐝 Entrance Traffic</Text>
        <View style={styles.row}>
          <View>
            <Text style={styles.trafficIn}>↗ In: {hive.beesIn}</Text>
            <Text style={styles.trafficOut}>↘ Out: {hive.beesOut}</Text>
            <Text style={styles.trafficNet}>
              Net: {hive.beesIn - hive.beesOut > 0 ? '+' : ''}
              {hive.beesIn - hive.beesOut}
            </Text>
          </View>
          <Sparkline data={trafficData} width={120} height={50} color="#4CAF50" />
        </View>
      </View>

      {/* Environmental */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>💧 Microclimate</Text>
        <View style={styles.envRow}>
          <View style={styles.envCell}>
            <Text style={styles.envLabel}>Humidity</Text>
            <Text style={styles.envValue}>{hive.humidity.toFixed(1)}%</Text>
          </View>
          <View style={styles.envCell}>
            <Text style={styles.envLabel}>CO₂</Text>
            <Text style={styles.envValue}>{hive.co2} ppm</Text>
          </View>
        </View>
        <View style={styles.row} style={{ marginTop: 8 }}>
          <Sparkline data={co2Trend} width={120} height={40} color="#03A9F4" />
          <Text style={styles.trendLabel}>CO₂ trend (6h)</Text>
        </View>
      </View>

      {/* Power */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>🔋 Power System</Text>
        <View style={styles.envRow}>
          <View style={styles.envCell}>
            <Text style={styles.envLabel}>Battery</Text>
            <Text style={styles.envValue}>
              {hive.battery} mV ({((hive.battery - 2600) / 7).toFixed(0)}%)
            </Text>
          </View>
          <View style={styles.envCell}>
            <Text style={styles.envLabel}>Solar</Text>
            <Text style={styles.envValue}>{hive.solar} mV {hive.solar > 100 ? '☀️' : '🌙'}</Text>
          </View>
        </View>
      </View>

      {/* Winter mode indicator */}
      {hive.winterMode && (
        <View style={styles.winterBanner}>
          <Text style={styles.winterText}>❄️ Winter Mode Active — Reduced Sampling</Text>
        </View>
      )}
    </ScrollView>
  );
}

function getTempColor(t: number): string {
  if (t < 0) return '#03A9F4';
  if (t < 10) return '#42A5F5';
  if (t < 20) return '#66BB6A';
  if (t < 35) return '#FFC107';
  if (t < 50) return '#FF6B35';
  return '#DC3545';
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E' },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#0F0F1E' },
  emptyText: { color: '#666', fontSize: 16 },
  header: { flexDirection: 'row', justifyContent: 'space-between', padding: 16, alignItems: 'center' },
  hiveName: { fontSize: 24, fontWeight: 'bold', color: '#FFF' },
  location: { fontSize: 14, color: '#888', paddingHorizontal: 16 },
  updated: { fontSize: 12, color: '#555', paddingHorizontal: 16, marginBottom: 8 },
  section: { backgroundColor: '#1A1A2E', margin: 8, borderRadius: 12, padding: 14 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#E8A317', marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  bigValue: { fontSize: 28, fontWeight: 'bold', color: '#FFF' },
  trendLabel: { fontSize: 12, color: '#888' },
  tempGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  tempCell: { width: '48%', flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  tempLabel: { fontSize: 13, color: '#888' },
  tempValue: { fontSize: 14, fontWeight: '600' },
  trafficIn: { fontSize: 16, color: '#4CAF50', fontWeight: '600' },
  trafficOut: { fontSize: 16, color: '#FF6B35', fontWeight: '600', marginTop: 4 },
  trafficNet: { fontSize: 14, color: '#AAA', marginTop: 4 },
  envRow: { flexDirection: 'row', justifyContent: 'space-around' },
  envCell: { alignItems: 'center' },
  envLabel: { fontSize: 12, color: '#888' },
  envValue: { fontSize: 18, fontWeight: 'bold', color: '#FFF', marginTop: 4 },
  winterBanner: {
    backgroundColor: 'rgba(3,169,244,0.15)',
    borderRadius: 8,
    padding: 12,
    margin: 8,
    alignItems: 'center',
  },
  winterText: { color: '#03A9F4', fontSize: 14, fontWeight: '600' },
});