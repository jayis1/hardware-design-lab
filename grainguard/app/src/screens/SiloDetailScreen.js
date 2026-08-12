/**
 * @file    SiloDetailScreen.js
 * @brief   Detailed per-silo sensor view: temp profile, CO2, moisture, insects.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useMemo, useState } from 'react';
import { View, Text, ScrollView, StyleSheet, Dimensions } from 'react-native';
import { useGrainGuard } from '../services/GrainGuardContext';
import SRIgauge from '../components/SRIgauge';
import TempProfileChart from '../components/TempProfileChart';
import { parseMeshPacket } from '../utils/protocol';

const { width } = Dimensions.get('window');

export default function SiloDetailScreen() {
  const { probes, selectedSiloId } = useGrainGuard();
  const [timeRange, setTimeRange] = useState('24h');  // '1h' | '24h' | '7d'

  const probe = probes[selectedSiloId];
  if (!probe) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>No silo selected.</Text>
        <Text style={styles.emptyHint}>Tap a silo on the Dashboard to view details.</Text>
      </View>
    );
  }

  const data = useMemo(() => parseMeshPacket(probe), [probe]);

  const sriBreakdown = useMemo(() => {
    return [
      { label: 'CO₂',           value: probe.co2Contribution || 0,  max: 35, color: '#42A5F5' },
      { label: 'Temp Gradient',  value: probe.tempGradContribution || 0, max: 25, color: '#FF7043' },
      { label: 'Temp Absolute',  value: probe.tempAbsContribution || 0,  max: 15, color: '#FFCA28' },
      { label: 'Moisture',       value: probe.emcContribution || 0, max: 15, color: '#66BB6A' },
      { label: 'Insects',        value: probe.acousticContribution || 0, max: 10, color: '#EF5350' },
    ];
  }, [probe]);

  return (
    <ScrollView style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <View>
          <Text style={styles.siloName}>{probe.siloName || `Silo ${probe.serial}`}</Text>
          <Text style={styles.grainType}>{data.grainName} · Probe #{probe.serial}</Text>
        </View>
        <SRIgauge value={data.sri} size={64} />
      </View>

      {/* Alert banner */}
      {data.alertLevel === 2 && (
        <View style={[styles.banner, { backgroundColor: '#FFCDD2' }]}>
          <Text style={styles.bannerText}>🔴 CRITICAL — SRI {data.sri}/100</Text>
          <Text style={styles.bannerSub}>
            {recommendAction(data.sri, data)} — act within hours.
          </Text>
        </View>
      )}
      {data.alertLevel === 1 && (
        <View style={[styles.banner, { backgroundColor: '#FFF9C4' }]}>
          <Text style={styles.bannerText}>🟡 CAUTION — SRI {data.sri}/100</Text>
          <Text style={styles.bannerSub}>Monitor closely and plan intervention.</Text>
        </View>
      )}
      {data.alertLevel === 0 && (
        <View style={[styles.banner, { backgroundColor: '#C8E6C9' }]}>
          <Text style={styles.bannerText}>🟢 STABLE — SRI {data.sri}/100</Text>
        </View>
      )}

      {/* Key metrics grid */}
      <View style={styles.metricsGrid}>
        <MetricCard label="CO₂" value={`${data.co2Ppm} ppm`} sub="NDIR sensor" />
        <MetricCard label="Moisture" value={`${data.emcPct}%`} sub={`Safe ≤ ${data.safeMcPct}%`} />
        <MetricCard label="Max Temp" value={`${data.tmaxC}°C`} sub={`Zone ${data.tmaxZone}`} />
        <MetricCard label="Min Temp" value={`${data.tminC}°C`} sub={`Zone ${data.tminZone}`} />
        <MetricCard label="ΔT" value={`${data.deltaTC}°C`} sub="Gradient" />
        <MetricCard label="RH" value={`${data.rhPct}%`} sub="Relative humidity" />
        <MetricCard label="Insects" value={`${data.aeEventsPerMin}/min`} sub={insectName(data.insectId)} />
        <MetricCard label="Battery" value={`${data.batteryMv} mV`} sub={data.batteryPct + '%'} />
      </View>

      {/* Temperature profile chart (9 zones) */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Temperature Profile (9 Zones)</Text>
        <TempProfileChart data={data.tempZones} width={width - 32} height={180} />
      </View>

      {/* SRI breakdown bars */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Spoilage Risk Breakdown</Text>
        {sriBreakdown.map((item, i) => (
          <View key={i} style={styles.barRow}>
            <Text style={styles.barLabel}>{item.label}</Text>
            <View style={styles.barTrack}>
              <View style={[styles.barFill, {
                width: `${(item.value / item.max) * 100}%`,
                backgroundColor: item.color,
              }]} />
            </View>
            <Text style={styles.barVal}>{item.value}/{item.max}</Text>
          </View>
        ))}
      </View>

      {/* Insect detection details */}
      {data.insectId > 0 && (
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Insect Detection</Text>
          <View style={styles.insectBox}>
            <Text style={styles.insectTitle}>{insectName(data.insectId)}</Text>
            <Text style={styles.insectDetail}>Events: {data.aeEventsPerMin}/min</Text>
            <Text style={styles.insectDetail}>Confidence: {data.aeConfidence || '--'}%</Text>
            <Text style={styles.insectDetail}>Peak amplitude: {data.aePeakMv || '--'} mV</Text>
            <Text style={styles.insectDetail}>
              Avg event duration: {data.aeAvgDurMs || '--'} ms
            </Text>
          </View>
        </View>
      )}

      {/* Time range selector */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Historical Trend</Text>
        <View style={styles.rangeBar}>
          {['1h', '24h', '7d'].map(r => (
            <View key={r} style={[
              styles.rangeBtn,
              timeRange === r && styles.rangeBtnActive,
            ]}>
              <Text
                style={timeRange === r ? styles.rangeTextActive : styles.rangeText}
                onPress={() => setTimeRange(r)}
              >
                {r}
              </Text>
            </View>
          ))}
        </View>
        <Text style={styles.hintText}>
          Showing {timeRange} trend data from probe flash log.
        </Text>
      </View>
    </ScrollView>
  );
}

function MetricCard({ label, value, sub }) {
  return (
    <View style={styles.metricCard}>
      <Text style={styles.metricCardLabel}>{label}</Text>
      <Text style={styles.metricCardValue}>{value}</Text>
      <Text style={styles.metricCardSub}>{sub}</Text>
    </View>
  );
}

function insectName(id) {
  const names = {
    0: 'None detected',
    1: 'S. granarius',
    2: 'T. castaneum',
    3: 'R. dominica',
    254: 'Unknown',
  };
  return names[id] || 'Unknown';
}

function recommendAction(sri, data) {
  if (data.co2Ppm > 2000) return 'Aerate immediately and inspect for mold';
  if (data.deltaTC > 5) return 'Run aeration fan to equalize temperature';
  if (data.emcPct > data.safeMcPct) return 'Dry grain or run aeration to reduce moisture';
  if (data.aeEventsPerMin > 10) return 'Fumigate or apply controlled atmosphere';
  if (data.tmaxC > 30) return 'Cool grain with aeration (target < 15°C)';
  return 'Inspect silo and monitor';
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    padding: 16, backgroundColor: '#1B5E20',
  },
  siloName: { fontSize: 22, fontWeight: 'bold', color: '#fff' },
  grainType: { fontSize: 13, color: '#A5D6A7', marginTop: 2 },
  banner: { padding: 12, alignItems: 'center' },
  bannerText: { fontSize: 16, fontWeight: 'bold', color: '#333' },
  bannerSub: { fontSize: 12, color: '#555', marginTop: 2 },
  metricsGrid: {
    flexDirection: 'row', flexWrap: 'wrap', padding: 8,
    justifyContent: 'space-between',
  },
  metricCard: {
    width: (width - 24) / 2 - 8, backgroundColor: '#fff', borderRadius: 6,
    padding: 12, marginBottom: 8, elevation: 1,
  },
  metricCardLabel: { fontSize: 11, color: '#9E9E9E' },
  metricCardValue: { fontSize: 22, fontWeight: 'bold', color: '#333', marginVertical: 4 },
  metricCardSub: { fontSize: 10, color: '#BDBDBD' },
  section: { padding: 16, backgroundColor: '#fff', marginTop: 8 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#333', marginBottom: 12 },
  barRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  barLabel: { width: 100, fontSize: 12, color: '#616161' },
  barTrack: { flex: 1, height: 18, backgroundColor: '#E0E0E0', borderRadius: 4, marginRight: 8 },
  barFill: { height: 18, borderRadius: 4 },
  barVal: { fontSize: 11, color: '#757575', width: 45 },
  insectBox: {
    backgroundColor: '#FFF3E0', borderRadius: 8, padding: 14,
    borderLeftWidth: 4, borderLeftColor: '#FF9800',
  },
  insectTitle: { fontSize: 15, fontWeight: 'bold', color: '#E65100', marginBottom: 6 },
  insectDetail: { fontSize: 12, color: '#795548', marginTop: 2 },
  rangeBar: { flexDirection: 'row', marginBottom: 12 },
  rangeBtn: {
    flex: 1, paddingVertical: 8, alignItems: 'center',
    borderWidth: 1, borderColor: '#E0E0E0',
  },
  rangeBtnActive: { backgroundColor: '#1B5E20' },
  rangeText: { color: '#616161', fontSize: 14 },
  rangeTextActive: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  hintText: { fontSize: 12, color: '#9E9E9E' },
  empty: { flex: 1, alignItems: 'center', justifyContent: 'center', padding: 40 },
  emptyText: { fontSize: 18, color: '#9E9E9E' },
  emptyHint: { fontSize: 13, color: '#BDBDBD', marginTop: 8 },
});