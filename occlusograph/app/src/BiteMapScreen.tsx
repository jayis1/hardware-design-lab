/*
 * BiteMapScreen.tsx — Real-time 64-element occlusal force heatmap.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays a live heatmap of the 64-element bite-force array, rendered as an
 * 8×8 grid of colored cells (blue=0 N, red=peak). Shows per-tooth peak force,
 * contact sequence animation, and jaw-opening angle from the IMU.
 */

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { ble, ForceFrame } from './ble';

const GRID_COLS = 8;
const GRID_ROWS = 8;
const MAX_FORCE_MN = 300000; // 300 N in mN

export default function BiteMapScreen(): React.ReactElement {
  const [forces, setForces] = useState<number[]>(new Array(64).fill(0));
  const [peakElement, setPeakElement] = useState<number>(-1);
  const [peakForce, setPeakForce] = useState<number>(0);
  const [jawOpen, setJawOpen] = useState<number>(0);
  const [streaming, setStreaming] = useState<boolean>(false);
  const [history, setHistory] = useState<number[]>([]);

  useEffect(() => {
    if (!streaming) return;

    ble.subscribeForce((frame: ForceFrame) => {
      setForces(frame.forces_mN);

      // Find peak element.
      let maxIdx = 0;
      let maxVal = 0;
      for (let i = 0; i < frame.forces_mN.length; i++) {
        const absF = Math.abs(frame.forces_mN[i]);
        if (absF > maxVal) { maxVal = absF; maxIdx = i; }
      }
      setPeakElement(maxIdx);
      setPeakForce(maxVal);

      // Compute total force for history graph.
      const total = frame.forces_mN.reduce((sum, f) => sum + Math.abs(f), 0);
      setHistory(prev => {
        const next = [...prev, total];
        if (next.length > 100) next.shift();
        return next;
      });
    });

    return () => { /* cleanup in ble module */ };
  }, [streaming]);

  const startStream = async () => {
    setStreaming(true);
  };
  const stopStream = () => setStreaming(false);

  const forceToColor = (mN: number): string => {
    const absF = Math.min(Math.abs(mN), MAX_FORCE_MN);
    const ratio = absF / MAX_FORCE_MN;
    if (ratio < 0.001) return '#E3F2FD';
    if (ratio < 0.1) return '#BBDEFB';
    if (ratio < 0.2) return '#90CAF9';
    if (ratio < 0.3) return '#64B5F6';
    if (ratio < 0.4) return '#42A5F5';
    if (ratio < 0.5) return '#FFB74D';
    if (ratio < 0.6) return '#FF9800';
    if (ratio < 0.7) return '#FB8C00';
    if (ratio < 0.8) return '#F57C00';
    if (ratio < 0.9) return '#EF6C00';
    return '#E53935';
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Live Bite Map</Text>
        <TouchableOpacity
          style={[styles.button, streaming ? styles.stopButton : styles.startButton]}
          onPress={streaming ? stopStream : startStream}
        >
          <Text style={styles.buttonText}>
            {streaming ? 'Stop' : 'Start'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* 8×8 grid heatmap */}
      <View style={styles.grid}>
        {Array.from({ length: GRID_ROWS }).map((_, row) => (
          <View key={row} style={styles.row}>
            {Array.from({ length: GRID_COLS }).map((_, col) => {
              const idx = row * GRID_COLS + col;
              const isPeak = idx === peakElement;
              return (
                <View
                  key={col}
                  style={[
                    styles.cell,
                    {
                      backgroundColor: forceToColor(forces[idx]),
                      borderColor: isPeak ? '#fff' : 'transparent',
                      borderWidth: isPeak ? 2 : 0,
                    },
                  ]}
                >
                  <Text style={styles.cellLabel}>
                    {idx === peakElement && peakForce > 100 ? '◆' : ''}
                  </Text>
                </View>
              );
            })}
          </View>
        ))}
      </View>

      {/* Force history sparkline */}
      <View style={styles.sparklineContainer}>
        <Text style={styles.sparklineTitle}>Total Force (last 100 frames)</Text>
        <View style={styles.sparkline}>
          {history.map((val, i) => {
            const h = Math.min((val / (MAX_FORCE_MN * 10)) * 100, 100);
            return (
              <View
                key={i}
                style={[styles.sparkBar, { height: h }]}
              />
            );
          })}
        </View>
      </View>

      {/* Stats panel */}
      <View style={styles.statsPanel}>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Peak Element:</Text>
          <Text style={styles.statValue}>
            {peakElement >= 0 ? `#${peakElement}` : '--'}
          </Text>
        </View>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Peak Force:</Text>
          <Text style={styles.statValue}>
            {(peakForce / 1000).toFixed(1)} N
          </Text>
        </View>
        <View style={styles.statRow}>
          <Text style={styles.statLabel}>Jaw Opening:</Text>
          <Text style={styles.statValue}>{jawOpen}°</Text>
        </View>
      </View>

      <Text style={styles.footer}>
        Occlusograph v1.0 · Author: jayis1 · MIT License
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA', padding: 12 },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 10 },
  title: { fontSize: 18, fontWeight: 'bold', color: '#0D47A1' },
  button: { paddingHorizontal: 16, paddingVertical: 8, borderRadius: 8 },
  startButton: { backgroundColor: '#4CAF50' },
  stopButton: { backgroundColor: '#F44336' },
  buttonText: { color: '#fff', fontWeight: 'bold' },
  grid: { alignItems: 'center', marginVertical: 8 },
  row: { flexDirection: 'row' },
  cell: { width: 36, height: 36, margin: 1, justifyContent: 'center', alignItems: 'center' },
  cellLabel: { fontSize: 10, color: '#fff', fontWeight: 'bold' },
  sparklineContainer: { marginTop: 12, padding: 8, backgroundColor: '#fff', borderRadius: 8 },
  sparklineTitle: { fontSize: 12, color: '#666', marginBottom: 4 },
  sparkline: { flexDirection: 'row', height: 60, alignItems: 'flex-end' },
  sparkBar: { flex: 1, backgroundColor: '#1565C0', marginHorizontal: 0.5, minHeight: 1 },
  statsPanel: { marginTop: 12, padding: 12, backgroundColor: '#fff', borderRadius: 8 },
  statRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 4 },
  statLabel: { fontSize: 14, color: '#555' },
  statValue: { fontSize: 14, fontWeight: 'bold', color: '#0D47A1' },
  footer: { marginTop: 8, fontSize: 10, color: '#999', textAlign: 'center' },
});