/**
 * LprDetailScreen — LPR sweep plot + corrosion rate
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, Button, StyleSheet, ActivityIndicator } from 'react-native';
import Svg, { Polyline, Rect, Text as SvgText, Circle, Line } from 'react-native-svg';
import type { StackScreenProps } from '@react-navigation/stack';
import type { RootStackParamList } from '../../App';
import BleManager from '../ble/BleManager';
import { parseLprResult } from '../ble/protocol';

type Props = StackScreenProps<RootStackParamList, 'Lpr'>;

export default function LprDetailScreen({ route }: Props) {
  const { surveyId } = route.params;
  const [ecorr, setEcorr] = useState(-250);
  const [running, setRunning] = useState(false);
  const [result, setResult] = useState<{ vCorr_mm_yr: number; rp_kohm: number; iCorr_ua_cm2: number } | null>(null);
  const [sweep, setSweep] = useState<{ e: number; i: number }[]>([]);

  const runSweep = async () => {
    setRunning(true);
    setResult(null);
    setSweep([]);
    /* Simulated sweep preview while the device runs the actual measurement.
     * In production, the device streams periodic samples; here we plot a
     * synthetic curve for UI demonstration and await the final result. */
    for (let n = 0; n <= 50; n++) {
      const e = ecorr - 25 + n;
      const cur = 0.5 + 0.04 * (n - 25) + 0.001 * (n - 25) * (n - 25);
      setSweep((prev) => [...prev, { e, i: cur }]);
      await new Promise((r) => setTimeout(r, 50));
    }
    const raw = await BleManager.startLpr(ecorr);
    if (raw) setResult(parseLprResult(raw));
    setRunning(false);
  };

  const W = 300;
  const H = 200;
  const eMin = ecorr - 25;
  const eMax = ecorr + 25;
  const iMax = sweep.length ? Math.max(...sweep.map((s) => s.i)) * 1.1 : 5;
  const xScale = (e: number) => ((e - eMin) / (eMax - eMin)) * W;
  const yScale = (i: number) => H - (i / iMax) * H;
  const pathStr = sweep.map((s, idx) => `${idx === 0 ? 'M' : 'L'} ${xScale(s.e)} ${yScale(s.i)}`).join(' ');

  return (
    <View style={styles.container}>
      <Text style={styles.header}>LPR Corrosion Rate</Text>
      <Text style={styles.subheader}>Survey #{surveyId}</Text>

      <Text style={styles.label}>Free corrosion potential E_corr (mV vs Cu/CuSO₄)</Text>
      <TextInput
        style={styles.input}
        value={String(ecorr)}
        onChangeText={(v) => setEcorr(Number(v))}
        keyboardType="numeric"
      />

      <Button title={running ? 'Running sweep…' : 'Run LPR Sweep'} onPress={runSweep} disabled={running} color="#9b59b6" />
      {running && <ActivityIndicator size="large" color="#9b59b6" style={{ marginTop: 12 }} />}

      {/* Sweep plot */}
      {sweep.length > 0 && (
        <Svg width={W} height={H} style={styles.plot}>
          <Rect x={0} y={0} width={W} height={H} fill="#0d0d1a" rx={6} />
          <Line x1={0} y1={H / 2} x2={W} y2={H / 2} stroke="#34495e" strokeWidth={0.5} />
          <Polyline points={pathStr} fill="none" stroke="#9b59b6" strokeWidth={2} />
          <SvgText x={4} y={14} fill="#95a5a6" fontSize={9}>I (µA)</SvgText>
          <SvgText x={W - 40} y={H - 4} fill="#95a5a6" fontSize={9}>E (mV)</SvgText>
        </Svg>
      )}

      {/* Results */}
      {result && (
        <View style={styles.resultBox}>
          <Text style={styles.resultHeader}>Stern-Geary Results</Text>
          <Text style={styles.resultLine}>
            R_p = <Text style={styles.val}>{result.rp_kohm.toFixed(2)} kΩ</Text>
          </Text>
          <Text style={styles.resultLine}>
            i_corr = <Text style={styles.val}>{result.iCorr_ua_cm2.toFixed(3)} µA/cm²</Text>
          </Text>
          <Text style={[styles.resultLine, { fontSize: 18 }]}>
            Corrosion rate: <Text style={[styles.val, { color: result.vCorr_mm_yr > 0.1 ? '#e74c3c' : '#2ecc71' }]}>
              {result.vCorr_mm_yr.toFixed(4)} mm/year
            </Text>
          </Text>
          <Text style={styles.note}>
            {result.vCorr_mm_yr > 0.5
              ? '⚠ Severe — high corrosion rate; recommend immediate intervention.'
              : result.vCorr_mm_yr > 0.1
              ? '⚠ Moderate — monitor and plan remediation.'
              : '✓ Low — passive or low-rate corrosion; routine monitoring.'}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#1a1a2e' },
  header: { fontSize: 22, fontWeight: 'bold', color: '#ecf0f1' },
  subheader: { fontSize: 12, color: '#95a5a6', marginBottom: 16 },
  label: { fontSize: 12, color: '#95a5a6', marginBottom: 4 },
  input: {
    backgroundColor: '#2a2a4e', color: '#ecf0f1', padding: 10,
    borderRadius: 6, marginBottom: 12, fontSize: 16,
  },
  plot: { alignSelf: 'center', marginTop: 12, marginBottom: 12 },
  resultBox: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 16, marginTop: 12 },
  resultHeader: { fontSize: 16, fontWeight: 'bold', color: '#ecf0f1', marginBottom: 8 },
  resultLine: { color: '#bdc3c7', fontSize: 14, paddingVertical: 4 },
  val: { fontWeight: 'bold', color: '#ecf0f1' },
  note: { color: '#f39c12', fontSize: 12, marginTop: 8 },
});