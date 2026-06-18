/**
 * DashboardScreen.tsx — live per-taxon concentrations + 24h chart
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';
import { useDevice, TAXON_NAMES } from '../components/DeviceContext';

export default function DashboardScreen() {
  const { statusJson } = useBLE();
  const { history } = useDevice();

  // Use latest BLE status if available, else last history row.
  const latest = statusJson ?? (history.length ? history[history.length - 1] : null);
  if (!latest) {
    return (
      <View style={styles.container}>
        <Text style={styles.empty}>No live data yet — scan & connect an AeroCast unit.</Text>
      </View>
    );
  }

  const counts: number[] = latest.c ?? latest.counts ?? [];
  const total = counts.reduce((a:number,b:number)=>a+b,0);
  const flow = latest.flow ?? 0;
  const T = latest.T ?? 0;
  const RH = latest.RH ?? 0;

  // Top 8 taxa by count
  const ranked = counts
    .map((c:number, i:number) => ({ i, c, name: TAXON_NAMES[i] }))
    .filter(x => x.i > 0)
    .sort((a, b) => b.c - a.c)
    .slice(0, 8);

  // 24h stacked total series (simplified: total per minute)
  const series = history.slice(-60).map(h => h.counts.reduce((a,b)=>a+b,0));

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Live Air Bioaerosol</Text>
      <Text style={styles.author}>AeroCast by jayis1</Text>

      <Card>
        <Text style={styles.big}>{total} particles/min</Text>
        <Text style={styles.muted}>Flow {flow.toFixed(1)} L/min · {T.toFixed(1)}°C · {RH.toFixed(0)}% RH</Text>
      </Card>

      <Card>
        <Text style={styles.section}>Top taxa (last minute)</Text>
        {ranked.map(r => (
          <View key={r.i} style={styles.row}>
            <Text style={styles.taxon}>{r.name}</Text>
            <Text style={styles.count}>{r.c}</Text>
            <View style={styles.barWrap}>
              <View style={[styles.bar, { width: `${Math.min(100, r.c / Math.max(1, ranked[0].c) * 100)}%` }]} />
            </View>
          </View>
        ))}
      </Card>

      <Card>
        <Text style={styles.section}>Last 60 minutes (total particles)</Text>
        <View style={styles.chart}>
          {series.map((v, i) => (
            <View key={i} style={[styles.col, { height: `${Math.min(100, v / Math.max(1, ...series) * 100)}%` }]} />
          ))}
        </View>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1020' },
  title: { color: '#fff', fontSize: 22, fontWeight: '700', margin: 14 },
  author: { color: '#5CB4FF', fontSize: 12, marginLeft: 14, marginBottom: 4 },
  empty: { color: '#8aa', textAlign: 'center', marginTop: 80 },
  big: { color: '#00C7A0', fontSize: 28, fontWeight: '700' },
  muted: { color: '#8aa', fontSize: 13, marginTop: 4 },
  section: { color: '#fff', fontSize: 15, fontWeight: '600', marginBottom: 8 },
  row: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  taxon: { color: '#cde', flexBasis: 90, fontSize: 13 },
  count: { color: '#fff', width: 50, fontSize: 13 },
  barWrap: { flex: 1, height: 8, backgroundColor: '#1f2b47', borderRadius: 4 },
  bar: { height: 8, backgroundColor: '#5CB4FF', borderRadius: 4 },
  chart: { flexDirection: 'row', alignItems: 'flex-end', height: 80, gap: 2 },
  col: { flex: 1, backgroundColor: '#00C7A0', borderRadius: 2, minHeight: 2 },
});