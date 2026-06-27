/*
 * DashboardScreen.js — Main dashboard for TremorSense Connect
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState, useEffect } from 'react';
import { View, Text, StyleSheet, Dimensions, TouchableOpacity } from 'react-native';
import { LineChart, ProgressChart } from 'react-native-chart-kit';
import { TremorContext } from '../models/TremorContext';
import { COLORS } from '../utils/theme';

const { width } = Dimensions.get('window');

const TREMOR_CLASS_NAMES = [
  'Parkinsonian',
  'Essential',
  'Cerebellar',
  'Physiological',
  'Drug-Induced',
];

const TREMOR_CLASS_COLORS = [
  COLORS.parkinsonian,
  COLORS.essential,
  COLORS.cerebellar,
  COLORS.physiological,
  COLORS.drug,
];

export default function DashboardScreen() {
  const { bleState, deviceData, theme } = useContext(TremorContext);
  const [score, setScore] = useState(0);
  const [tremorHours, setTremorHours] = useState(0);
  const [currentFreq, setCurrentFreq] = useState(0);
  const [currentAmp, setCurrentAmp] = useState(0);
  const [currentClass, setCurrentClass] = useState(-1);
  const [isTremoring, setIsTremoring] = useState(false);
  const [batteryPct, setBatteryPct] = useState(80);
  const [waveform, setWaveform] = useState([]);

  useEffect(() => {
    if (!deviceData) return;

    // Parse BLE notification data (14 bytes)
    const data = deviceData;
    if (data.frequency !== undefined) setCurrentFreq(data.frequency);
    if (data.amplitude !== undefined) setCurrentAmp(data.amplitude);
    if (data.tremorClass !== undefined && data.tremorClass !== 0xFF) {
      setCurrentClass(data.tremorClass);
      setIsTremoring(true);
    } else {
      setIsTremoring(false);
    }
    if (data.score !== undefined) setScore(data.score);
    if (data.battery !== undefined) setBatteryPct(data.battery);

    // Update waveform with new amplitude sample
    setWaveform((prev) => {
      const newWave = [...prev, data.amplitude || 0];
      if (newWave.length > 50) newWave.shift();
      return newWave;
    });
  }, [deviceData]);

  const chartData = {
    labels: waveform.map((_, i) => ''),
    datasets: [{
      data: waveform.length > 0 ? waveform : [0, 0, 0, 0, 0],
      color: () => isTremoring ? COLORS.parkinsonian : COLORS.physiological,
      strokeWidth: 2,
    }],
  };

  const scoreData = {
    labels: ['Score'],
    data: [score / 100],
  };

  return (
    <View style={styles.container}>
      {/* Connection status bar */}
      <View style={styles.statusBar}>
        <View style={[styles.statusDot,
          { backgroundColor: bleState === 'connected' ? '#34C759' : '#FF3B30' }]} />
        <Text style={styles.statusText}>
          {bleState === 'connected' ? 'Connected' : 'Disconnected'}
        </Text>
        <Text style={styles.batteryText}>🔋 {batteryPct}%</Text>
      </View>

      {/* Tremor status indicator */}
      <View style={styles.statusCard}>
        <Text style={styles.statusLabel}>Current Status</Text>
        <Text style={[styles.statusText2,
          { color: isTremoring ? TREMOR_CLASS_COLORS[currentClass] || '#FF3B30' : '#34C759' }]}>
          {isTremoring ? 'TREMOR DETECTED' : 'STABLE'}
        </Text>
        {isTremoring && currentClass >= 0 && (
          <Text style={styles.className}>
            {TREMOR_CLASS_NAMES[currentClass]}
          </Text>
        )}
        {isTremoring && (
          <View style={styles.metricRow}>
            <View style={styles.metric}>
              <Text style={styles.metricLabel}>Frequency</Text>
              <Text style={styles.metricValue}>{currentFreq.toFixed(2)} Hz</Text>
            </View>
            <View style={styles.metric}>
              <Text style={styles.metricLabel}>Amplitude</Text>
              <Text style={styles.metricValue}>{currentAmp.toFixed(4)} g</Text>
            </View>
          </View>
        )}
      </View>

      {/* Daily score */}
      <View style={styles.scoreCard}>
        <Text style={styles.cardTitle}>Daily Tremor Score</Text>
        <Text style={styles.scoreValue}>{score}/100</Text>
        <ProgressChart
          data={scoreData}
          width={width - 40}
          height={80}
          strokeWidth={10}
          radius={32}
          chartConfig={{
            backgroundGradientFrom: theme.surface,
            backgroundGradientTo: theme.surface,
            color: () => score < 30 ? '#34C759' : score < 70 ? '#FF9500' : '#FF3B30',
          }}
        />
      </View>

      {/* Live waveform */}
      <View style={styles.waveformCard}>
        <Text style={styles.cardTitle}>Live Waveform</Text>
        <LineChart
          data={chartData}
          width={width - 40}
          height={120}
          chartConfig={{
            backgroundGradientFrom: theme.surface,
            backgroundGradientTo: theme.surface,
            color: () => isTremoring ? COLORS.parkinsonian : COLORS.physiological,
            strokeWidth: 2,
          }}
          withDots={false}
          withInnerLines={false}
          withOuterLines={false}
          withHorizontalLabels={false}
          withVerticalLabels={false}
          bezier
        />
      </View>

      {/* Quick stats */}
      <View style={styles.statsRow}>
        <View style={styles.statBox}>
          <Text style={styles.statLabel}>Tremor Today</Text>
          <Text style={styles.statValue}>{tremorHours.toFixed(1)}h</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={styles.statLabel}>Battery</Text>
          <Text style={styles.statValue}>{batteryPct}%</Text>
        </View>
        <View style={styles.statBox}>
          <Text style={styles.statLabel}>Connection</Text>
          <Text style={styles.statValue} style={{ fontSize: 12 }}>
            {bleState === 'connected' ? 'Live' : 'Offline'}
          </Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#1C1C1E',
    padding: 10,
  },
  statusBar: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 8,
    paddingHorizontal: 12,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  statusText: {
    color: '#8E8E93',
    fontSize: 14,
    flex: 1,
  },
  batteryText: {
    color: '#8E8E93',
    fontSize: 14,
  },
  statusCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
  },
  statusLabel: {
    color: '#8E8E93',
    fontSize: 12,
    marginBottom: 4,
  },
  statusText2: {
    fontSize: 24,
    fontWeight: 'bold',
  },
  className: {
    color: '#8E8E93',
    fontSize: 14,
    marginTop: 4,
  },
  metricRow: {
    flexDirection: 'row',
    marginTop: 12,
  },
  metric: {
    flex: 1,
  },
  metricLabel: {
    color: '#8E8E93',
    fontSize: 11,
  },
  metricValue: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
  },
  scoreCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
    alignItems: 'center',
  },
  cardTitle: {
    color: '#8E8E93',
    fontSize: 14,
    marginBottom: 8,
  },
  scoreValue: {
    color: '#FFFFFF',
    fontSize: 32,
    fontWeight: 'bold',
  },
  waveformCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  statBox: {
    flex: 1,
    backgroundColor: '#2C2C2E',
    borderRadius: 10,
    padding: 12,
    marginHorizontal: 4,
    alignItems: 'center',
  },
  statLabel: {
    color: '#8E8E93',
    fontSize: 11,
    marginBottom: 4,
  },
  statValue: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
  },
});