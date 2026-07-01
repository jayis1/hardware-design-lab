/**
 * AlertsScreen — recent alert list with acknowledge/snooze
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { useStore, getAlerts } from '../store';
import { NCHORDS } from '../proto';

export default function AlertsScreen() {
  const { } = useStore();
  const alerts = getAlerts().slice(-100).reverse();

  const ack = (seq: number, chord: number) => {
    // In a real app, mark acknowledged in local state + gateway.
    // For the reference we just log.
  };

  return (
    <View style={s.wrap}>
      <Text style={s.title}>Alerts</Text>
      <Text style={s.sub}>Anomalies flagged by the probe firmware ({"|Z| > 3"} on f₀ or Q for ≥ 3 cycles).</Text>
      {alerts.length === 0 && <Text style={s.empty}>No active alerts. 🟢</Text>}
      <FlatList
        data={alerts}
        keyExtractor={(a, i) => `${a.probeId}-${a.seq}-${a.chord}-${i}`}
        renderItem={({ item }) => (
          <View style={s.card}>
            <View style={s.row}>
              <Text style={s.cardTitle}>Probe {item.probeId}</Text>
              <Text style={s.chord}>Chord {item.chord} ({(item.chord * 0.3).toFixed(1)} m)</Text>
            </View>
            <Text style={s.detail}>Seq {item.seq} · {new Date(item.ts).toLocaleString()}</Text>
            <View style={s.actions}>
              <TouchableOpacity style={s.btnAck} onPress={() => ack(item.seq, item.chord)}>
                <Text style={s.btnText}>Acknowledge</Text>
              </TouchableOpacity>
              <TouchableOpacity style={s.btnSnooze}>
                <Text style={s.btnText}>Snooze 1h</Text>
              </TouchableOpacity>
            </View>
          </View>
        )}
      />
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0d1f17', padding: 12 },
  title: { color: '#e8f5e9', fontSize: 22, fontWeight: 'bold' },
  sub: { color: '#9ccc9c', fontSize: 12, marginBottom: 12 },
  empty: { color: '#8bc34a', fontSize: 14, textAlign: 'center', marginTop: 40 },
  card: { backgroundColor: '#3e1a1a', borderRadius: 12, padding: 14, marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between' },
  cardTitle: { color: '#ffcdd2', fontWeight: 'bold', fontSize: 14 },
  chord: { color: '#ef9a9a', fontSize: 12 },
  detail: { color: '#e0c3c3', fontSize: 11, marginTop: 4 },
  actions: { flexDirection: 'row', marginTop: 10, gap: 8 },
  btnAck: { backgroundColor: '#d32f2f', padding: 8, borderRadius: 8 },
  btnSnooze: { backgroundColor: '#5d4037', padding: 8, borderRadius: 8 },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
});