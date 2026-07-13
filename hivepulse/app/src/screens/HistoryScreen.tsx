/**
 * HistoryScreen — Historical data viewer and CSV export
 * Author: jayis1
 * License: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, ScrollView, StyleSheet, TouchableOpacity, Picker,
} from 'react-native';
import { useSelector } from 'react-redux';
import { AppState } from '../store/hiveStore';
import { ColonyState, HistoryPoint } from '../types';
import Sparkline from '../components/Sparkline';

export default function HistoryScreen() {
  const hives = useSelector((s: AppState) => s.hives);
  const [selectedHiveId, setSelectedHiveId] = useState(hives[0]?.id || '');
  const [dateRange, setDateRange] = useState('7d'); // 7d, 30d, 90d, 1y

  const selectedHive = hives.find(h => h.id === selectedHiveId);

  // Generate demo historical data
  const generateHistory = (): HistoryPoint[] => {
    const now = Date.now() / 1000;
    const points: HistoryPoint[] = [];
    const numPoints = dateRange === '7d' ? 168 : dateRange === '30d' ? 120 : 90;
    const interval = dateRange === '7d' ? 3600 :
                     dateRange === '30d' ? 21600 : 43200;

    for (let i = 0; i < numPoints; i++) {
      const ts = now - (numPoints - i) * interval;
      // Simulate state changes
      let state = ColonyState.QueenrightHealthy;
      if (i > numPoints * 0.7 && i < numPoints * 0.8) state = ColonyState.PreparingSwarm;
      if (i > numPoints * 0.85) state = ColonyState.QueenrightHealthy;

      points.push({
        timestamp: ts,
        colonyState: state,
        confidence: 0.85 + Math.random() * 0.1,
        weight: 35 + (i / numPoints) * 8 + Math.random() * 0.5,
        ambientTemp: 15 + Math.sin(i / 20) * 10,
        broodTemp: 34.5 + Math.random() * 0.5,
        beesIn: 50 + Math.floor(Math.random() * 100),
        beesOut: 50 + Math.floor(Math.random() * 100),
      });
    }
    return points;
  };

  const history = selectedHive ? generateHistory() : [];

  // Compute statistics
  const weightStart = history[0]?.weight || 0;
  const weightEnd = history[history.length - 1]?.weight || 0;
  const weightGain = weightEnd - weightStart;
  const avgBroodTemp = history.reduce((s, p) => s + p.broodTemp, 0) / (history.length || 1);
  const totalBeeTraffic = history.reduce((s, p) => s + p.beesIn + p.beesOut, 0);

  const weightData = history.map(p => p.weight);
  const tempData = history.map(p => p.ambientTemp);

  const handleExport = () => {
    // In production: generate CSV and share via ShareSheet
    console.log('Exporting CSV for hive', selectedHiveId);
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>📈 History & Reports</Text>

      {/* Hive selector */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Select Hive</Text>
        <View style={styles.pickerContainer}>
          {hives.map(hive => (
            <TouchableOpacity
              key={hive.id}
              style={[
                styles.hivePill,
                selectedHiveId === hive.id && styles.hivePillActive,
              ]}
              onPress={() => setSelectedHiveId(hive.id)}
            >
              <Text style={[
                styles.hivePillText,
                selectedHiveId === hive.id && styles.hivePillTextActive,
              ]}>
                {hive.name}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Date range selector */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Date Range</Text>
        <View style={styles.rangeRow}>
          {['7d', '30d', '90d', '1y'].map(range => (
            <TouchableOpacity
              key={range}
              style={[
                styles.rangeButton,
                dateRange === range && styles.rangeButtonActive,
              ]}
              onPress={() => setDateRange(range)}
            >
              <Text style={[
                styles.rangeText,
                dateRange === range && styles.rangeTextActive,
              ]}>
                {range === '7d' ? '7 days' : range === '30d' ? '30 days' :
                 range === '90d' ? '90 days' : '1 year'}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {selectedHive && (
        <>
          {/* Summary statistics */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>📊 Season Summary</Text>
            <View style={styles.statsGrid}>
              <View style={styles.statBox}>
                <Text style={styles.statBoxLabel}>Honey Gain</Text>
                <Text style={[styles.statBoxValue, { color: '#E8A317' }]}>
                  +{weightGain.toFixed(2)} kg
                </Text>
              </View>
              <View style={styles.statBox}>
                <Text style={styles.statBoxLabel}>Avg Brood Temp</Text>
                <Text style={styles.statBoxValue}>{avgBroodTemp.toFixed(1)}°C</Text>
              </View>
              <View style={styles.statBox}>
                <Text style={styles.statBoxLabel}>Total Traffic</Text>
                <Text style={styles.statBoxValue}>{totalBeeTraffic.toLocaleString()}</Text>
              </View>
              <View style={styles.statBox}>
                <Text style={styles.statBoxLabel}>Data Points</Text>
                <Text style={styles.statBoxValue}>{history.length}</Text>
              </View>
            </View>
          </View>

          {/* Weight chart */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>⚖️ Weight History</Text>
            <View style={styles.chartRow}>
              <Sparkline data={weightData} width={280} height={80} color="#E8A317" />
            </View>
            <Text style={styles.chartLabel}>
              {weightStart.toFixed(1)} kg → {weightEnd.toFixed(1)} kg
            </Text>
          </View>

          {/* Temperature chart */}
          <View style={styles.section}>
            <Text style={styles.sectionTitle}>🌡️ Temperature History</Text>
            <View style={styles.chartRow}>
              <Sparkline data={tempData} width={280} height={80} color="#FF6B35" />
            </View>
          </View>

          {/* Export button */}
          <TouchableOpacity style={styles.exportButton} onPress={handleExport}>
            <Text style={styles.exportButtonText}>📋 Export CSV</Text>
          </TouchableOpacity>
        </>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0F0F1E' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#E8A317', padding: 16 },
  section: { backgroundColor: '#1A1A2E', margin: 8, borderRadius: 12, padding: 14 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#E8A317', marginBottom: 10 },
  pickerContainer: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  hivePill: {
    paddingHorizontal: 12, paddingVertical: 8, borderRadius: 16,
    backgroundColor: '#0F0F1E', borderWidth: 1, borderColor: '#333',
  },
  hivePillActive: { backgroundColor: '#E8A317', borderColor: '#E8A317' },
  hivePillText: { fontSize: 13, color: '#888' },
  hivePillTextActive: { fontSize: 13, color: '#1A1A2E', fontWeight: 'bold' },
  rangeRow: { flexDirection: 'row', justifyContent: 'space-around' },
  rangeButton: { paddingHorizontal: 12, paddingVertical: 8, borderRadius: 8, backgroundColor: '#0F0F1E' },
  rangeButtonActive: { backgroundColor: '#E8A317' },
  rangeText: { fontSize: 13, color: '#888' },
  rangeTextActive: { fontSize: 13, color: '#1A1A2E', fontWeight: 'bold' },
  statsGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  statBox: { width: '48%', backgroundColor: '#0F0F1E', borderRadius: 8, padding: 10, marginBottom: 8 },
  statBoxLabel: { fontSize: 12, color: '#888' },
  statBoxValue: { fontSize: 20, fontWeight: 'bold', color: '#FFF', marginTop: 4 },
  chartRow: { alignItems: 'center' },
  chartLabel: { fontSize: 12, color: '#888', textAlign: 'center', marginTop: 4 },
  exportButton: {
    backgroundColor: '#4CAF50', borderRadius: 8, padding: 14,
    margin: 16, alignItems: 'center',
  },
  exportButtonText: { color: '#FFF', fontWeight: 'bold', fontSize: 16 },
});