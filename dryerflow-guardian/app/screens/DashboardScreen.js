/**
 * DashboardScreen.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React from 'react';
import { ScrollView, View, Text, StyleSheet } from 'react-native';
import MetricCard from '../components/MetricCard';
import TrendPanel from '../components/TrendPanel';
import { sampleTelemetry, sampleHistory, sampleHumidity, alertFlagsToList, healthTone } from '../utils/protocol';

export default function DashboardScreen() {
  const tone = healthTone(sampleTelemetry.vri, sampleTelemetry.bss);
  const topAlerts = alertFlagsToList(sampleTelemetry.alerts).slice(0, 2);

  return (
    <ScrollView style={styles.screen} contentContainerStyle={styles.content}>
      <View style={styles.hero}>
        <Text style={styles.title}>DryerFlow Guardian</Text>
        <Text style={[styles.status, { color: tone.color }]}>{tone.label}</Text>
        <Text style={styles.subtitle}>Author: jayis1 • Predictive vent health for dryers</Text>
      </View>

      <View style={styles.grid}>
        <MetricCard label="Vent Resistance" value={sampleTelemetry.vri.toFixed(1)} unit="VRI" accent="#f59e0b" subtitle="Compared with learned baseline" />
        <MetricCard label="Service Horizon" value={sampleTelemetry.service_horizon.toFixed(1)} unit="loads" accent="#60a5fa" subtitle="Estimated loads before cleanout" />
        <MetricCard label="Airflow" value={sampleTelemetry.flow_cfm.toFixed(1)} unit="CFM" accent="#34d399" subtitle="Flow estimate after filtering" />
        <MetricCard label="CO Trend" value={sampleTelemetry.co_ppm.toFixed(1)} unit="ppm" accent="#f87171" subtitle="Gas dryer backdraft indicator" />
      </View>

      <TrendPanel title="Vent resistance across recent loads" points={sampleHistory} color="#f59e0b" note="Upward drift suggests lint accumulation near an elbow or wall section." />
      <TrendPanel title="Humidity dry-down profile" points={sampleHumidity} color="#38bdf8" note="A healthy cycle should show a clear post-peak decline as free moisture is exhausted." />

      <View style={styles.alertCard}>
        <Text style={styles.alertTitle}>Immediate guidance</Text>
        {topAlerts.map((item) => <Text key={item} style={styles.alertItem}>• {item}</Text>)}
        <Text style={styles.alertBody}>If this trend persists, clean the duct path, inspect the flex hose for crushing, and compare before/after VRI in the Service Coach.</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#020617' },
  content: { padding: 16, paddingBottom: 28 },
  hero: {
    backgroundColor: '#111827',
    borderRadius: 20,
    padding: 18,
    marginBottom: 16
  },
  title: { color: '#f8fafc', fontSize: 26, fontWeight: '700' },
  status: { marginTop: 6, fontSize: 20, fontWeight: '700' },
  subtitle: { marginTop: 8, color: '#9ca3af' },
  grid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between'
  },
  alertCard: {
    backgroundColor: '#111827',
    borderRadius: 16,
    padding: 16
  },
  alertTitle: { color: '#f8fafc', fontWeight: '700', fontSize: 16, marginBottom: 8 },
  alertItem: { color: '#fef3c7', marginBottom: 4 },
  alertBody: { color: '#cbd5e1', marginTop: 8, lineHeight: 20 }
});
