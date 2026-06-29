/*
 * screens/HiveDetail.tsx — Detailed view of a single hive with charts
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import { Ionicons } from '@expo/vector-icons';
import type { HiveData } from '../App';
import { stateName, stateColor, formatWeight, formatTemp,
         formatRelativeTime, formatBattery } from '../components/StateUtils';

type Props = {
  hive: HiveData | null;
  history: any[];
};

export default function HiveDetail({ hive, history }: Props) {
  if (!hive) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>Select a hive from the Apiary tab</Text>
      </View>
    );
  }

  const color = stateColor(hive.state);

  // Prepare chart data (last 48 points = ~8h at 10min intervals)
  const recent = history.slice(0, 48).reverse();
  const tempData = recent.map((h) => h.brood_temp_c / 100);
  const weightData = recent.map((h) => h.weight_g / 1000);
  const labels = recent.map((_, i) => (i % 6 === 0 ? `${i}` : ''));

  const chartConfig = {
    backgroundGradientFrom: '#fff',
    backgroundGradientTo: '#fff',
    color: (opacity = 1) => `rgba(232, 168, 0, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(100, 100, 100, ${opacity})`,
    strokeWidth: 2,
    decimalCount: 1,
  };

  return (
    <ScrollView style={styles.container}>
      {/* Header card */}
      <View style={[styles.headerCard, { borderLeftColor: color }]}>
        <View style={styles.headerTop}>
          <Text style={styles.hiveName}>{hive.name}</Text>
          <View style={[styles.statePill, { backgroundColor: color }]}>
            <Text style={styles.statePillText}>{stateName(hive.state)}</Text>
          </View>
        </View>
        <Text style={styles.deveui}>DevEUI: {hive.deveui}</Text>
        <Text style={styles.lastSeen}>Last reading: {formatRelativeTime(hive.lastSeen)}</Text>
        {hive.rssi !== 0 && (
          <Text style={styles.rssi}>Signal: {hive.rssi} dBm</Text>
        )}
      </View>

      {/* Current metrics grid */}
      <View style={styles.metricsGrid}>
        <MetricBox icon="thermometer" label="Brood Temp"
          value={formatTemp(hive.broodTempC)} color="#f44336" />
        <MetricBox icon="scale" label="Weight"
          value={formatWeight(hive.weightG)} color="#e8a800" />
        <MetricBox icon="musical-notes" label="Dominant Freq"
          value={hive.dominantHz ? hive.dominantHz + ' Hz' : '—'} color="#2196f3" />
        <MetricBox icon="water" label="Humidity"
          value={hive.humidity + ' %'} color="#00bcd4" />
        <MetricBox icon="battery-half" label="Battery"
          value={formatBattery(hive.batteryMv)} color="#4caf50" />
        <MetricBox icon="pulse" label="Loudness CV"
          value={(hive.cvLoud / 255).toFixed(2)} color="#9c27b0" />
      </View>

      {/* Temperature chart */}
      <View style={styles.chartCard}>
        <Text style={styles.chartTitle}>Brood Temperature (°C) — last 8h</Text>
        {tempData.length > 1 ? (
          <LineChart
            data={{ labels, datasets: [{ data: tempData }] }}
            width={320} height={160}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
          />
        ) : (
          <Text style={styles.noData}>Not enough data yet</Text>
        )}
      </View>

      {/* Weight chart */}
      <View style={styles.chartCard}>
        <Text style={styles.chartTitle}>Weight (kg) — last 8h</Text>
        {weightData.length > 1 ? (
          <LineChart
            data={{ labels, datasets: [{ data: weightData }] }}
            width={320} height={160}
            chartConfig={{ ...chartConfig,
              color: (opacity = 1) => `rgba(76, 175, 80, ${opacity})` }}
            bezier
            style={styles.chart}
          />
        ) : (
          <Text style={styles.noData}>Not enough data yet</Text>
        )}
      </View>

      {/* Acoustic features detail */}
      <View style={styles.featureCard}>
        <Text style={styles.cardTitle}>Acoustic Analysis</Text>
        <FeatureRow label="Dominant frequency" value={hive.dominantHz + ' Hz'} />
        <FeatureRow label="High-band ratio (r_hi)"
          value={(hive.rHi / 255).toFixed(3)} />
        <FeatureRow label="Loudness CV"
          value={(hive.cvLoud / 255).toFixed(3)} />
        <FeatureRow label="State code" value={hive.state.toString()} />
        <View style={styles.legendRow}>
          <Text style={styles.legendText}>Classification thresholds:</Text>
          <Text style={styles.legendItem}>Queenright: 240–320 Hz, r_hi&lt;0.20</Text>
          <Text style={styles.legendItem}>Queenless: &gt;380 Hz, r_hi&gt;0.35</Text>
          <Text style={styles.legendItem}>Swarm prep: 95–125 Hz, pulsing</Text>
          <Text style={styles.legendItem}>Winter: 50–95 Hz, &lt;10°C</Text>
          <Text style={styles.legendItem}>Dead: energy &lt;threshold</Text>
        </View>
      </View>

      {/* Alert flags */}
      {hive.flags !== 0 && (
        <View style={styles.flagsCard}>
          <Text style={styles.cardTitle}>Status Flags</Text>
          {hive.flags & 0x01 && <Text style={styles.flagAlert}>⚠ Anomaly alert active</Text>}
          {hive.flags & 0x02 && <Text style={styles.flagOk}>☀ Solar panel charging</Text>}
          {hive.flags & 0x04 && <Text style={styles.flagWarn}>⚠ Probe fault detected</Text>}
        </View>
      )}
    </ScrollView>
  );
}

function MetricBox({ icon, label, value, color }: {
  icon: string; label: string; value: string; color: string;
}) {
  return (
    <View style={styles.metricBox}>
      <Ionicons name={icon as any} size={20} color={color} />
      <Text style={styles.metricBoxLabel}>{label}</Text>
      <Text style={styles.metricBoxValue}>{value}</Text>
    </View>
  );
}

function FeatureRow({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.featureRow}>
      <Text style={styles.featureLabel}>{label}</Text>
      <Text style={styles.featureValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center' },
  emptyText: { fontSize: 16, color: '#888' },
  headerCard: {
    backgroundColor: '#fff', padding: 16, margin: 12,
    borderRadius: 10, borderLeftWidth: 4, elevation: 2,
    shadowColor: '#000', shadowOpacity: 0.08, shadowRadius: 3,
    shadowOffset: { width: 0, height: 1 },
  },
  headerTop: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  hiveName: { fontSize: 20, fontWeight: '800', color: '#222' },
  statePill: { paddingHorizontal: 10, paddingVertical: 4, borderRadius: 12 },
  statePillText: { color: 'white', fontSize: 12, fontWeight: '600' },
  deveui: { fontSize: 11, color: '#999', marginTop: 6 },
  lastSeen: { fontSize: 12, color: '#777', marginTop: 2 },
  rssi: { fontSize: 12, color: '#777' },
  metricsGrid: {
    flexDirection: 'row', flexWrap: 'wrap',
    paddingHorizontal: 8, justifyContent: 'space-between',
  },
  metricBox: {
    backgroundColor: '#fff', borderRadius: 8, padding: 10,
    width: '31%', margin: 4, alignItems: 'center', elevation: 1,
    shadowColor: '#000', shadowOpacity: 0.05, shadowRadius: 2,
    shadowOffset: { width: 0, height: 1 },
  },
  metricBoxLabel: { fontSize: 10, color: '#888', marginTop: 4 },
  metricBoxValue: { fontSize: 14, fontWeight: '700', color: '#333', marginTop: 2 },
  chartCard: {
    backgroundColor: '#fff', margin: 12, borderRadius: 10,
    padding: 12, elevation: 1,
  },
  chartTitle: { fontSize: 14, fontWeight: '600', color: '#555', marginBottom: 8 },
  chart: { borderRadius: 8 },
  noData: { color: '#aaa', textAlign: 'center', padding: 20 },
  featureCard: {
    backgroundColor: '#fff', margin: 12, borderRadius: 10, padding: 14, elevation: 1,
  },
  cardTitle: { fontSize: 15, fontWeight: '700', color: '#444', marginBottom: 10 },
  featureRow: {
    flexDirection: 'row', justifyContent: 'space-between',
    paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#eee',
  },
  featureLabel: { fontSize: 13, color: '#666' },
  featureValue: { fontSize: 13, fontWeight: '600', color: '#333' },
  legendRow: { marginTop: 12 },
  legendText: { fontSize: 12, fontWeight: '600', color: '#888', marginBottom: 4 },
  legendItem: { fontSize: 11, color: '#999', marginTop: 2 },
  flagsCard: {
    backgroundColor: '#fff', margin: 12, borderRadius: 10, padding: 14, elevation: 1,
  },
  flagAlert: { fontSize: 13, color: '#f44336', marginTop: 4 },
  flagOk: { fontSize: 13, color: '#4caf50', marginTop: 4 },
  flagWarn: { fontSize: 13, color: '#ff9800', marginTop: 4 },
});