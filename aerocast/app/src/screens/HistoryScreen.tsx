/**
 * HistoryScreen.tsx — calendar heatmap of total pollen load
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Card } from '../components/Card';
import { useDevice, TAXON_NAMES } from '../components/DeviceContext';

function heatColor(v: number, max: number) {
  const r = v / Math.max(1, max);
  if (r > 0.75) return '#C62828';
  if (r > 0.5)  return '#FF6F00';
  if (r > 0.25) return '#FFC107';
  if (r > 0.05) return '#4CAF50';
  return '#1f2b47';
}

export default function HistoryScreen() {
  const { history } = useDevice();
  const [selectedDay, setSelectedDay] = useState<string | null>(null);

  // Bucket history by hour for the last 24h (simple heatmap)
  const hourly = Array.from({ length: 24 }, () => 0);
  history.forEach(h => {
    const hr = new Date(h.epoch_min * 60000).getUTCHours();
    hourly[hr] += h.counts.reduce((a, b) => a + b, 0);
  });
  const max = Math.max(1, ...hourly);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>History — last 24h</Text>
      <Text style={styles.author}>AeroCast by jayis1</Text>

      <Card>
        <Text style={styles.section}>Hourly total particles (heatmap)</Text>
        <View style={styles.grid}>
          {hourly.map((v, h) => (
            <TouchableOpacity key={h} onPress={() => setSelectedDay(`${h}:00`)}>
              <View style={[styles.cell, { backgroundColor: heatColor(v, max) }]}>
                <Text style={styles.cellText}>{h}</Text>
                <Text style={styles.cellVal}>{v}</Text>
              </View>
            </TouchableOpacity>
          ))}
        </View>
        {selectedDay && (
          <Text style={styles.selected}>Selected hour: {selectedDay}</Text>
        )}
      </Card>

      <Card>
        <Text style={styles.section}>Cumulative per taxon (24h)</Text>
        {(() => {
          const sums = new Array(24).fill(0);
          history.forEach(h => h.counts.forEach((c, i) => sums[i] += c));
          return sums
            .map((c, i) => ({ i, c, name: TAXON_NAMES[i] }))
            .filter(x => x.i > 0 && x.c > 0)
            .sort((a, b) => b.c - a.c)
            .slice(0, 12)
            .map(r => (
              <View key={r.i} style={styles.row}>
                <Text style={styles.taxon}>{r.name}</Text>
                <Text style={styles.count}>{r.c}</Text>
              </View>
            ));
        })()}
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1020' },
  title: { color: '#fff', fontSize: 22, fontWeight: '700', margin: 14 },
  author: { color: '#5CB4FF', fontSize: 12, marginLeft: 14, marginBottom: 8 },
  section: { color: '#fff', fontSize: 15, fontWeight: '600', marginBottom: 8 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 4 },
  cell: { width: 58, height: 58, borderRadius: 6, justifyContent: 'center', alignItems: 'center' },
  cellText: { color: '#fff', fontSize: 10 },
  cellVal: { color: '#fff', fontSize: 12, fontWeight: '700' },
  selected: { color: '#5CB4FF', marginTop: 8, fontSize: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginVertical: 3 },
  taxon: { color: '#cde', fontSize: 13 },
  count: { color: '#fff', fontSize: 13, fontWeight: '600' },
});