// SurveyScreen.js — Survey Control + Status Display
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React, { useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, ScrollView } from 'react-native';
import { useBle } from '../utils/BleContext';
import StatusBar from '../components/StatusBar';
import { BAND_PRESETS } from '../utils/protocol';
import { formatDistance } from '../utils/gprMath';

export default function SurveyScreen() {
  const ble = useBle();
  const [selectedBand, setSelectedBand] = useState(2);
  const [epsR, setEpsR] = useState(6.0);
  const [traceSpacing, setTraceSpacing] = useState(20);

  if (!ble.connected) {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>StrataScan Survey</Text>
        <Text style={styles.subtitle}>Not connected to a device</Text>
        <TouchableOpacity style={styles.button} onPress={ble.connect} disabled={ble.connecting}>
          <Text style={styles.buttonText}>
            {ble.connecting ? 'Connecting...' : 'Connect to StrataScan'}
          </Text>
        </TouchableOpacity>
        {ble.error ? <Text style={styles.error}>{ble.error}</Text> : null}
      </View>
    );
  }

  const isSurveying = ble.status?.stateName === 'SURVEY';
  const isPaused = ble.status?.stateName === 'PAUSE';

  const handleStart = () => {
    ble.setBand(selectedBand);
    ble.setEpsR(epsR);
    ble.setSpacing(traceSpacing);
    ble.startSurvey();
  };

  return (
    <ScrollView style={styles.container}>
      <StatusBar status={ble.status} />

      <Text style={styles.sectionTitle}>Survey Parameters</Text>

      <Text style={styles.label}>Band Preset</Text>
      <View style={styles.bandGrid}>
        {BAND_PRESETS.map(band => (
          <TouchableOpacity
            key={band.id}
            style={[
              styles.bandButton,
              selectedBand === band.id && styles.bandSelected,
            ]}
            onPress={() => setSelectedBand(band.id)}
          >
            <Text style={[
              styles.bandText,
              selectedBand === band.id && styles.bandTextSelected,
            ]}>
              {band.name}
            </Text>
            <Text style={styles.bandRange}>{band.range}</Text>
            <Text style={styles.bandDepth}>{band.depth}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.label}>Relative Permittivity (ε_r): {epsR.toFixed(1)}</Text>
      <View style={styles.sliderRow}>
        {[1, 3, 6, 9, 12, 20, 30, 50, 81].map(val => (
          <TouchableOpacity
            key={val}
            style={[styles.epsButton, epsR === val && styles.epsSelected]}
            onPress={() => setEpsR(val)}
          >
            <Text style={styles.epsText}>{val}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={styles.label}>Trace Spacing: {traceSpacing} mm</Text>
      <View style={styles.sliderRow}>
        {[10, 20, 50, 100, 200].map(val => (
          <TouchableOpacity
            key={val}
            style={[styles.epsButton, traceSpacing === val && styles.epsSelected]}
            onPress={() => setTraceSpacing(val)}
          >
            <Text style={styles.epsText}>{val}mm</Text>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.controls}>
        {!isSurveying && !isPaused && (
          <TouchableOpacity style={[styles.button, styles.startButton]} onPress={handleStart}>
            <Text style={styles.buttonText}>▶ Start Survey</Text>
          </TouchableOpacity>
        )}
        {isSurveying && (
          <TouchableOpacity style={[styles.button, styles.pauseButton]} onPress={ble.pauseSurvey}>
            <Text style={styles.buttonText}>⏸ Pause</Text>
          </TouchableOpacity>
        )}
        {isPaused && (
          <TouchableOpacity style={[styles.button, styles.startButton]} onPress={ble.startSurvey}>
            <Text style={styles.buttonText}>▶ Resume</Text>
          </TouchableOpacity>
        )}
        {(isSurveying || isPaused) && (
          <TouchableOpacity style={[styles.button, styles.stopButton]} onPress={ble.stopSurvey}>
            <Text style={styles.buttonText}>⏹ Stop</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={[styles.button, styles.recalButton]} onPress={ble.recalibrate}>
          <Text style={styles.buttonText}>⟳ Recalibrate</Text>
        </TouchableOpacity>
      </View>

      {ble.status && (
        <View style={styles.statsCard}>
          <Text style={styles.statsTitle}>Survey Statistics</Text>
          <Text style={styles.statRow}>Traces acquired: {ble.status.tracesAcquired}</Text>
          <Text style={styles.statRow}>
            Distance: {formatDistance(ble.status.tracesAcquired * traceSpacing / 1000)}
          </Text>
          <Text style={styles.statRow}>Battery: {ble.status.batteryPct}%</Text>
          <Text style={styles.statRow}>
            GNSS: {ble.status.lat.toFixed(5)}, {ble.status.lon.toFixed(5)}
          </Text>
        </View>
      )}

      <TouchableOpacity
        style={[styles.button, styles.disconnectButton]}
        onPress={() => {
          ble.stopSurvey();
          ble.disconnect();
        }}
      >
        <Text style={styles.buttonText}>Disconnect</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a', padding: 15 },
  title: { color: '#0ff', fontSize: 24, fontWeight: 'bold', textAlign: 'center', marginTop: 60 },
  subtitle: { color: '#888', textAlign: 'center', marginVertical: 20, fontSize: 14 },
  sectionTitle: { color: '#0ff', fontSize: 18, fontWeight: 'bold', marginTop: 15, marginBottom: 8 },
  label: { color: '#ccc', fontSize: 14, marginTop: 12, marginBottom: 5 },
  bandGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  bandButton: {
    width: '31%', backgroundColor: '#16213e', padding: 8, borderRadius: 8,
    borderWidth: 1, borderColor: '#333', marginBottom: 6, alignItems: 'center',
  },
  bandSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  bandText: { color: '#fff', fontSize: 14, fontWeight: 'bold' },
  bandTextSelected: { color: '#0ff' },
  bandRange: { color: '#888', fontSize: 9, marginTop: 2, textAlign: 'center' },
  bandDepth: { color: '#5a5', fontSize: 9, marginTop: 1 },
  sliderRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  epsButton: {
    backgroundColor: '#16213e', paddingHorizontal: 12, paddingVertical: 6,
    borderRadius: 6, borderWidth: 1, borderColor: '#333',
  },
  epsSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  epsText: { color: '#ccc', fontSize: 12 },
  controls: { flexDirection: 'row', flexWrap: 'wrap', gap: 10, marginTop: 20 },
  button: {
    backgroundColor: '#16213e', paddingHorizontal: 20, paddingVertical: 12,
    borderRadius: 8, borderWidth: 1, borderColor: '#333', minWidth: 140,
    alignItems: 'center', marginVertical: 5,
  },
  startButton: { backgroundColor: '#1a3050', borderColor: '#0066CC' },
  pauseButton: { backgroundColor: '#2a2a10', borderColor: '#aa8800' },
  stopButton: { backgroundColor: '#2a1010', borderColor: '#aa0000' },
  recalButton: { backgroundColor: '#1a1a2e', borderColor: '#555' },
  disconnectButton: { backgroundColor: '#1a1a2e', borderColor: '#555', marginTop: 20, alignSelf: 'center' },
  buttonText: { color: '#fff', fontSize: 15, fontWeight: 'bold' },
  statsCard: {
    backgroundColor: '#16213e', borderRadius: 8, padding: 15, marginTop: 20,
    borderWidth: 1, borderColor: '#333',
  },
  statsTitle: { color: '#0ff', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  statRow: { color: '#ccc', fontSize: 13, marginVertical: 3, fontFamily: 'monospace' },
  error: { color: '#ff4444', textAlign: 'center', marginTop: 10 },
});