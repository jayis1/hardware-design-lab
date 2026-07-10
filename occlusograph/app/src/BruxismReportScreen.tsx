/*
 * BruxismReportScreen.tsx — Overnight bruxism summary and trends.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Collects bruxism events from the device, computes overnight summary
 * statistics (episodes, duration, peak force per tooth, tonic vs phasic
 * ratio), and displays a multi-week trend chart. Events are stored
 * locally in AsyncStorage between sessions.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ble, EventRecord } from './ble';

interface NightSummary {
  date: string;
  episodes: number;
  total_duration_min: number;
  peak_force_N: number;
  phasic_count: number;
  tonic_count: number;
  most_affected_tooth: number;
}

const CLASS_BRUX_PHASIC = 4;
const CLASS_BRUX_TONIC = 5;
const MAX_ELEMENT = 64;

export default function BruxismReportScreen(): React.ReactElement {
  const [events, setEvents] = useState<EventRecord[]>([]);
  const [summaries, setSummaries] = useState<NightSummary[]>([]);
  const [syncing, setSyncing] = useState(false);
  const [lastSync, setLastSync] = useState<string>('');

  useEffect(() => {
    loadSummaries();
    subscribeToEvents();
  }, []);

  const loadSummaries = async () => {
    try {
      const stored = await AsyncStorage.getItem('bruxism_summaries');
      if (stored) setSummaries(JSON.parse(stored));
    } catch (e) { console.warn('loadSummaries:', e); }
  };

  const saveSummaries = async (data: NightSummary[]) => {
    try {
      await AsyncStorage.setItem('bruxism_summaries', JSON.stringify(data));
    } catch (e) { console.warn('saveSummaries:', e); }
  };

  const subscribeToEvents = async () => {
    ble.subscribeEvents((evt: EventRecord) => {
      setEvents(prev => {
        const next = [...prev, evt];
        if (next.length > 5000) next.shift();
        return next;
      });
    });
  };

  const syncFromDevice = async () => {
    setSyncing(true);
    try {
      // Read battery and status first.
      const battery = await ble.readBattery();
      console.log('Device battery:', battery);
      // Events arrive via subscription; we just wait briefly.
      await new Promise(resolve => setTimeout(resolve, 2000));
      setLastSync(new Date().toLocaleTimeString());
      Alert.alert('Sync Complete', `${events.length} events received.`);
    } catch (e) {
      Alert.alert('Sync Failed', String(e));
    }
    setSyncing(false);
  };

  const generateReport = () => {
    if (events.length === 0) {
      Alert.alert('No Data', 'Sync events from the device first.');
      return;
    }
    // Compute summary for the current session.
    const bruxEvents = events.filter(
      e => e.class_id === CLASS_BRUX_PHASIC || e.class_id === CLASS_BRUX_TONIC
    );
    const phasic = bruxEvents.filter(e => e.class_id === CLASS_BRUX_PHASIC).length;
    const tonic = bruxEvents.filter(e => e.class_id === CLASS_BRUX_TONIC).length;
    const totalDur = bruxEvents.reduce((sum, e) => sum + e.duration_ms, 0) / 60000;
    const peakForce = Math.max(...bruxEvents.map(e => e.peak_force_mN), 0) / 1000;

    // Most affected tooth.
    const toothCounts = new Array(MAX_ELEMENT).fill(0);
    for (const e of bruxEvents) {
      if (e.peak_element < MAX_ELEMENT) toothCounts[e.peak_element]++;
    }
    const mostAffected = toothCounts.indexOf(Math.max(...toothCounts));

    const summary: NightSummary = {
      date: new Date().toLocaleDateString(),
      episodes: bruxEvents.length,
      total_duration_min: Math.round(totalDur * 10) / 10,
      peak_force_N: Math.round(peakForce * 10) / 10,
      phasic_count: phasic,
      tonic_count: tonic,
      most_affected_tooth: mostAffected,
    };

    const updated = [...summaries, summary];
    setSummaries(updated);
    saveSummaries(updated);
  };

  const exportPDF = () => {
    Alert.alert('Export', 'PDF export would generate a clinical report here.');
  };

  // ---- Current session stats ----
  const bruxEvents = events.filter(
    e => e.class_id === CLASS_BRUX_PHASIC || e.class_id === CLASS_BRUX_TONIC
  );
  const phasicCount = bruxEvents.filter(e => e.class_id === CLASS_BRUX_PHASIC).length;
  const tonicCount = bruxEvents.filter(e => e.class_id === CLASS_BRUX_TONIC).length;
  const totalDurationMin = bruxEvents.reduce((s, e) => s + e.duration_ms, 0) / 60000;
  const peakForceN = bruxEvents.length > 0
    ? Math.max(...bruxEvents.map(e => e.peak_force_mN)) / 1000 : 0;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Bruxism Report</Text>

      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.syncButton} onPress={syncFromDevice} disabled={syncing}>
          <Text style={styles.buttonText}>{syncing ? 'Syncing…' : 'Sync Events'}</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.reportButton} onPress={generateReport}>
          <Text style={styles.buttonText}>Save Report</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.exportButton} onPress={exportPDF}>
          <Text style={styles.buttonText}>Export PDF</Text>
        </TouchableOpacity>
      </View>

      {lastSync ? <Text style={styles.syncTime}>Last sync: {lastSync}</Text> : null}

      {/* Current session summary */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Current Session</Text>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Bruxism Episodes:</Text>
          <Text style={styles.statValue}>{bruxEvents.length}</Text>
        </View>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Total Duration:</Text>
          <Text style={styles.statValue}>{totalDurationMin.toFixed(1)} min</Text>
        </View>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Peak Force:</Text>
          <Text style={styles.statValue}>{peakForceN.toFixed(1)} N</Text>
        </View>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Phasic / Tonic:</Text>
          <Text style={styles.statValue}>{phasicCount} / {tonicCount}</Text>
        </View>
      </View>

      {/* Per-tooth force distribution bar chart */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Per-Tooth Force Distribution</Text>
        <View style={styles.barChart}>
          {Array.from({ length: 16 }).map((_, i) => {
            const elemBase = i * 4;
            const toothForce = bruxEvents
              .filter(e => e.peak_element >= elemBase && e.peak_element < elemBase + 4)
              .reduce((max, e) => Math.max(max, e.peak_force_mN), 0) / 1000;
            const h = Math.min((toothForce / 300) * 100, 100);
            return (
              <View key={i} style={styles.barCol}>
                <View style={[styles.bar, { height: h }]} />
                <Text style={styles.barLabel}>{i + 1}</Text>
              </View>
            );
          })}
        </View>
      </View>

      {/* Multi-week trend */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Trend (Recent Nights)</Text>
        {summaries.length === 0 ? (
          <Text style={styles.emptyText}>No saved reports yet.</Text>
        ) : (
          summaries.slice(-14).map((s, i) => (
            <View key={i} style={styles.trendRow}>
              <Text style={styles.trendDate}>{s.date}</Text>
              <Text style={styles.trendVal}>{s.episodes} ep</Text>
              <Text style={styles.trendVal}>{s.total_duration_min}m</Text>
              <Text style={styles.trendVal}>{s.peak_force_N}N</Text>
            </View>
          ))
        )}
      </View>

      <Text style={styles.footer}>Occlusograph · Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#0D47A1', padding: 16 },
  actionRow: { flexDirection: 'row', paddingHorizontal: 16, marginBottom: 8, gap: 8 },
  syncButton: { backgroundColor: '#1565C0', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  reportButton: { backgroundColor: '#2E7D32', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  exportButton: { backgroundColor: '#6A1B9A', padding: 10, borderRadius: 8, flex: 1, alignItems: 'center' },
  buttonText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  syncTime: { fontSize: 11, color: '#888', paddingHorizontal: 16 },
  card: { margin: 16, marginTop: 8, padding: 16, backgroundColor: '#fff', borderRadius: 12, elevation: 2 },
  cardTitle: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 12 },
  statRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  statLabel: { fontSize: 14, color: '#555' },
  statValue: { fontSize: 14, fontWeight: 'bold', color: '#0D47A1' },
  barChart: { flexDirection: 'row', height: 120, alignItems: 'flex-end', justifyContent: 'space-around' },
  barCol: { alignItems: 'center', flex: 1 },
  bar: { width: 16, backgroundColor: '#E53935', borderRadius: 4, minHeight: 2 },
  barLabel: { fontSize: 9, color: '#666', marginTop: 4 },
  emptyText: { fontSize: 14, color: '#aaa', textAlign: 'center', padding: 20 },
  trendRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#eee' },
  trendDate: { fontSize: 12, color: '#555', flex: 2 },
  trendVal: { fontSize: 12, fontWeight: 'bold', color: '#0D47A1', flex: 1, textAlign: 'center' },
  footer: { fontSize: 10, color: '#999', textAlign: 'center', padding: 16 },
});