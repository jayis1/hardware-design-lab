/**
 * DashboardScreen.js — Real-time power quality overview
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity } from 'react-native';
import { useBle } from '../utils/BleContext';
import Gauge from '../components/Gauge';

export default function DashboardScreen() {
  const { metrics, status, connectionState, reconnect, errors } = useBle();

  if (connectionState !== 'connected') {
    return (
      <View style={styles.center}>
        <Text style={styles.statusText}>
          {connectionState === 'scanning' ? 'Scanning for WattLens...' :
           connectionState === 'error' ? 'Connection error' :
           'Not connected'}
        </Text>
        <TouchableOpacity style={styles.button} onPress={reconnect}>
          <Text style={styles.buttonText}>Connect</Text>
        </TouchableOpacity>
      </View>
    );
  }

  if (!metrics) {
    return (
      <View style={styles.center}>
        <Text style={styles.statusText}>Waiting for data...</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      {/* Error banner */}
      {errors.length > 0 && (
        <View style={styles.errorBanner}>
          <Text style={styles.errorText}>⚠ {errors.join(', ')}</Text>
        </View>
      )}

      {/* Battery and status header */}
      <View style={styles.headerRow}>
        <Text style={styles.batteryText}>
          🔋 {status ? status.battery : '--'}%
        </Text>
        <Text style={styles.freqText}>
          {metrics.freq.toFixed(2)} Hz
        </Text>
      </View>

      {/* Total power gauges */}
      <Text style={styles.sectionTitle}>Total Power</Text>
      <Gauge label="Active Power" value={metrics.pTotalReal} unit="W"
             min={0} max={15000} format={(v) => v.toFixed(0)} />
      <Gauge label="Reactive Power" value={metrics.pTotalReactive} unit="var"
             min={-10000} max={10000} format={(v) => v.toFixed(0)} />
      <Gauge label="Apparent Power" value={metrics.pTotalApparent} unit="VA"
             min={0} max={15000} format={(v) => v.toFixed(0)} />

      {/* Power factor */}
      <Text style={styles.sectionTitle}>Power Quality</Text>
      <Gauge label="Power Factor" value={metrics.pf} unit=""
             min={0} max={1} format={(v) => v.toFixed(3)} />
      <Gauge label="Voltage THD" value={metrics.thdV} unit="%"
             min={0} max={20} format={(v) => v.toFixed(1)} />
      <Gauge label="Current THD" value={metrics.thdI} unit="%"
             min={0} max={50} format={(v) => v.toFixed(1)} />

      {/* Per-phase voltages */}
      <Text style={styles.sectionTitle}>Per-Phase Voltages (RMS)</Text>
      {metrics.vRms.map((v, i) => (
        <Gauge key={`v${i}`} label={`L${i + 1} Voltage`} value={v} unit="V"
               min={0} max={300} format={(val) => val.toFixed(1)} />
      ))}

      {/* Per-phase currents */}
      <Text style={styles.sectionTitle}>Per-Phase Currents (RMS)</Text>
      {metrics.iRms.map((c, i) => (
        <Gauge key={`i${i}`} label={`L${i + 1} Current`} value={c} unit="A"
               min={0} max={50} format={(val) => val.toFixed(2)} />
      ))}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: '#0a0a14' },
  content: { paddingVertical: 16 },
  center: { flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#0a0a14' },
  statusText: { color: '#888', fontSize: 16, marginBottom: 16 },
  button: { backgroundColor: '#0080FF', paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8 },
  buttonText: { color: '#FFF', fontWeight: '700', fontSize: 16 },
  errorBanner: { backgroundColor: '#440000', padding: 8, marginHorizontal: 12, borderRadius: 6, marginBottom: 8 },
  errorText: { color: '#FF6666', fontSize: 13 },
  headerRow: { flexDirection: 'row', justifyContent: 'space-between', paddingHorizontal: 16, marginBottom: 8 },
  batteryText: { color: '#00CC44', fontSize: 16, fontWeight: '600' },
  freqText: { color: '#00CCFF', fontSize: 16, fontWeight: '600' },
  sectionTitle: { color: '#0080FF', fontSize: 14, fontWeight: '700', marginTop: 12, marginBottom: 4, paddingHorizontal: 12 },
});