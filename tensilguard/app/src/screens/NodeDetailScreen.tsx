/**
 * NodeDetailScreen — time-series plots and AE timeline for one node
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Fetches the node's measurement history and AE events from the gateway
 * and renders simple text-based plots (tension vs time, AE timeline).
 * In a production build these would be real charts (e.g. victory-native
 * or d3); here we use a compact ASCII-sparkline style for portability.
 */
import React, { useEffect, useState, useCallback } from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity } from 'react-native';
import { RouteProp, useRoute } from '@react-navigation/native';
import type { RootStackParamList } from '../../App';
import { fetchNodeHistory, fetchAeEvents, sendDownlink, type AeEventLog, type NodeHistory } from '../api';
import { encodeSetInterval, aeLabel } from '../proto';

type RouteProps = RouteProp<RootStackParamList, 'NodeDetail'>;

function sparkline(values: number[], width = 40): string {
  if (values.length === 0) return '';
  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = max - min || 1;
  const chars = '▁▂▃▄▅▆▇█';
  const step = Math.max(1, Math.floor(values.length / width));
  let out = '';
  for (let i = 0; i < values.length; i += step) {
    const v = values[i];
    const idx = Math.floor((v - min) / range * 7);
    out += chars[Math.max(0, Math.min(7, idx))];
  }
  return out;
}

export default function NodeDetailScreen() {
  const route = useRoute<RouteProps>();
  const nodeId = route.params.nodeId;
  const [history, setHistory] = useState<NodeHistory | null>(null);
  const [events, setEvents] = useState<AeEventLog[]>([]);
  const [interval, setIntervalInput] = useState(1800);

  const load = useCallback(async () => {
    const now = Math.floor(Date.now() / 1000);
    const from = now - 86400; // last 24 h
    const h = await fetchNodeHistory(nodeId, from, now);
    setHistory(h);
    const e = await fetchAeEvents(nodeId, 50);
    setEvents(e);
  }, [nodeId]);

  useEffect(() => { load(); }, [load]);

  const setIntervalCmd = async () => {
    await sendDownlink(nodeId, encodeSetInterval(interval));
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Node {nodeId}</Text>
        <Text style={styles.sub}>Tension (last 24 h)</Text>
        {history ? (
          <View>
            <Text style={styles.plotLabel}>T_mag (magnetoelastic):</Text>
            <Text style={styles.spark}>{sparkline(history.tMag)}</Text>
            <Text style={styles.plotLabel}>T_vib (vibration):</Text>
            <Text style={styles.spark}>{sparkline(history.tVib)}</Text>
            <Text style={styles.plotLabel}>ΔT (disagreement):</Text>
            <Text style={styles.spark}>{sparkline(history.dt)}</Text>
            <Text style={styles.plotLabel}>Temperature (°C):</Text>
            <Text style={styles.spark}>{sparkline(history.temp)}</Text>
            <Text style={styles.plotLabel}>Battery (%):</Text>
            <Text style={styles.spark}>{sparkline(history.battery)}</Text>
            <Text style={styles.plotLabel}>f₁ (Hz):</Text>
            <Text style={styles.spark}>{sparkline(history.f1)}</Text>
          </View>
        ) : (
          <Text style={styles.empty}>Loading history…</Text>
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>Wire-Break Timeline</Text>
        {events.length === 0 ? (
          <Text style={styles.empty}>No AE events recorded.</Text>
        ) : (
          events.map((e, i) => (
            <View key={i} style={[styles.aeRow, { borderLeftColor: e.classification === 3 ? '#d32f2f' : '#8a9ab0' }]}>
              <Text style={styles.aeTime}>{new Date(e.unixTime * 1000).toLocaleString()}</Text>
              <Text style={styles.aeLabel}>{aeLabel(e.classification)} (score {e.score})</Text>
              <Text style={styles.aeDetail}>
                peak {e.peakUv.toFixed(0)} µV · rise {e.riseUs.toFixed(0)} µs · dur {e.durUs.toFixed(0)} µs · {e.centroidKhz.toFixed(0)} kHz
              </Text>
            </View>
          ))
        )}
      </View>

      <View style={styles.card}>
        <Text style={styles.title}>Configuration</Text>
        <Text style={styles.sub}>Measurement interval: {interval}s</Text>
        <View style={styles.intervalRow}>
          {[300, 600, 1800, 3600, 7200].map((s) => (
            <TouchableOpacity
              key={s}
              style={[styles.intervalBtn, interval === s && styles.intervalBtnActive]}
              onPress={() => setIntervalInput(s)}
            >
              <Text style={styles.intervalBtnText}>{s / 60}m</Text>
            </TouchableOpacity>
          ))}
        </View>
        <TouchableOpacity style={styles.sendBtn} onPress={setIntervalCmd}>
          <Text style={styles.sendBtnText}>Send to node</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a', padding: 8 },
  card: { backgroundColor: '#142535', borderRadius: 8, padding: 16, margin: 8 },
  title: { color: '#e8f0f8', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  sub: { color: '#8a9ab0', fontSize: 13, marginBottom: 8 },
  plotLabel: { color: '#6a8aa0', fontSize: 11, marginTop: 8 },
  spark: { color: '#4fc3f7', fontSize: 14, fontFamily: 'monospace', marginTop: 2 },
  empty: { color: '#8a9ab0', fontSize: 13, textAlign: 'center', padding: 20 },
  aeRow: { borderLeftWidth: 3, paddingLeft: 10, paddingVertical: 6, marginVertical: 2 },
  aeTime: { color: '#8a9ab0', fontSize: 11 },
  aeLabel: { color: '#e8f0f8', fontSize: 13, fontWeight: '600' },
  aeDetail: { color: '#6a8aa0', fontSize: 11 },
  intervalRow: { flexDirection: 'row', gap: 8, marginVertical: 12 },
  intervalBtn: { backgroundColor: '#1a3a5a', padding: 8, borderRadius: 6 },
  intervalBtnActive: { backgroundColor: '#2196f3' },
  intervalBtnText: { color: '#e8f0f8', fontSize: 12 },
  sendBtn: { backgroundColor: '#2196f3', padding: 12, borderRadius: 8, alignItems: 'center', marginTop: 8 },
  sendBtnText: { color: '#fff', fontWeight: '600' },
});