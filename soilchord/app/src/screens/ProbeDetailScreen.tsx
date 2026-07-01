/**
 * ProbeDetailScreen — per-chord time-series charts and depth profile
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useEffect, useState } from 'react';
import { View, Text, ScrollView, StyleSheet, ActivityIndicator } from 'react-native';
import { RouteProp } from '@react-navigation/native';
import { RootStackParamList } from '../../App';
import { useStore, getProbe, getHistory, riskScoreForProbe, moistureEstimate } from '../store';
import { UL_FLAG_ALERT } from '../proto';

type Route = RouteProp<RootStackParamList, 'ProbeDetail'>;

const DEPTHS = [0.0, 0.3, 0.6, 0.9, 1.2, 1.5];
const SOIL_TYPES = ['sand', 'loam', 'silt', 'clay'] as const;

export default function ProbeDetailScreen({ route }: { route: Route }) {
  const { probeId } = route.params;
  const probe = getProbe(probeId);
  const hist = getHistory(probeId);
  const [soil, setSoil] = useState<'sand' | 'loam' | 'silt' | 'clay'>('loam');

  if (!probe) {
    return <View style={s.wrap}><ActivityIndicator color="#8bc34a" /></View>;
  }

  const last = hist[hist.length - 1];
  const score = riskScoreForProbe(probeId);

  // Sparkline: last 30 f0 values for the selected chord
  const [selChord, setSelChord] = useState(0);
  const f0Series = hist.slice(-30).map((f) => f.chords[selChord]?.f0Hz ?? 0);
  const qSeries = hist.slice(-30).map((f) => f.chords[selChord]?.qFreq ?? 0);
  const tempSeries = hist.slice(-30).map((f) => f.chords[selChord]?.tempC ?? 0);
  const moistSeries = hist.slice(-30).map((f) => moistureEstimate(f.chords[selChord]?.qFreq ?? 80, soil));

  return (
    <ScrollView style={s.wrap}>
      <View style={s.header}>
        <Text style={s.title}>{probe.name}</Text>
        <Text style={s.sub}>{probe.site} · risk {score}/100</Text>
      </View>

      <View style={s.depthProfile}>
        <Text style={s.sectionTitle}>Depth Profile (latest)</Text>
        {last && last.chords.map((c, i) => {
          const moist = moistureEstimate(c.qFreq, soil);
          return (
            <View key={i} style={s.depthRow}>
              <Text style={s.depthLabel}>{DEPTHS[i].toFixed(1)} m</Text>
              <View style={s.barWrap}>
                <View style={[s.bar, { width: `${Math.min(100, c.qFreq)}%`, backgroundColor: c.flags & UL_FLAG_ALERT ? '#d32f2f' : '#8bc34a' }]} />
              </View>
              <Text style={s.depthVal}>Q{c.qFreq.toFixed(0)} f{c.f0Hz.toFixed(0)} {moist.toFixed(0)}%</Text>
            </View>
          );
        })}
      </View>

      <View style={s.chordPicker}>
        {DEPTHS.map((d, i) => (
          <Text key={i} style={[s.chordBtn, i === selChord && s.chordBtnSel]} onPress={() => setSelChord(i)}>
            {d.toFixed(1)}m
          </Text>
        ))}
      </View>

      <View style={s.chart}>
        <Text style={s.sectionTitle}>f₀ (Hz) — chord @ {DEPTHS[selChord].toFixed(1)} m</Text>
        <Sparkline data={f0Series} color="#8bc34a" />
      </View>
      <View style={s.chart}>
        <Text style={s.sectionTitle}>Q (damping)</Text>
        <Sparkline data={qSeries} color="#ffb300" />
      </View>
      <View style={s.chart}>
        <Text style={s.sectionTitle}>Temperature (°C)</Text>
        <Sparkline data={tempSeries} color="#42a5f5" />
      </View>
      <View style={s.chart}>
        <Text style={s.sectionTitle}>Moisture estimate (% VWC) — {soil}</Text>
        <Sparkline data={moistSeries} color="#26c6da" />
      </View>

      <View style={s.soilPicker}>
        {SOIL_TYPES.map((t) => (
          <Text key={t} style={[s.chordBtn, t === soil && s.chordBtnSel]} onPress={() => setSoil(t)}>{t}</Text>
        ))}
      </View>

      <View style={s.power}>
        <Text style={s.sectionTitle}>Power</Text>
        {last && <Text style={s.powerLine}>Battery {(last.batteryMv / 1000).toFixed(2)} V · Solar {(last.solarMv / 1000).toFixed(2)} V</Text>}
        <Text style={s.powerLine}>Interval {probe.urgent ? '120s (urgent)' : '600s'} · {hist.length} records</Text>
      </View>
    </ScrollView>
  );
}

function Sparkline({ data, color }: { data: number[]; color: string }) {
  if (data.length < 2) return <Text style={s.noData}>No data yet</Text>;
  const min = Math.min(...data), max = Math.max(...data);
  const range = max - min || 1;
  const W = 300, H = 80;
  const pts = data.map((v, i) => {
    const x = (i / (data.length - 1)) * W;
    const y = H - ((v - min) / range) * H;
    return `${x.toFixed(1)},${y.toFixed(1)}`;
  }).join(' ');
  // We render a simple SVG-free bar strip since react-native-svg isn't a dep.
  const bars = data.map((v, i) => {
    const h = ((v - min) / range) * H;
    return <View key={i} style={{ width: 300 / data.length - 2, height: Math.max(2, h), backgroundColor: color, marginHorizontal: 1 }} />;
  });
  return (
    <View style={{ flexDirection: 'row', alignItems: 'flex-end', height: H, width: W, marginTop: 8 }}>
      {bars}
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0d1f17', padding: 12 },
  header: { marginBottom: 12 },
  title: { color: '#e8f5e9', fontSize: 22, fontWeight: 'bold' },
  sub: { color: '#9ccc9c', fontSize: 13 },
  sectionTitle: { color: '#c8e6c9', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  depthProfile: { backgroundColor: '#1a3a2a', borderRadius: 12, padding: 12, marginBottom: 12 },
  depthRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  depthLabel: { color: '#9ccc9c', fontSize: 12, width: 50 },
  barWrap: { flex: 1, height: 16, backgroundColor: '#0d1f17', borderRadius: 8, marginHorizontal: 8, overflow: 'hidden' },
  bar: { height: 16, borderRadius: 8 },
  depthVal: { color: '#c8e6c9', fontSize: 11, width: 140, textAlign: 'right' },
  chordPicker: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 12 },
  soilPicker: { flexDirection: 'row', justifyContent: 'space-around', marginBottom: 12 },
  chordBtn: { color: '#9ccc9c', fontSize: 12, padding: 6, borderRadius: 8, backgroundColor: '#1a3a2a' },
  chordBtnSel: { color: '#0d1f17', backgroundColor: '#8bc34a', fontWeight: 'bold' },
  chart: { backgroundColor: '#1a3a2a', borderRadius: 12, padding: 12, marginBottom: 12 },
  noData: { color: '#9ccc9c', fontSize: 12 },
  power: { backgroundColor: '#1a3a2a', borderRadius: 12, padding: 12, marginBottom: 24 },
  powerLine: { color: '#c8e6c9', fontSize: 12, marginVertical: 2 },
});