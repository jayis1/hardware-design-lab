/**
 * WaveformScreen.js — Live voltage/current waveform display
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, ScrollView, StyleSheet, TouchableOpacity, Switch } from 'react-native';
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';
import { useBle } from '../utils/BleContext';

const WAVEFORM_WIDTH = 340;
const WAVEFORM_HEIGHT = 120;
const MAX_POINTS = 200; // Display 200 points (simplified — would request waveform capture)

export default function WaveformScreen() {
  const { metrics, nilm, sendCommand } = useBle();
  const [waveformData, setWaveformData] = useState({ v: [], i: [] });
  const [showVoltage, setShowVoltage] = useState(true);
  const [showCurrent, setShowCurrent] = useState(true);
  const [phase, setPhase] = useState(0);

  // Generate a simulated waveform from the current metrics (as a visual placeholder)
  useEffect(() => {
    if (!metrics) return;

    const vRms = metrics.vRms[phase] || 230;
    const iRms = metrics.iRms[phase] || 5;
    const freq = metrics.freq || 50;
    const pf = metrics.pf || 0.9;

    const vPoints = [];
    const iPoints = [];
    for (let n = 0; n < MAX_POINTS; n++) {
      const t = (n / MAX_POINTS) * 0.04; // 40 ms window (2 cycles at 50 Hz)
      const wt = 2 * Math.PI * freq * t;
      const vPeak = vRms * Math.SQRT2;
      const iPeak = iRms * Math.SQRT2;
      const phaseAngle = Math.acos(Math.max(-1, Math.min(1, pf)));

      const v = vPeak * Math.sin(wt);
      const i = iPeak * Math.sin(wt - phaseAngle);

      const x = (n / MAX_POINTS) * WAVEFORM_WIDTH;
      const vY = WAVEFORM_HEIGHT / 2 - (v / 400) * (WAVEFORM_HEIGHT / 2 - 5);
      const iY = WAVEFORM_HEIGHT / 2 - (i / 100) * (WAVEFORM_HEIGHT / 2 - 5);

      vPoints.push(`${x},${vY}`);
      iPoints.push(`${x},${iY}`);
    }
    setWaveformData({ v: vPoints, i: iPoints });
  }, [metrics, phase]);

  return (
    <ScrollView style={styles.scroll} contentContainerStyle={styles.content}>
      <Text style={styles.title}>Waveform Display</Text>
      <Text style={styles.subtitle}>Phase L{phase + 1} · {metrics ? metrics.freq.toFixed(1) + ' Hz' : '--'}</Text>

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

      {/* Voltage waveform */}
      {showVoltage && (
        <View style={styles.waveformContainer}>
          <Text style={styles.waveLabel}>Voltage (V)</Text>
          <Svg width={WAVEFORM_WIDTH} height={WAVEFORM_HEIGHT}>
            <Line x1={0} y1={WAVEFORM_HEIGHT / 2} x2={WAVEFORM_WIDTH} y2={WAVEFORM_HEIGHT / 2}
                  stroke="#333" strokeWidth={0.5} />
            <Polyline points={waveformData.v.join(' ')} fill="none" stroke="#FFAA00" strokeWidth={1.5} />
            <SvgText x={4} y={12} fontSize={9} fill="#FFAA00">
              {metrics ? `${metrics.vRms[phase].toFixed(1)} Vrms` : ''}
            </SvgText>
          </Svg>
        </View>
      )}

      {/* Current waveform */}
      {showCurrent && (
        <View style={styles.waveformContainer}>
          <Text style={styles.waveLabel}>Current (A)</Text>
          <Svg width={WAVEFORM_WIDTH} height={WAVEFORM_HEIGHT}>
            <Line x1={0} y1={WAVEFORM_HEIGHT / 2} x2={WAVEFORM_WIDTH} y2={WAVEFORM_HEIGHT / 2}
                  stroke="#333" strokeWidth={0.5} />
            <Polyline points={waveformData.i.join(' ')} fill="none" stroke="#00CCFF" strokeWidth={1.5} />
            <SvgText x={4} y={12} fontSize={9} fill="#00CCFF">
              {metrics ? `${metrics.iRms[phase].toFixed(2)} Arms` : ''}
            </SvgText>
          </Svg>
        </View>
      )}

      {/* Display toggles */}
      <View style={styles.toggleRow}>
        <Text style={styles.toggleLabel}>Show Voltage</Text>
        <Switch value={showVoltage} onValueChange={setShowVoltage} />
      </View>
      <View style={styles.toggleRow}>
        <Text style={styles.toggleLabel}>Show Current</Text>
        <Switch value={showCurrent} onValueChange={setShowCurrent} />
      </View>

      {/* Capture button */}
      <TouchableOpacity style={styles.captureBtn} onPress={() => sendCommand && sendCommand([])}>
        <Text style={styles.captureBtnText}>⏺ Capture 60s Waveform</Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  scroll: { flex: 1, backgroundColor: '#0a0a14' },
  content: { padding: 16, alignItems: 'center' },
  title: { color: '#FFF', fontSize: 20, fontWeight: '700', marginBottom: 4 },
  subtitle: { color: '#888', fontSize: 14, marginBottom: 12 },
  phaseSelector: { flexDirection: 'row', marginBottom: 16 },
  phaseBtn: { paddingHorizontal: 20, paddingVertical: 8, marginHorizontal: 4, borderRadius: 6, backgroundColor: '#222' },
  phaseBtnActive: { backgroundColor: '#0080FF' },
  phaseBtnText: { color: '#888', fontWeight: '600' },
  phaseBtnTextActive: { color: '#FFF' },
  waveformContainer: { marginBottom: 16 },
  waveLabel: { color: '#FFF', fontSize: 12, fontWeight: '600', marginBottom: 4 },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', width: '100%', paddingHorizontal: 20, paddingVertical: 8 },
  toggleLabel: { color: '#FFF', fontSize: 14 },
  captureBtn: { backgroundColor: '#AA0000', paddingHorizontal: 24, paddingVertical: 12, borderRadius: 8, marginTop: 16 },
  captureBtnText: { color: '#FFF', fontWeight: '700', fontSize: 16 },
});