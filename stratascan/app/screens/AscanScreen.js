// AscanScreen.js — A-Scan Waveform Inspector
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React, { useState, useMemo } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Slider, TextInput } from 'react-native';
import { useBle } from '../utils/BleContext';
import AscanPlot from '../components/AscanPlot';
import { detectPeaks, formatDepth, MATERIAL_EPS_R } from '../utils/gprMath';

export default function AscanScreen() {
  const ble = useBle();
  const [selectedIdx, setSelectedIdx] = useState(0);
  const [threshold, setThreshold] = useState(0.3);
  const [epsR, setEpsR] = useState(6.0);

  const traces = ble.bscanBuffer?.traces || [];
  const depths = ble.bscanBuffer?.depths || [];

  const currentTrace = useMemo(() => {
    const t = traces[selectedIdx];
    if (t) return t;
    // Find nearest non-null trace
    for (let i = selectedIdx; i < traces.length; i++) {
      if (traces[i]) return traces[i];
    }
    for (let i = selectedIdx; i >= 0; i--) {
      if (traces[i]) return traces[i];
    }
    return null;
  }, [traces, selectedIdx]);

  const peaks = useMemo(() => {
    if (!currentTrace) return [];
    return detectPeaks(currentTrace, threshold);
  }, [currentTrace, threshold]);

  if (!ble.connected) {
    return (
      <View style={styles.container}>
        <Text style={styles.empty}>Connect to a StrataScan device to inspect A-scans.</Text>
      </View>
    );
  }

  if (!currentTrace) {
    return (
      <View style={styles.container}>
        <Text style={styles.empty}>No trace data available. Start a survey first.</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>A-Scan Inspector</Text>
        <Text style={styles.subtitle}>Reflectivity vs. Depth Profile</Text>
      </View>

      <View style={styles.traceSelector}>
        <Text style={styles.label}>Trace #{selectedIdx} of {traces.length}</Text>
        <View style={styles.selectorRow}>
          <TouchableOpacity
            style={styles.navButton}
            onPress={() => setSelectedIdx(Math.max(0, selectedIdx - 1))}
          >
            <Text style={styles.navText}>◀ Prev</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.navButton}
            onPress={() => setSelectedIdx(Math.min(traces.length - 1, selectedIdx + 1))}
          >
            <Text style={styles.navText}>Next ▶</Text>
          </TouchableOpacity>
        </View>
      </View>

      <AscanPlot trace={currentTrace} depths={depths} threshold={threshold} epsR={epsR} />

      <View style={styles.controlsCard}>
        <Text style={styles.cardTitle}>Detection Settings</Text>

        <Text style={styles.label}>Peak Detection Threshold: {threshold.toFixed(2)}</Text>
        <View style={styles.sliderContainer}>
          <TouchableOpacity style={styles.smallBtn} onPress={() => setThreshold(Math.max(0.1, threshold - 0.05))}>
            <Text style={styles.smallBtnText}>−</Text>
          </TouchableOpacity>
          <View style={styles.sliderTrack}>
            <View style={[styles.sliderFill, { width: `${threshold * 100}%` }]} />
          </View>
          <TouchableOpacity style={styles.smallBtn} onPress={() => setThreshold(Math.min(0.9, threshold + 0.05))}>
            <Text style={styles.smallBtnText}>+</Text>
          </TouchableOpacity>
        </View>

        <Text style={styles.label}>Relative Permittivity (ε_r): {epsR.toFixed(1)}</Text>
        <View style={styles.materialRow}>
          {Object.entries(MATERIAL_EPS_R).slice(0, 8).map(([name, val]) => (
            <TouchableOpacity
              key={name}
              style={[styles.materialBtn, Math.abs(epsR - val) < 0.1 && styles.materialSelected]}
              onPress={() => setEpsR(val)}
            >
              <Text style={styles.materialText}>{name}</Text>
              <Text style={styles.materialVal}>{val}</Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {peaks.length > 0 && (
        <View style={styles.peaksCard}>
          <Text style={styles.cardTitle}>Detected Interfaces</Text>
          {peaks.slice(0, 8).map((pk, i) => {
            const depth = depths[pk.index] || pk.index * 0.1;
            return (
              <View key={`pk${i}`} style={styles.peakRow}>
                <Text style={styles.peakIdx}>#{i + 1}</Text>
                <Text style={styles.peakDepth}>{formatDepth(depth)}</Text>
                <Text style={styles.peakAmp}>{(pk.amplitude * 100).toFixed(0)}%</Text>
              </View>
            );
          })}
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  header: { padding: 15, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  title: { color: '#0ff', fontSize: 20, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 12, marginTop: 2 },
  empty: { color: '#888', textAlign: 'center', marginTop: 100, fontSize: 16 },
  traceSelector: { padding: 15, borderBottomWidth: 1, borderBottomColor: '#1a1a2e' },
  label: { color: '#ccc', fontSize: 13, marginBottom: 8 },
  selectorRow: { flexDirection: 'row', gap: 10 },
  navButton: {
    backgroundColor: '#16213e', paddingHorizontal: 16, paddingVertical: 8,
    borderRadius: 6, borderWidth: 1, borderColor: '#333', flex: 1, alignItems: 'center',
  },
  navText: { color: '#fff', fontSize: 14 },
  controlsCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
  },
  cardTitle: { color: '#0ff', fontSize: 15, fontWeight: 'bold', marginBottom: 10 },
  sliderContainer: { flexDirection: 'row', alignItems: 'center', gap: 10, marginBottom: 15 },
  smallBtn: {
    backgroundColor: '#1a3050', width: 30, height: 30, borderRadius: 6,
    alignItems: 'center', justifyContent: 'center', borderWidth: 1, borderColor: '#333',
  },
  smallBtnText: { color: '#fff', fontSize: 20 },
  sliderTrack: { flex: 1, height: 6, backgroundColor: '#0a0a1a', borderRadius: 3, overflow: 'hidden' },
  sliderFill: { height: '100%', backgroundColor: '#0066CC' },
  materialRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  materialBtn: {
    backgroundColor: '#0a0a1a', paddingHorizontal: 10, paddingVertical: 6,
    borderRadius: 6, borderWidth: 1, borderColor: '#333', alignItems: 'center',
  },
  materialSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  materialText: { color: '#ccc', fontSize: 11 },
  materialVal: { color: '#5a5', fontSize: 10 },
  peaksCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333', marginBottom: 30,
  },
  peakRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4, borderBottomWidth: 1, borderBottomColor: '#0a0a1a' },
  peakIdx: { color: '#888', fontSize: 12, fontFamily: 'monospace' },
  peakDepth: { color: '#0ff', fontSize: 12, fontFamily: 'monospace' },
  peakAmp: { color: '#5a5', fontSize: 12, fontFamily: 'monospace' },
});