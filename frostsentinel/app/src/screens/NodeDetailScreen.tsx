// src/screens/NodeDetailScreen.tsx — Per-node detail with time-series plots
//
// Shows live time-series of sky T, air T, wet-bulb, leaf wetness, AE
// energy, and the RFRI gauge for the selected node.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';
import bleManager from '../ble/BleManager';
import db from '../db/database';
import RFRIGauge from '../components/RFRIGauge';
import { aeStatusLabel, rfriColor } from '../ble/protocol';
import type { LiveData } from '../ble/BleManager';
import type { SampleRecord } from '../db/database';

export default function NodeDetailScreen({ route }: any) {
  const nodeId: number = route?.params?.nodeId ?? 1;
  const [live, setLive] = useState<LiveData | null>(null);
  const [samples, setSamples] = useState<SampleRecord[]>([]);

  useEffect(() => {
    loadSamples();

    const unsub = bleManager.on('liveData', (data: LiveData) => {
      if (data.nodeId === nodeId) {
        setLive(data);
      }
    });
    return unsub;
  }, [nodeId]);

  const loadSamples = async () => {
    try {
      await db.open();
      const s = await db.getSamples(nodeId, 288); // last 24h at 5-min
      setSamples(s);
    } catch (e) {
      console.warn('[NodeDetail] loadSamples:', e);
    }
  };

  const rfri = live?.rfri ?? 0;
  const twet = live?.twetC ?? 0;
  const tsky = live?.tskyC ?? 0;
  const tair = live?.tairC ?? 0;
  const deltaRad = live?.deltaRadK ?? 0;
  const leafWet = live?.leafWet ?? 0;
  const aeStatus = live?.aeStatus ?? 0;
  const batteryPct = live?.batteryPct ?? 0;

  // Build plot data from samples (last 24 h)
  const plotData = samples.slice(-288);
  const tempPlot = buildPlot(plotData, [
    { key: 'tair', color: '#FFC107' },
    { key: 'tsky', color: '#42A5F5' },
    { key: 'twet', color: '#66BB6A' },
  ], -40, 40);

  const wetnessPlot = buildPlot(plotData, [
    { key: 'leafWet', color: '#26C6DA' },
  ], 0, 1000);

  const rfriPlot = buildPlot(plotData, [
    { key: 'rfri', color: '#EF5350' },
  ], 0, 1);

  return (
    <ScrollView style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.nodeTitle}>Node {nodeId}</Text>
        <Text style={styles.nodeStatus}>
          {aeStatusLabel(aeStatus)} · {batteryPct}% battery
        </Text>
      </View>

      {/* RFRI Gauge */}
      <View style={styles.card}>
        <RFRIGauge rfri={rfri} size={220} label="Radiative Frost Risk Index" />
      </View>

      {/* Current readings */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Current Readings</Text>
        <View style={styles.readingGrid}>
          <Reading label="Air Temp" value={`${tair.toFixed(1)} °C`} color="#FFC107" />
          <Reading label="Sky Temp" value={`${tsky.toFixed(1)} °C`} color="#42A5F5" />
          <Reading label="Wet Bulb" value={`${twet.toFixed(1)} °C`}
                   color={twet <= 0 ? '#F44336' : '#66BB6A'} />
          <Reading label="ΔT Rad" value={`${deltaRad.toFixed(1)} K`} color="#fff" />
          <Reading label="Leaf Wet" value={`${(leafWet / 10).toFixed(0)}%`} color="#26C6DA" />
          <Reading label="AE Status" value={aeStatusLabel(aeStatus)}
                   color={aeStatus === 2 ? '#F44336' : '#ccc'} />
        </View>
      </View>

      {/* Temperature plot (24 h) */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Temperature (24 h)</Text>
        <PlotView data={tempPlot} height={120} />
        <View style={styles.legend}>
          <LegendItem color="#FFC107" label="Air" />
          <LegendItem color="#42A5F5" label="Sky" />
          <LegendItem color="#66BB6A" label="Wet Bulb" />
        </View>
      </View>

      {/* Leaf wetness plot */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Leaf Wetness (24 h)</Text>
        <PlotView data={wetnessPlot} height={80} />
      </View>

      {/* RFRI plot */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>RFRI Trend (24 h)</Text>
        <PlotView data={rfriPlot} height={80} />
      </View>

      {/* Time-to-critical estimate */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Time to Critical Freeze</Text>
        <Text style={styles.ttcValue}>
          {twet <= 0 ? 'CRITICAL — wet-bulb at/below 0°C' :
           twet <= 5 ? `~${Math.round(twet / Math.max(deltaRad / 20, 0.5))} h` :
           '> 5 h — no imminent risk'}
        </Text>
      </View>
    </ScrollView>
  );
}

// ---- Helpers ----

function Reading({ label, value, color }: { label: string; value: string; color: string }) {
  return (
    <View style={styles.reading}>
      <Text style={styles.readingLabel}>{label}</Text>
      <Text style={[styles.readingValue, { color }]}>{value}</Text>
    </View>
  );
}

function LegendItem({ color, label }: { color: string; label: string }) {
  return (
    <View style={styles.legendItem}>
      <View style={[styles.legendDot, { backgroundColor: color }]} />
      <Text style={styles.legendText}>{label}</Text>
    </View>
  );
}

interface PlotSeries {
  points: string;
  color: string;
}

function buildPlot(samples: SampleRecord[], series: { key: string; color: string }[],
                  minVal: number, maxVal: number): PlotSeries[] {
  if (samples.length === 0) {
    return series.map(s => ({ points: '', color: s.color }));
  }
  const width = 300;
  const height = 100;
  const n = samples.length;

  return series.map(s => {
    const points = samples.map((sample, i) => {
      const x = (i / (n - 1)) * width;
      const val = (sample as any)[s.key] ?? 0;
      const y = height - ((val - minVal) / (maxVal - minVal)) * height;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    }).join(' ');
    return { points, color: s.color };
  });
}

function PlotView({ data, height = 100 }: { data: PlotSeries[]; height?: number }) {
  const width = 300;
  return (
    <Svg width={width} height={height}>
      {/* Grid lines */}
      <Line x1="0" y1={height / 2} x2={width} y2={height / 2}
            stroke="#333" strokeWidth="0.5" strokeDasharray="2,2" />
      {/* Series */}
      {data.map((s, i) =>
        s.points ? (
          <Polyline
            key={i}
            points={s.points}
            fill="none"
            stroke={s.color}
            strokeWidth="1.5"
          />
        ) : null
      )}
    </Svg>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a' },
  header: { padding: 15, paddingTop: 30, borderBottomWidth: 1, borderBottomColor: '#1b263b' },
  nodeTitle: { fontSize: 20, fontWeight: 'bold', color: '#fff' },
  nodeStatus: { fontSize: 12, color: '#778da9', marginTop: 4 },
  card: {
    backgroundColor: '#1b263b',
    borderRadius: 8,
    padding: 12,
    margin: 10,
  },
  cardTitle: { fontSize: 14, fontWeight: '600', color: '#e0e1dd', marginBottom: 8 },
  readingGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  reading: { width: '48%', marginBottom: 8 },
  readingLabel: { fontSize: 10, color: '#778da9' },
  readingValue: { fontSize: 16, fontWeight: '600', marginTop: 2 },
  legend: { flexDirection: 'row', justifyContent: 'center', marginTop: 6 },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginHorizontal: 8 },
  legendDot: { width: 8, height: 8, borderRadius: 4, marginRight: 4 },
  legendText: { fontSize: 10, color: '#778da9' },
  ttcValue: { fontSize: 15, color: '#FF9800', fontWeight: '600' },
});