/**
 * LoadsScreen.js — NILM appliance disaggregation display
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, ScrollView, StyleSheet } from 'react-native';
import { useBle } from '../utils/BleContext';
import ApplianceCard from '../components/ApplianceCard';
import { APPLIANCE_NAMES } from '../utils/protocol';

export default function LoadsScreen() {
  const { nilm, metrics } = useBle();

  // Get active and inactive appliances
  const appliances = nilm?.appliances || [];
  const active = appliances.filter((a) => a.state === 1);
  const inactive = appliances.filter((a) => a.state === 0);

  // Total disaggregated power
  const totalDisagPower = active.reduce((sum, a) => {
    const power = metrics ? a.prob * metrics.pTotalReal : a.prob * 1000;
    return sum + power;
  }, 0);

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Load Disaggregation</Text>
      <Text style={styles.subtitle}>Non-Intrusive Load Monitoring (NILM)</Text>

      {/* Summary card */}
      <View style={styles.summaryCard}>
        <View style={styles.summaryItem}>
          <Text style={styles.summaryLabel}>Active Loads</Text>
          <Text style={styles.summaryValue}>{active.length}</Text>
        </View>
        <View style={styles.summaryItem}>
          <Text style={styles.summaryLabel}>Disag. Power</Text>
          <Text style={styles.summaryValue}>{Math.round(totalDisagPower)} W</Text>
        </View>
        <View style={styles.summaryItem}>
          <Text style={styles.summaryLabel}>Total Power</Text>
          <Text style={styles.summaryValue}>{metrics ? Math.round(metrics.pTotalReal) : '--'} W</Text>
        </View>
      </View>

      {/* Accuracy indicator */}
      {metrics && (
        <View style={styles.accuracyBar}>
          <Text style={styles.accuracyText}>
            Coverage: {((totalDisagPower / Math.max(metrics.pTotalReal, 1)) * 100).toFixed(0)}%
            of measured power disaggregated
          </Text>
        </View>
      )}

      {/* Active appliances */}
      <Text style={styles.sectionTitle}>🟢 Running Now</Text>
      {active.length > 0 ? (
        active.map((a) => (
          <ApplianceCard
            key={a.classId}
            name={APPLIANCE_NAMES[a.classId] || `Appliance ${a.classId}`}
            isOn={true}
            probability={a.prob}
            power={metrics ? a.prob * metrics.pTotalReal : a.prob * 1000}
          />
        ))
      ) : (
        <Text style={styles.emptyText}>No active loads detected</Text>
      )}

      {/* Inactive appliances */}
      <Text style={styles.sectionTitle}>⚫ Standby / Off</Text>
      {inactive.length > 0 ? (
        inactive.slice(0, 8).map((a) => (
          <ApplianceCard
            key={a.classId}
            name={APPLIANCE_NAMES[a.classId] || `Appliance ${a.classId}`}
            isOn={false}
            probability={a.prob}
            power={0}
          />
        ))
      ) : (
        <Text style={styles.emptyText}>All known loads are active</Text>
      )}

      {/* Energy estimate note */}
      <View style={styles.noteBox}>
        <Text style={styles.noteText}>
          💡 WattLens disaggregates loads from the aggregate mains signature using
          an on-device neural network. No per-outlet sensors needed. Accuracy
          depends on model training — upload improved models via Settings.
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: '#0a0a14' },
  content: { paddingVertical: 16 },
  title: { color: '#FFF', fontSize: 20, fontWeight: '700', textAlign: 'center', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 13, textAlign: 'center', marginBottom: 16 },
  summaryCard: {
    flexDirection: 'row', justifyContent: 'space-around',
    backgroundColor: '#1a1a2e', borderRadius: 10, padding: 12, marginHorizontal: 12, marginBottom: 8,
  },
  summaryItem: { alignItems: 'center' },
  summaryLabel: { color: '#888', fontSize: 11, marginBottom: 4 },
  summaryValue: { color: '#00CC44', fontSize: 18, fontWeight: '700' },
  accuracyBar: { backgroundColor: '#0a2a0a', borderRadius: 6, padding: 8, marginHorizontal: 12, marginBottom: 16 },
  accuracyText: { color: '#00CC44', fontSize: 12, textAlign: 'center' },
  sectionTitle: { color: '#0080FF', fontSize: 14, fontWeight: '700', marginTop: 12, marginBottom: 6, paddingHorizontal: 12 },
  emptyText: { color: '#555', fontSize: 13, paddingHorizontal: 12, paddingVertical: 8, fontStyle: 'italic' },
  noteBox: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginHorizontal: 12, marginTop: 20, marginBottom: 12 },
  noteText: { color: '#999', fontSize: 12, lineHeight: 18 },
});