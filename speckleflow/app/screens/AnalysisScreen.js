/**
 * AnalysisScreen.js — ROI statistics and data analysis
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useState, useMemo, useCallback } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView, TextInput,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';
import { useDevice, CMD } from '../utils/protocol';

export default function AnalysisScreen() {
  const { connected, assembler, roi, setRoi, sendCommand } = useDevice();
  const [stats, setStats] = useState({ mean: 0, min: 0, max: 0, std: 0, count: 0 });
  const [timeSeries, setTimeSeries] = useState([]);
  const [sampling, setSampling] = useState(false);
  const [sampleInterval, setSampleInterval] = useState(500);  // ms

  const computeStats = useCallback(() => {
    if (!assembler || !assembler.frame) return;
    const frame = assembler.frame;
    let sum = 0, sumSq = 0, min = 255, max = 0, count = 0;

    for (let y = roi.y; y < roi.y + roi.h && y < 480; y++) {
      for (let x = roi.x; x < roi.x + roi.w && x < 640; x++) {
        const v = frame[y * 640 + x];
        sum += v;
        sumSq += v * v;
        if (v < min) min = v;
        if (v > max) max = v;
        count++;
      }
    }

    if (count === 0) return;
    const mean = sum / count;
    const variance = (sumSq / count) - (mean * mean);
    const std = Math.sqrt(Math.max(0, variance));

    setStats({ mean: Math.round(mean), min, max, std: Math.round(std), count });
  }, [assembler, roi]);

  const startSampling = useCallback(() => {
    setSampling(true);
    setTimeSeries([]);
    const interval = setInterval(() => {
      if (!assembler || !assembler.frame) return;
      const frame = assembler.frame;
      let sum = 0, count = 0;
      for (let y = roi.y; y < roi.y + roi.h && y < 480; y++) {
        for (let x = roi.x; x < roi.x + roi.w && x < 640; x++) {
          sum += frame[y * 640 + x];
          count++;
        }
      }
      const mean = count > 0 ? Math.round(sum / count) : 0;
      setTimeSeries(prev => {
        const next = [...prev, { t: Date.now(), flow: mean }];
        return next.slice(-200);  // keep last 200 samples
      });
    }, sampleInterval);

    // Store interval ID for cleanup
    startSampling._interval = interval;
  }, [assembler, roi, sampleInterval]);

  const stopSampling = useCallback(() => {
    setSampling(false);
    if (startSampling._interval) {
      clearInterval(startSampling._interval);
      startSampling._interval = null;
    }
  }, []);

  const updateRoi = useCallback((field, value) => {
    const newRoi = { ...roi, [field]: Math.max(0, parseInt(value) || 0) };
    setRoi(newRoi);
    sendCommand(CMD.SET_ROI, newRoi.x, newRoi.y, newRoi.w > 0 ? newRoi.w : 160);
  }, [roi, setRoi, sendCommand]);

  // Render time series as text (simple sparkline)
  const sparkline = useMemo(() => {
    if (timeSeries.length < 2) return '';
    const values = timeSeries.map(s => s.flow);
    const min = Math.min(...values);
    const max = Math.max(...values);
    const range = max - min || 1;
    const bars = '▁▂▃▄▅▆▇█';
    return values.map(v => {
      const idx = Math.floor((v - min) / range * 7);
      return bars[Math.max(0, Math.min(7, idx))];
    }).join('');
  }, [timeSeries]);

  if (!connected) {
    return (
      <View style={styles.emptyContainer}>
        <Icon name="analytics" size={64} color="#444" />
        <Text style={styles.emptyText}>No device connected</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>ROI Analysis</Text>

      {/* ROI configuration */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Region of Interest</Text>
        <View style={styles.inputRow}>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>X</Text>
            <TextInput
              style={styles.input}
              value={String(roi.x)}
              onChangeText={v => updateRoi('x', v)}
              keyboardType="numeric"
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>Y</Text>
            <TextInput
              style={styles.input}
              value={String(roi.y)}
              onChangeText={v => updateRoi('y', v)}
              keyboardType="numeric"
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>W</Text>
            <TextInput
              style={styles.input}
              value={String(roi.w)}
              onChangeText={v => updateRoi('w', v)}
              keyboardType="numeric"
            />
          </View>
          <View style={styles.inputGroup}>
            <Text style={styles.inputLabel}>H</Text>
            <TextInput
              style={styles.input}
              value={String(roi.h)}
              onChangeText={v => updateRoi('h', v)}
              keyboardType="numeric"
            />
          </View>
        </View>
      </View>

      {/* Statistics */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>Statistics</Text>
          <TouchableOpacity onPress={computeStats} style={styles.refreshBtn}>
            <Icon name="refresh" size={18} color="#00d4ff" />
          </TouchableOpacity>
        </View>
        <View style={styles.statsGrid}>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Mean</Text>
            <Text style={styles.statValue}>{stats.mean}</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Min</Text>
            <Text style={styles.statValue}>{stats.min}</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Max</Text>
            <Text style={styles.statValue}>{stats.max}</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Std Dev</Text>
            <Text style={styles.statValue}>{stats.std}</Text>
          </View>
          <View style={styles.statItem}>
            <Text style={styles.statLabel}>Pixels</Text>
            <Text style={styles.statValue}>{stats.count}</Text>
          </View>
        </View>
      </View>

      {/* Time series */}
      <View style={styles.card}>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>Flow Time Series</Text>
          <TouchableOpacity
            onPress={sampling ? stopSampling : startSampling}
            style={styles.sampleBtn}
          >
            <Icon name={sampling ? 'stop' : 'play-arrow'} size={16} color="#fff" />
            <Text style={styles.sampleBtnText}>{sampling ? 'Stop' : 'Sample'}</Text>
          </TouchableOpacity>
        </View>
        {timeSeries.length > 0 ? (
          <>
            <Text style={styles.sparkline}>{sparkline}</Text>
            <Text style={styles.sparklineLabel}>
              {timeSeries.length} samples · {timeSeries[0].flow} → {timeSeries[timeSeries.length - 1].flow}
            </Text>
          </>
        ) : (
          <Text style={styles.emptyText}>Press Sample to start recording flow over time</Text>
        )}
      </View>

      {/* Interpretation guide */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Interpretation</Text>
        <Text style={styles.guideText}>
          • High flow values (red/warm) indicate active perfusion{'\n'}
          • Low flow values (blue/cool) indicate reduced or no blood flow{'\n'}
          • Compare ROI mean to surrounding tissue for relative assessment{'\n'}
          • Temporal variation indicates vasomotor reactivity{'\n'}
          • Static areas (no flow) show maximum speckle contrast (K→255)
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f1e', padding: 12 },
  emptyContainer: { flex: 1, backgroundColor: '#0f0f1e', alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#666', fontSize: 14, textAlign: 'center' },
  title: { color: '#00d4ff', fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 12, marginBottom: 12 },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 },
  cardTitle: { color: '#e0e0e0', fontSize: 14, fontWeight: '600' },
  inputRow: { flexDirection: 'row', gap: 8 },
  inputGroup: { flex: 1 },
  inputLabel: { color: '#666', fontSize: 10, marginBottom: 2 },
  input: { backgroundColor: '#0f0f1e', color: '#fff', borderRadius: 4, padding: 8, fontSize: 14 },
  refreshBtn: { padding: 4 },
  statsGrid: { flexDirection: 'row', flexWrap: 'wrap', gap: 8 },
  statItem: { backgroundColor: '#0f0f1e', borderRadius: 4, padding: 8, minWidth: 70, alignItems: 'center' },
  statLabel: { color: '#666', fontSize: 10, textTransform: 'uppercase' },
  statValue: { color: '#00d4ff', fontSize: 20, fontWeight: 'bold' },
  sampleBtn: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#0066cc', paddingHorizontal: 8, paddingVertical: 4, borderRadius: 4 },
  sampleBtnText: { color: '#fff', fontSize: 12, marginLeft: 4 },
  sparkline: { color: '#00d4ff', fontSize: 14, fontFamily: 'monospace', letterSpacing: 1 },
  sparklineLabel: { color: '#888', fontSize: 11, marginTop: 4 },
  guideText: { color: '#aaa', fontSize: 12, lineHeight: 20 },
});