/**
 * LiveRunScreen.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React from 'react';
import { ScrollView, View, Text, StyleSheet } from 'react-native';
import MetricCard from '../components/MetricCard';
import TrendPanel from '../components/TrendPanel';
import { sampleTelemetry, sampleHistory, sampleHumidity } from '../utils/protocol';

export default function LiveRunScreen() {
  return (
    <ScrollView style={styles.screen} contentContainerStyle={styles.content}>
      <Text style={styles.header}>Active Cycle Telemetry</Text>
      <Text style={styles.caption}>Run-state indexed metrics from DryerFlow Guardian • Author: jayis1</Text>
      <View style={styles.grid}>
        <MetricCard label="Pressure Drop" value={sampleTelemetry.pressure_pa.toFixed(1)} unit="Pa" accent="#38bdf8" subtitle="Across vent reference collar" />
        <MetricCard label="Exhaust Temp" value={sampleTelemetry.temp_c.toFixed(1)} unit="°C" accent="#fb7185" subtitle="Filtered exhaust air temperature" />
        <MetricCard label="Humidity" value={sampleTelemetry.humidity_rh.toFixed(1)} unit="%RH" accent="#60a5fa" subtitle="Used for dry-down detection" />
        <MetricCard label="Backdraft Score" value={sampleTelemetry.bss.toFixed(1)} unit="BSS" accent="#f97316" subtitle="CO + VOC + pressure fusion" />
      </View>
      <TrendPanel title="Load progression" points={sampleHistory.map((value, index) => value - index)} color="#22c55e" note="In a healthy cycle, flow should stay relatively stable while humidity falls after the wet peak." />
      <TrendPanel title="Moisture release envelope" points={sampleHumidity} color="#38bdf8" note="A prolonged plateau indicates restricted air exchange or overloaded fabric mass." />
      <View style={styles.notes}>
        <Text style={styles.notesTitle}>Operator note</Text>
        <Text style={styles.notesBody}>When pressure rises but flow falls, the firmware attributes more of the risk score to obstruction rather than normal load variation.</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#030712' },
  content: { padding: 16, paddingBottom: 24 },
  header: { color: '#f8fafc', fontSize: 24, fontWeight: '700' },
  caption: { color: '#94a3b8', marginTop: 6, marginBottom: 16 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  notes: { backgroundColor: '#111827', padding: 16, borderRadius: 16 },
  notesTitle: { color: '#e5e7eb', fontSize: 16, fontWeight: '700', marginBottom: 8 },
  notesBody: { color: '#cbd5e1', lineHeight: 20 }
});
