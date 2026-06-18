/**
 * TaxaDetailScreen.tsx - Per-taxon detail with time-series + heatmap
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useMemo } from 'react';
import {
  View, Text, StyleSheet, ScrollView, Picker,
} from 'react-native';
import { useStation, makeDemoStation } from '../context/StationContext';
import { TAXA_NAMES, pollenIndex } from '../types';

export default function TaxaDetailScreen() {
  const { station: live } = useStation();
  const station = live ?? useMemo(() => makeDemoStation(), []);
  const [selected, setSelected] = useState(station.taxa[0]?.classId ?? 30);

  const taxa = station.taxa.find((t) => t.classId === selected);
  const fcRow = station.forecast[0] ?? [];
  const maxFc = Math.max(1, ...fcRow);
  const idx = pollenIndex(taxa?.concentration ?? 0);

  /* 24-cell calendar heatmap (last 24 h, synthetic) */
  const heatmap = useMemo(() => {
    const arr: number[] = [];
    for (let i = 0; i < 24; i++) {
      const base = taxa?.concentration ?? 10;
      arr.push(Math.max(0, base * (0.6 + 0.8 * Math.abs(Math.sin(i / 3)))));
    }
    return arr;
  }, [taxa]);
  const hmMax = Math.max(1, ...heatmap);

  return (
    <ScrollView style={s.container}>
      <Text style={s.title}>Taxa Detail</Text>
      <View style={s.pickerWrap}>
        <Picker
          selectedValue={selected}
          onValueChange={(v) => setSelected(v as number)}
          style={s.picker}
        >
          {TAXA_NAMES.map((n, i) => (
            <Picker.Item key={i} label={n} value={i} />
          ))}
        </Picker>
      </View>

      {taxa && (
        <View style={s.card}>
          <Text style={s.cardTitle}>{taxa.name}</Text>
          <Text style={s.bigValue}>{Math.round(taxa.concentration)} grains/m³</Text>
          <Text style={s.conf}>Confidence: {Math.round(taxa.confidence * 100)}%</Text>
          <View style={[s.idxPill, { backgroundColor: IDX_COLORS[idx] }]}>
            <Text style={s.idxText}>{POLLEN_INDEX_LABELS[idx]}</Text>
          </View>
        </View>
      )}

      <Text style={s.sectionTitle}>72-hour forecast</Text>
      <View style={s.chartRow}>
        {fcRow.map((v, i) => (
          <View
            key={i}
            style={[s.bar, { height: 10 + (v / maxFc) * 80 }]}
          />
        ))}
      </View>
      <View style={s.axisRow}>
        <Text style={s.axisLabel}>now</Text>
        <Text style={s.axisLabel}>+24h</Text>
        <Text style={s.axisLabel}>+48h</Text>
        <Text style={s.axisLabel}>+72h</Text>
      </View>

      <Text style={s.sectionTitle}>Last 24 hours (heatmap)</Text>
      <View style={s.heatRow}>
        {heatmap.map((v, i) => (
          <View
            key={i}
            style={[s.heatCell, {
              backgroundColor: `rgba(46,125,50,${0.2 + (v / hmMax) * 0.8})`,
            }]}
          />
        ))}
      </View>
      <Text style={s.footer}>Pollen Scout · jayis1 · MIT</Text>
    </ScrollView>
  );
}

const IDX_COLORS = ['#4CAF50', '#FFC107', '#FF9800', '#F44336'];
const POLLEN_INDEX_LABELS = ['Low', 'Moderate', 'High', 'Very High'];

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  title: { fontSize: 20, fontWeight: '800', color: '#2E7D32', margin: 16 },
  pickerWrap: { backgroundColor: '#fff', margin: 12, borderRadius: 8, elevation: 1 },
  picker: { height: 50 },
  card: { backgroundColor: '#fff', margin: 12, padding: 16, borderRadius: 10, elevation: 1, alignItems: 'center' },
  cardTitle: { fontSize: 16, fontWeight: '700', color: '#333' },
  bigValue: { fontSize: 28, fontWeight: '800', color: '#2E7D32', marginVertical: 6 },
  conf: { fontSize: 12, color: '#888' },
  idxPill: { paddingHorizontal: 12, paddingVertical: 4, borderRadius: 12, marginTop: 8 },
  idxText: { color: '#fff', fontWeight: '700', fontSize: 12 },
  sectionTitle: { fontSize: 15, fontWeight: '700', color: '#333', margin: 12, marginBottom: 4 },
  chartRow: { flexDirection: 'row', alignItems: 'flex-end', height: 100, backgroundColor: '#fff', marginHorizontal: 12, padding: 8, borderRadius: 8, elevation: 1 },
  bar: { flex: 1, marginHorizontal: 0.5, backgroundColor: '#2E7D32', minWidth: 1 },
  axisRow: { flexDirection: 'row', justifyContent: 'space-between', marginHorizontal: 16, marginTop: 2 },
  axisLabel: { fontSize: 10, color: '#999' },
  heatRow: { flexDirection: 'row', flexWrap: 'wrap', backgroundColor: '#fff', marginHorizontal: 12, padding: 8, borderRadius: 8, elevation: 1 },
  heatCell: { width: 18, height: 18, margin: 1, borderRadius: 3 },
  footer: { textAlign: 'center', color: '#999', fontSize: 11, margin: 16 },
});