/**
 * TelemetryScreen — Historical telemetry and device logs
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, ViewStyle } from 'react-native';
import { useDeviceStore } from '../store/deviceStore';
import { BatteryWidget } from '../components/BatteryWidget';
import type { Telemetry } from '../types';

export const TelemetryScreen: React.FC = () => {
  const { telemetry, thermalFrame, lastGesture, config } = useDeviceStore();
  const [history, setHistory] = useState<Telemetry[]>([]);

  useEffect(() => {
    if (telemetry) {
      setHistory(prev => [...prev.slice(-49), telemetry]);
    }
  }, [telemetry]);

  const renderTempChart = () => {
    if (history.length < 2) return null;
    const temps = history.map(t => t.skinTempAvgC);
    const minT = Math.min(...temps);
    const maxT = Math.max(...temps);
    const range = maxT - minT || 1;

    return (
      <View style={styles.chart}>
        <Text style={styles.chartTitle}>Skin Temperature History</Text>
        <View style={styles.chartBars}>
          {history.map((t, i) => {
            const normalized = (t.skinTempAvgC - minT) / range;
            const height = Math.max(2, normalized * 60);
            return (
              <View
                key={i}
                style={[styles.chartBar, { height }] as ViewStyle}
              />
            );
          })}
        </View>
        <Text style={styles.chartAxis}>
          Min: {minT}°C · Max: {maxT}°C
        </Text>
      </View>
    );
  };

  const renderBatteryChart = () => {
    if (history.length < 2) return null;
    return (
      <View style={styles.chart}>
        <Text style={styles.chartTitle}>Battery History</Text>
        <View style={styles.chartBars}>
          {history.map((t, i) => {
            const height = Math.max(2, (t.batteryPct / 100) * 60);
            const color = t.batteryPct > 50 ? '#4a4' : t.batteryPct > 20 ? '#aa4' : '#a44';
            return (
              <View
                key={i}
                style={[styles.chartBar, { height, backgroundColor: color }] as ViewStyle}
              />
            );
          })}
        </View>
      </View>
    );
  };

  return (
    <ScrollView style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>Telemetry</Text>
        <Text style={styles.subtitle}>Real-time device data</Text>
      </View>

      {/* Current telemetry */}
      <BatteryWidget telemetry={telemetry} />

      {/* Thermal array detail */}
      <Text style={styles.sectionTitle}>Thermal Array Detail</Text>
      <View style={styles.detailCard}>
        <Text style={styles.detailRow}>
          Active: {thermalFrame?.active ? 'Yes' : 'No'}
        </Text>
        <Text style={styles.detailRow}>
          Cells: {thermalFrame?.cells.length ?? 0} / 96
        </Text>
        {thermalFrame?.cells.slice(0, 8).map((c, i) => (
          <Text key={i} style={styles.detailCell}>
            [{c.row},{c.col}] target={c.targetTempC}°C measured={c.measuredTempC.toFixed(1)}°C
          </Text>
        ))}
      </View>

      {/* Gesture log */}
      <Text style={styles.sectionTitle}>Last Gesture</Text>
      <View style={styles.detailCard}>
        <Text style={styles.detailRow}>Gesture: {lastGesture}</Text>
      </View>

      {/* Device configuration */}
      <Text style={styles.sectionTitle}>Device Configuration</Text>
      <View style={styles.detailCard}>
        <Text style={styles.detailRow}>Intensity Scale: {config.intensityScale}/255</Text>
        <Text style={styles.detailRow}>PID Kp: {config.pidKp}</Text>
        <Text style={styles.detailRow}>PID Ki: {config.pidKi}</Text>
        <Text style={styles.detailRow}>PID Kd: {config.pidKd}</Text>
        <Text style={styles.detailRow}>Max Temp: {config.maxTempC}°C</Text>
        <Text style={styles.detailRow}>Min Temp: {config.minTempC}°C</Text>
        <Text style={styles.detailRow}>Power Mode: {config.powerMode}</Text>
      </View>

      {/* Charts */}
      {renderTempChart()}
      {renderBatteryChart()}

      {/* Raw data dump */}
      <Text style={styles.sectionTitle}>Raw Telemetry ({history.length} samples)</Text>
      <View style={styles.rawCard}>
        {history.slice(-10).reverse().map((t, i) => (
          <Text key={i} style={styles.rawLine}>
            bat={t.batteryPct}% sol={t.solarMv}mV tmp={t.skinTempAvgC}°C rssi={t.loraRssi} st={t.state}
          </Text>
        ))}
        {history.length === 0 && (
          <Text style={styles.rawLine}>No data yet — connect to device</Text>
        )}
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 13, marginTop: 4 },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 8 },
  detailCard: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8 },
  detailRow: { color: '#ccc', fontSize: 13, fontFamily: 'monospace', marginVertical: 2 },
  detailCell: { color: '#888', fontSize: 11, fontFamily: 'monospace', marginVertical: 1 },
  chart: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8, marginVertical: 4 },
  chartTitle: { color: '#aaa', fontSize: 13, marginBottom: 8 },
  chartBars: { flexDirection: 'row', alignItems: 'flex-end', height: 64, gap: 2 },
  chartBar: { width: 4, backgroundColor: '#fa4', borderRadius: 1 },
  chartAxis: { color: '#666', fontSize: 11, marginTop: 4, fontFamily: 'monospace' },
  rawCard: { backgroundColor: '#111', borderRadius: 10, padding: 12, marginHorizontal: 8, marginBottom: 20 },
  rawLine: { color: '#6a6', fontSize: 10, fontFamily: 'monospace', marginVertical: 1 },
});