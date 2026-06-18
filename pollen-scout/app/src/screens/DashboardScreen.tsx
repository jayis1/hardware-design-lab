/**
 * DashboardScreen.tsx - Main station dashboard
 *
 * Shows current pollen index, top taxa with confidence bars,
 * 72-hour forecast chart, and a weather strip.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useMemo } from 'react';
import {
  View, Text, StyleSheet, ScrollView, RefreshControl, TouchableOpacity,
} from 'react-native';
import { useStation, makeDemoStation, pollenIndex } from '../context/StationContext';
import { POLLEN_INDEX_LABELS, TAXA_NAMES } from '../types';

const INDEX_COLORS = ['#4CAF50', '#FFC107', '#FF9800', '#F44336'];

export default function DashboardScreen() {
  const { station: live } = useStation();
  const station = live ?? useMemo(() => makeDemoStation(), []);
  const [refreshing, setRefreshing] = React.useState(false);

  const top = station.taxa.slice(0, 6);
  const dominant = top[0];
  const idx = pollenIndex(dominant?.concentration ?? 0);

  const onRefresh = async () => {
    setRefreshing(true);
    await new Promise((r) => setTimeout(r, 800));
    setRefreshing(false);
  };

  return (
    <ScrollView
      style={s.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
      }
    >
      {/* ---- Pollen index banner ---- */}
      <View style={[s.banner, { backgroundColor: INDEX_COLORS[idx] }]}>
        <Text style={s.bannerTitle}>Pollen Index</Text>
        <Text style={s.bannerValue}>{POLLEN_INDEX_LABELS[idx]}</Text>
        <Text style={s.bannerSub}>
          {dominant?.name ?? '—'} · {Math.round(dominant?.concentration ?? 0)} grains/m³
        </Text>
      </View>

      {/* ---- Weather strip ---- */}
      <View style={s.weatherRow}>
        <WeatherChip label="Temp" value={`${station.weather.temperatureC.toFixed(1)}°C`} />
        <WeatherChip label="RH"   value={`${station.weather.humidityPct.toFixed(0)}%`} />
        <WeatherChip label="UV"   value={station.weather.uvIndex.toFixed(1)} />
        <WeatherChip label="Wind" value={`${station.weather.windSpeedMps.toFixed(1)} m/s`} />
        <WeatherChip label="PM2.5" value={`${station.weather.pm2_5}`} />
      </View>

      {/* ---- Top taxa ---- */}
      <Text style={s.sectionTitle}>Top Taxa (live)</Text>
      {top.map((t) => (
        <View key={t.classId} style={s.taxaRow}>
          <View style={{ flex: 1 }}>
            <Text style={s.taxaName}>{t.name}</Text>
            <View style={s.barBg}>
              <View
                style={[s.barFg, {
                  width: `${Math.round(t.confidence * 100)}%`,
                  backgroundColor: INDEX_COLORS[pollenIndex(t.concentration)],
                }]}
              />
            </View>
          </View>
          <Text style={s.taxaConc}>{Math.round(t.concentration)}</Text>
          <Text style={s.taxaConf}>{Math.round(t.confidence * 100)}%</Text>
        </View>
      ))}

      {/* ---- 72-hour forecast chart (text sparkline) ---- */}
      <Text style={s.sectionTitle}>72-hour Forecast</Text>
      <ForecastSparkline forecast={station.forecast} />

      <TouchableOpacity style={s.pairBtn}>
        <Text style={s.pairBtnText}>Pair a new station →</Text>
      </TouchableOpacity>
      <Text style={s.footer}>Pollen Scout · jayis1 · MIT</Text>
    </ScrollView>
  );
}

function WeatherChip({ label, value }: { label: string; value: string }) {
  return (
    <View style={s.chip}>
      <Text style={s.chipLabel}>{label}</Text>
      <Text style={s.chipValue}>{value}</Text>
    </View>
  );
}

function ForecastSparkline({ forecast }: { forecast: number[][] }) {
  const max = Math.max(1, ...forecast.flat());
  return (
    <View style={s.sparkBox}>
      {forecast.map((row, i) => (
        <View key={i} style={s.sparkRow}>
          <Text style={s.sparkLabel}>{TAXA_NAMES[30 + i]?.slice(0, 10) ?? `T${i}`}</Text>
          <View style={s.sparkBars}>
            {row.map((v, h) => (
              <View
                key={h}
                style={[s.sparkBar, { height: 2 + (v / max) * 28 }]}
              />
            ))}
          </View>
        </View>
      ))}
    </View>
  );
}

const s = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  banner: { margin: 12, padding: 20, borderRadius: 14, alignItems: 'center' },
  bannerTitle: { color: '#fff', fontSize: 13, opacity: 0.9 },
  bannerValue: { color: '#fff', fontSize: 30, fontWeight: '800' },
  bannerSub:   { color: '#fff', fontSize: 13, marginTop: 4 },
  weatherRow: { flexDirection: 'row', flexWrap: 'wrap', paddingHorizontal: 8, justifyContent: 'space-between' },
  chip: { alignItems: 'center', padding: 8, margin: 4, backgroundColor: '#fff', borderRadius: 10, minWidth: 64, elevation: 1 },
  chipLabel: { fontSize: 10, color: '#666' },
  chipValue: { fontSize: 14, fontWeight: '700', color: '#333' },
  sectionTitle: { fontSize: 16, fontWeight: '700', color: '#333', margin: 12, marginBottom: 4 },
  taxaRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#fff', marginHorizontal: 12, marginVertical: 3, padding: 10, borderRadius: 8, elevation: 1 },
  taxaName: { fontSize: 13, fontWeight: '600', color: '#333' },
  barBg: { height: 6, backgroundColor: '#E0E0E0', borderRadius: 3, marginTop: 4 },
  barFg: { height: 6, borderRadius: 3 },
  taxaConc: { width: 50, textAlign: 'right', fontSize: 13, fontWeight: '700', color: '#333', marginLeft: 8 },
  taxaConf: { width: 40, textAlign: 'right', fontSize: 11, color: '#888', marginLeft: 4 },
  sparkBox: { backgroundColor: '#fff', margin: 12, padding: 10, borderRadius: 8, elevation: 1 },
  sparkRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 2 },
  sparkLabel: { width: 60, fontSize: 10, color: '#666' },
  sparkBars: { flex: 1, flexDirection: 'row', alignItems: 'flex-end', height: 30 },
  sparkBar: { flex: 1, marginHorizontal: 0.5, backgroundColor: '#2E7D32', minWidth: 1 },
  pairBtn: { backgroundColor: '#2E7D32', margin: 16, padding: 14, borderRadius: 10, alignItems: 'center' },
  pairBtnText: { color: '#fff', fontWeight: '700' },
  footer: { textAlign: 'center', color: '#999', fontSize: 11, margin: 12 },
});