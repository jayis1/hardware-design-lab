/**
 * RiskScreen — composite site risk score with explanation
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React from 'react';
import { View, Text, ScrollView, StyleSheet } from 'react-native';
import { useStore, riskScoreForProbe } from '../store';

function band(score: number) {
  if (score >= 70) return { label: 'CRITICAL', color: '#d32f2f', advice: 'Immediate field inspection advised. Rapid Q-drop detected — possible internal erosion.' };
  if (score >= 35) return { label: 'ELEVATED', color: '#fbc02d', advice: 'Monitor closely. Schedule a site visit within 24 hours.' };
  return { label: 'NORMAL', color: '#388e3c', advice: 'All chords within baseline. Continue routine schedule.' };
}

export default function RiskScreen() {
  const { probes } = useStore();
  return (
    <ScrollView style={s.wrap}>
      <Text style={s.title}>Site Risk Summary</Text>
      <Text style={s.sub}>Composite score from Q-rate-of-change, alert flags, and absolute saturation.</Text>
      {probes.length === 0 && <Text style={s.empty}>No probes.</Text>}
      {probes.map((p) => {
        const score = riskScoreForProbe(p.id);
        const b = band(score);
        return (
          <View key={p.id} style={s.card}>
            <View style={s.cardHead}>
              <Text style={s.cardTitle}>{p.name}</Text>
              <View style={[s.badge, { backgroundColor: b.color }]}>
                <Text style={s.badgeText}>{b.label} · {score}</Text>
              </View>
            </View>
            <Text style={s.advice}>{b.advice}</Text>
            <Text style={s.loc}>{p.site}</Text>
          </View>
        );
      })}
    </ScrollView>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0d1f17', padding: 12 },
  title: { color: '#e8f5e9', fontSize: 20, fontWeight: 'bold' },
  sub: { color: '#9ccc9c', fontSize: 12, marginBottom: 12 },
  empty: { color: '#9ccc9c', marginTop: 24, textAlign: 'center' },
  card: { backgroundColor: '#1a3a2a', borderRadius: 12, padding: 16, marginBottom: 12 },
  cardHead: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  cardTitle: { color: '#e8f5e9', fontSize: 16, fontWeight: 'bold' },
  badge: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 8 },
  badgeText: { color: '#fff', fontWeight: 'bold', fontSize: 12 },
  advice: { color: '#c8e6c9', fontSize: 12, marginTop: 8 },
  loc: { color: '#9ccc9c', fontSize: 11, marginTop: 4 },
});