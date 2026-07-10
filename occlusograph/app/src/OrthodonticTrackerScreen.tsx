/*
 * OrthodonticTrackerScreen.tsx — Longitudinal occlusal contact-pattern tracker.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Plots the change in occlusal contact patterns over the course of orthodontic
 * treatment. Shows expected vs. actual settling curves, contact-pattern change
 * per tooth, and alerts the orthodontist if relapse is detected.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, Alert } from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ble, EventRecord } from './ble';

interface TreatmentPoint {
  date: string;
  week: number;
  left_balance: number;   // 0-1, fraction of force on left side
  right_balance: number;  // 0-1, fraction of force on right side
  anterior_ratio: number; // 0-1, fraction of force on anterior teeth
  posterior_ratio: number;
  asymmetry_index: number; // 0-1, higher = worse
  contact_count: number;   // avg contacts per bite
}

const TOTAL_WEEKS = 104; // 2-year treatment default
const EXPECTED_ASYMMETRY_CURVE = [0.5, 0.42, 0.35, 0.3, 0.25, 0.22, 0.2, 0.18, 0.16, 0.15];

export default function OrthodonticTrackerScreen(): React.ReactElement {
  const [data, setData] = useState<TreatmentPoint[]>([]);
  const [events, setEvents] = useState<EventRecord[]>([]);
  const [recording, setRecording] = useState(false);

  useEffect(() => {
    loadData();
    ble.subscribeEvents((evt: EventRecord) => {
      if (recording) {
        setEvents(prev => [...prev, evt]);
      }
    });
  }, [recording]);

  const loadData = async () => {
    try {
      const stored = await AsyncStorage.getItem('ortho_data');
      if (stored) setData(JSON.parse(stored));
    } catch (e) { console.warn('loadData:', e); }
  };

  const saveData = async (newData: TreatmentPoint[]) => {
    try {
      await AsyncStorage.setItem('ortho_data', JSON.stringify(newData));
    } catch (e) { console.warn('saveData:', e); }
  };

  const recordSession = async () => {
    setRecording(true);
    Alert.alert('Recording', 'Recording occlusal session for 30 seconds.');
    setTimeout(() => {
      setRecording(false);
      computePoint();
    }, 30000);
  };

  const computePoint = () => {
    if (events.length === 0) {
      Alert.alert('No Data', 'No bite events recorded.');
      return;
    }

    // Left vs right: elements 0-31 = left, 32-63 = right.
    let leftForce = 0, rightForce = 0;
    let anteriorForce = 0, posteriorForce = 0;
    for (const e of events) {
      const f = e.peak_force_mN;
      if (e.peak_element < 32) leftForce += f;
      else rightForce += f;
      // Anterior: elements 48-63 (front teeth), posterior: 0-47.
      if (e.peak_element >= 48) anteriorForce += f;
      else posteriorForce += f;
    }
    const totalF = leftForce + rightForce || 1;
    const leftBalance = leftForce / totalF;
    const rightBalance = rightForce / totalF;
    const anteriorRatio = anteriorForce / totalF;
    const posteriorRatio = posteriorForce / totalF;
    const asymmetry = Math.abs(leftBalance - rightBalance);

    const week = data.length + 1;
    const point: TreatmentPoint = {
      date: new Date().toLocaleDateString(),
      week,
      left_balance: Math.round(leftBalance * 100) / 100,
      right_balance: Math.round(rightBalance * 100) / 100,
      anterior_ratio: Math.round(anteriorRatio * 100) / 100,
      posterior_ratio: Math.round(posteriorRatio * 100) / 100,
      asymmetry_index: Math.round(asymmetry * 100) / 100,
      contact_count: events.length,
    };

    const updated = [...data, point];
    setData(updated);
    saveData(updated);
    setEvents([]);

    // Relapse detection: if asymmetry increases by >0.1 over last 3 sessions.
    if (updated.length >= 4) {
      const recent = updated.slice(-3);
      const trend = recent[2].asymmetry_index - recent[0].asymmetry_index;
      if (trend > 0.1) {
        Alert.alert(
          '⚠ Relapse Alert',
          'Occlusal asymmetry has increased over the last 3 sessions. ' +
          'Consider scheduling a check-up.'
        );
      }
    }
  };

  const clearData = () => {
    Alert.alert('Clear Data', 'Remove all treatment history?', [
      { text: 'Cancel' },
      { text: 'Clear', onPress: () => { setData([]); saveData([]); } },
    ]);
  };

  // Expected asymmetry at current week.
  const expectedAsymmetry = data.length > 0 && data.length <= EXPECTED_ASYMMETRY_CURVE.length
    ? EXPECTED_ASYMMETRY_CURVE[data.length - 1]
    : 0.15;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Orthodontic Tracker</Text>

      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.recordButton} onPress={recordSession} disabled={recording}>
          <Text style={styles.buttonText}>
            {recording ? 'Recording…' : 'Record Session'}
          </Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.clearButton} onPress={clearData}>
          <Text style={styles.buttonText}>Clear</Text>
        </TouchableOpacity>
      </View>

      {/* Current metrics */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Latest Session</Text>
        {data.length === 0 ? (
          <Text style={styles.emptyText}>No sessions recorded yet.</Text>
        ) : (
          <>
            <View style={styles.statRow}>
              <Text style={styles.statLabel}>Week:</Text>
              <Text style={styles.statValue}>{data[data.length-1].week}</Text>
            </View>
            <View style={styles.statRow}>
              <Text style={styles.statLabel}>Left / Right:</Text>
              <Text style={styles.statValue}>
                {(data[data.length-1].left_balance * 100).toFixed(0)}% / {(data[data.length-1].right_balance * 100).toFixed(0)}%
              </Text>
            </View>
            <View style={styles.statRow}>
              <Text style={styles.statLabel}>Anterior / Posterior:</Text>
              <Text style={styles.statValue}>
                {(data[data.length-1].anterior_ratio * 100).toFixed(0)}% / {(data[data.length-1].posterior_ratio * 100).toFixed(0)}%
              </Text>
            </View>
            <View style={styles.statRow}>
              <Text style={styles.statLabel}>Asymmetry Index:</Text>
              <Text style={[
                styles.statValue,
                { color: data[data.length-1].asymmetry_index > 0.3 ? '#E53935' : '#2E7D32' },
              ]}>
                {data[data.length-1].asymmetry_index.toFixed(2)}
              </Text>
            </View>
            <View style={styles.statRow}>
              <Text style={styles.statLabel}>Expected Asymmetry:</Text>
              <Text style={styles.statValue}>{expectedAsymmetry.toFixed(2)}</Text>
            </View>
          </>
        )}
      </View>

      {/* Settling curve — actual vs expected asymmetry over weeks */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Settling Curve (Asymmetry)</Text>
        <View style={styles.chart}>
          {/* Expected curve (dotted line approximation with bars) */}
          <View style={styles.chartRow}>
            <Text style={styles.chartLabel}>Expected</Text>
            {EXPECTED_ASYMMETRY_CURVE.map((v, i) => (
              <View key={i} style={[styles.chartBarExpected, { height: v * 100 }]} />
            ))}
          </View>
          <View style={styles.chartRow}>
            <Text style={styles.chartLabel}>Actual</Text>
            {Array.from({ length: 10 }).map((_, i) => {
              const pt = data[i];
              const h = pt ? pt.asymmetry_index * 100 : 0;
              return <View key={i} style={[styles.chartBarActual, { height: h }]} />;
            })}
          </View>
        </View>
        <Text style={styles.chartCaption}>Green = expected · Blue = actual</Text>
      </View>

      {/* Full history table */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Treatment History</Text>
        <ScrollView style={styles.tableScroll} nestedScrollEnabled>
          {data.map((pt, i) => (
            <View key={i} style={styles.tableRow}>
              <Text style={styles.tableCell}>W{pt.week}</Text>
              <Text style={styles.tableCell}>{pt.date}</Text>
              <Text style={styles.tableCell}>{(pt.left_balance*100).toFixed(0)}/{(pt.right_balance*100).toFixed(0)}</Text>
              <Text style={styles.tableCell}>{pt.asymmetry_index.toFixed(2)}</Text>
            </View>
          ))}
        </ScrollView>
      </View>

      <Text style={styles.footer}>Occlusograph · Author: jayis1 · MIT License</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 22, fontWeight: 'bold', color: '#0D47A1', padding: 16 },
  actionRow: { flexDirection: 'row', paddingHorizontal: 16, marginBottom: 8, gap: 12 },
  recordButton: { backgroundColor: '#1565C0', padding: 12, borderRadius: 8, flex: 1, alignItems: 'center' },
  clearButton: { backgroundColor: '#757575', padding: 12, borderRadius: 8, alignItems: 'center' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  card: { margin: 16, marginTop: 8, padding: 16, backgroundColor: '#fff', borderRadius: 12, elevation: 2 },
  cardTitle: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 12 },
  statRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  statLabel: { fontSize: 14, color: '#555' },
  statValue: { fontSize: 14, fontWeight: 'bold', color: '#0D47A1' },
  emptyText: { fontSize: 14, color: '#aaa', textAlign: 'center', padding: 16 },
  chart: { marginTop: 8 },
  chartRow: { flexDirection: 'row', alignItems: 'flex-end', height: 60, marginBottom: 4 },
  chartLabel: { fontSize: 9, color: '#666', width: 50 },
  chartBarExpected: { width: 18, backgroundColor: '#81C784', marginHorizontal: 1, borderRadius: 3 },
  chartBarActual: { width: 18, backgroundColor: '#1565C0', marginHorizontal: 1, borderRadius: 3 },
  chartCaption: { fontSize: 10, color: '#999', marginTop: 4 },
  tableScroll: { maxHeight: 200 },
  tableRow: { flexDirection: 'row', paddingVertical: 4, borderBottomWidth: 0.5, borderBottomColor: '#eee' },
  tableCell: { fontSize: 11, color: '#555', flex: 1 },
  footer: { fontSize: 10, color: '#999', textAlign: 'center', padding: 16 },
});