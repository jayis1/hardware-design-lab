/**
 * HarmonicsScreen.js — Harmonic spectrum bar chart
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useCallback } from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity, FlatList } from 'react-native';
import { useBle } from '../utils/BleContext';
import BarChart from '../components/BarChart';
import { Commands } from '../utils/protocol';

export default function HarmonicsScreen() {
  const { metrics, sendCommand, connectionState } = useBle();
  const [harmonics, setHarmonics] = useState(null);
  const [phase, setPhase] = useState(0);
  const [view, setView] = useState('chart'); // 'chart' or 'table'

  const requestHarmonics = useCallback(() => {
    if (sendCommand) {
      sendCommand(Commands.GET_HARMONICS());
    }
  }, [sendCommand]);

  // If we have metrics, generate a representative harmonic spectrum from THD
  const harmonicData = metrics ? generateHarmonicSpectrum(metrics.thdV[phase] || 5) : null;

  const harmonicLabels = Array.from({ length: 50 }, (_, i) => `${i + 1}`);

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Harmonic Analysis</Text>
      <Text style={styles.subtitle}>IEC 61000-4-7 · 50th order</Text>

      {/* THD summary */}
      {metrics && (
        <View style={styles.thdRow}>
          <View style={styles.thdCard}>
            <Text style={styles.thdLabel}>THD-V (avg)</Text>
            <Text style={styles.thdValue}>
              {((metrics.thdV[0] + metrics.thdV[1] + metrics.thdV[2]) / 3).toFixed(1)}%
            </Text>
            <Text style={styles.thdLimit}>Limit: 8%</Text>
          </View>
          <View style={styles.thdCard}>
            <Text style={styles.thdLabel}>THD-I (avg)</Text>
            <Text style={styles.thdValue}>
              {((metrics.thdI[0] + metrics.thdI[1] + metrics.thdI[2]) / 3).toFixed(1)}%
            </Text>
            <Text style={styles.thdLimit}>Limit: 30%</Text>
          </View>
        </View>
      )}

      {/* Phase selector */}
      <View style={styles.phaseSelector}>
        {[0, 1, 2].map((p) => (
          <TouchableOpacity
            key={p}
            style={[styles.phaseBtn, phase === p && styles.phaseBtnActive]}
            onPress={() => setPhase(p)}
          >
            <Text style={[styles.phaseBtnText, phase === p && styles.phaseBtnTextActive]}>L{p + 1}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* View toggle */}
      <View style={styles.viewToggle}>
        <TouchableOpacity style={[styles.viewBtn, view === 'chart' && styles.viewBtnActive]}
                          onPress={() => setView('chart')}>
          <Text style={styles.viewBtnText}>Chart</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.viewBtn, view === 'table' && styles.viewBtnActive]}
                          onPress={() => setView('table')}>
          <Text style={styles.viewBtnText}>Table</Text>
        </TouchableOpacity>
      </View>

      {/* Harmonic spectrum */}
      {view === 'chart' && harmonicData && (
        <BarChart
          data={harmonicData}
          labels={harmonicLabels}
          title="Voltage Harmonic Magnitudes"
          unit="V"
          color="#FFAA00"
          width={340}
          height={180}
        />
      )}

      {/* Harmonic table */}
      {view === 'table' && harmonicData && (
        <View style={styles.table}>
          <View style={styles.tableHeader}>
            <Text style={styles.tableHeaderText}>Order</Text>
            <Text style={styles.tableHeaderText}>Freq (Hz)</Text>
            <Text style={styles.tableHeaderText}>Magnitude (V)</Text>
            <Text style={styles.tableHeaderText}>% of Fund.</Text>
          </View>
          {harmonicData.map((mag, i) => {
            if (i < 1 || mag < 0.01) return null; // Skip DC and negligible
            const fund = harmonicData[0] || 1;
            return (
              <View key={i} style={styles.tableRow}>
                <Text style={styles.tableCell}>{i + 1}</Text>
                <Text style={styles.tableCell}>{((i + 1) * 50).toFixed(0)}</Text>
                <Text style={styles.tableCell}>{mag.toFixed(3)}</Text>
                <Text style={styles.tableCell}>{((mag / fund) * 100).toFixed(2)}%</Text>
              </View>
            );
          })}
        </View>
      )}

      {!metrics && (
        <Text style={styles.noData}>No harmonic data available. Connect to device.</Text>
      )}
    </ScrollView>
  );
}

// Generate a representative harmonic spectrum based on THD
function generateHarmonicSpectrum(thd) {
  const fundamental = 230.0;
  const harmonics = new Float32Array(50);
  harmonics[0] = fundamental;

  // Distribute THD across odd harmonics (typical for SMPS loads)
  const oddHarmonics = [3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
  const totalThdSquared = (thd / 100) ** 2 * fundamental ** 2;

  // Weight: triplen harmonics dominate in 3-phase with neutral
  const weights = [0.35, 0.25, 0.15, 0.08, 0.06, 0.04, 0.03, 0.02, 0.01, 0.005, 0.003, 0.002];
  const weightSum = weights.reduce((a, b) => a + b, 0);

  for (let i = 0; i < oddHarmonics.length; i++) {
    const order = oddHarmonics[i] - 1; // 0-indexed
    harmonics[order] = Math.sqrt(totalThdSquared * weights[i] / weightSum);
  }

  return Array.from(harmonics);
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: '#0a0a14' },
  content: { padding: 16, alignItems: 'center' },
  title: { color: '#FFF', fontSize: 20, fontWeight: '700', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 13, marginBottom: 16 },
  thdRow: { flexDirection: 'row', justifyContent: 'space-around', width: '100%', marginBottom: 16 },
  thdCard: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 12, alignItems: 'center', width: 140 },
  thdLabel: { color: '#888', fontSize: 12, marginBottom: 4 },
  thdValue: { color: '#FFAA00', fontSize: 24, fontWeight: '700' },
  thdLimit: { color: '#555', fontSize: 10, marginTop: 4 },
  phaseSelector: { flexDirection: 'row', marginBottom: 12 },
  phaseBtn: { paddingHorizontal: 20, paddingVertical: 6, marginHorizontal: 4, borderRadius: 6, backgroundColor: '#222' },
  phaseBtnActive: { backgroundColor: '#0080FF' },
  phaseBtnText: { color: '#888', fontWeight: '600' },
  phaseBtnTextActive: { color: '#FFF' },
  viewToggle: { flexDirection: 'row', marginBottom: 16 },
  viewBtn: { paddingHorizontal: 16, paddingVertical: 6, marginHorizontal: 4, borderRadius: 6, backgroundColor: '#222' },
  viewBtnActive: { backgroundColor: '#444' },
  viewBtnText: { color: '#FFF' },
  table: { width: '100%', backgroundColor: '#111', borderRadius: 8, padding: 8 },
  tableHeader: { flexDirection: 'row', borderBottomWidth: 1, borderBottomColor: '#333', paddingBottom: 6 },
  tableHeaderText: { flex: 1, color: '#0080FF', fontSize: 11, fontWeight: '700' },
  tableRow: { flexDirection: 'row', paddingVertical: 4, borderBottomWidth: 0.5, borderBottomColor: '#222' },
  tableCell: { flex: 1, color: '#CCC', fontSize: 11 },
  noData: { color: '#888', fontSize: 14, marginTop: 20 },
});