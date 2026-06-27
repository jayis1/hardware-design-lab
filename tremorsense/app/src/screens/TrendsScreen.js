/*
 * TrendsScreen.js — 7/30/90-day tremor trend analysis
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { LineChart, BarChart } from 'react-native-chart-kit';
import { COLORS } from '../utils/theme';
import { Dimensions } from 'react-native';

const { width } = Dimensions.get('window');

export default function TrendsScreen() {
  const [range, setRange] = useState(7); // 7, 30, or 90 days
  const [tremorHours, setTremorHours] = useState([]);
  const [avgFreq, setAvgFreq] = useState([]);
  const [avgAmplitude, setAvgAmplitude] = useState([]);
  const [classDistribution, setClassDistribution] = useState([]);

  useEffect(() => {
    loadTrends(range);
  }, [range]);

  const loadTrends = (days) => {
    // In production: fetch from AsyncStorage or BLE download
    // Generate representative trend data
    const hours = [];
    const freqs = [];
    const amps = [];
    const labels = [];

    for (let i = days - 1; i >= 0; i--) {
      const date = new Date(Date.now() - i * 86400000);
      labels.push(date.toLocaleDateString('en-US', { month: 'short', day: 'numeric' }));

      // Simulate trend data
      const baseHours = 2.5 - (days - i) * 0.01; // Slight worsening trend
      const noise = (Math.random() - 0.5) * 1.5;
      hours.push(Math.max(0, baseHours + noise));

      const baseFreq = 4.8 + (days - i) * 0.005; // Slight frequency drift
      freqs.push(baseFreq + (Math.random() - 0.5) * 0.3);

      const baseAmp = 0.04 + (days - i) * 0.0002;
      amps.push(baseAmp + (Math.random() - 0.5) * 0.02);
    }

    setTremorHours(hours);
    setAvgFreq(freqs);
    setAvgAmplitude(amps);
    setClassDistribution([0.55, 0.25, 0.05, 0.10, 0.05]); // PD, ET, Cer, Phys, Drug
  };

  const hoursChartData = {
    labels: tremorHours.map((_, i) => ''),
    datasets: [{
      data: tremorHours,
      color: () => COLORS.parkinsonian,
      strokeWidth: 2,
    }],
  };

  const freqChartData = {
    labels: avgFreq.map((_, i) => ''),
    datasets: [{
      data: avgFreq,
      color: () => COLORS.essential,
      strokeWidth: 2,
    }],
  };

  const classChartData = {
    labels: ['PD', 'ET', 'Cer', 'Phys', 'Drug'],
    datasets: [{
      data: classDistribution.map(d => d * 100),
      colors: [
        () => COLORS.parkinsonian,
        () => COLORS.essential,
        () => COLORS.cerebellar,
        () => COLORS.physiological,
        () => COLORS.drug,
      ],
    }],
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Trends</Text>
      <Text style={styles.subtitle}>Disease progression tracking</Text>

      {/* Range selector */}
      <View style={styles.rangeSelector}>
        {[7, 30, 90].map(d => (
          <TouchableOpacity
            key={d}
            style={[styles.rangeButton, range === d && styles.rangeButtonActive]}
            onPress={() => setRange(d)}
          >
            <Text style={[styles.rangeText, range === d && styles.rangeTextActive]}>
              {d} days
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      <ScrollView style={styles.scrollContent}>
        {/* Daily tremor hours */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Daily Tremor Duration (hours)</Text>
          <Text style={styles.summaryText}>
            Average: {(tremorHours.reduce((a, b) => a + b, 0) / tremorHours.length || 0).toFixed(1)}h/day
          </Text>
          <LineChart
            data={hoursChartData}
            width={width - 50}
            height={150}
            chartConfig={{
              backgroundGradientFrom: '#2C2C2E',
              backgroundGradientTo: '#2C2C2E',
              color: () => COLORS.parkinsonian,
              strokeWidth: 2,
              fillShadowGradient: COLORS.parkinsonian,
              fillShadowGradientOpacity: 0.2,
            }}
            withDots={false}
            withInnerLines={false}
            withOuterLines={true}
            bezier
          />
        </View>

        {/* Average frequency trend */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Dominant Frequency Trend (Hz)</Text>
          <Text style={styles.summaryText}>
            Average: {(avgFreq.reduce((a, b) => a + b, 0) / avgFreq.length || 0).toFixed(2)} Hz
          </Text>
          <LineChart
            data={freqChartData}
            width={width - 50}
            height={150}
            chartConfig={{
              backgroundGradientFrom: '#2C2C2E',
              backgroundGradientTo: '#2C2C2E',
              color: () => COLORS.essential,
              strokeWidth: 2,
            }}
            withDots={false}
            withInnerLines={false}
            bezier
          />
        </View>

        {/* Average amplitude trend */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Amplitude Trend (g)</Text>
          <Text style={styles.summaryText}>
            Average: {(avgAmplitude.reduce((a, b) => a + b, 0) / avgAmplitude.length || 0).toFixed(4)} g
          </Text>
          <LineChart
            data={{
              labels: avgAmplitude.map(() => ''),
              datasets: [{
                data: avgAmplitude,
                color: () => COLORS.cerebellar,
                strokeWidth: 2,
              }],
            }}
            width={width - 50}
            height={120}
            chartConfig={{
              backgroundGradientFrom: '#2C2C2E',
              backgroundGradientTo: '#2C2C2E',
              color: () => COLORS.cerebellar,
              strokeWidth: 2,
            }}
            withDots={false}
            withInnerLines={false}
            bezier
          />
        </View>

        {/* Class distribution */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Tremor Type Distribution</Text>
          <BarChart
            data={classChartData}
            width={width - 50}
            height={180}
            chartConfig={{
              backgroundGradientFrom: '#2C2C2E',
              backgroundGradientTo: '#2C2C2E',
              color: () => '#FFFFFF',
              barPercentage: 0.6,
            }}
            withInnerLines={false}
            showBarTops={true}
          />
        </View>

        {/* Clinical interpretation */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Clinical Summary</Text>
          <Text style={styles.summaryText}>
            Over {range} days: tremor duration shows a {" "}
            {tremorHours[0] > tremorHours[tremorHours.length - 1] ? 'decreasing' : 'increasing'} trend.
            {"\n\n"}
            Mean frequency: {(avgFreq.reduce((a, b) => a + b, 0) / avgFreq.length || 0).toFixed(2)} Hz —
            {" "}consistent with {" "}
            {avgFreq.reduce((a, b) => a + b, 0) / avgFreq.length < 6 ? 'Parkinsonian' : 'Essential'} tremor.
            {"\n\n"}
            Dominant type: Parkinsonian ({(classDistribution[0] * 100).toFixed(0)}%)
          </Text>
        </View>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#8E8E93', fontSize: 14, marginBottom: 16 },
  rangeSelector: { flexDirection: 'row', marginBottom: 16 },
  rangeButton: {
    flex: 1,
    padding: 10,
    borderRadius: 8,
    backgroundColor: '#2C2C2E',
    marginHorizontal: 4,
    alignItems: 'center',
  },
  rangeButtonActive: { backgroundColor: '#0A84FF' },
  rangeText: { color: '#8E8E93', fontSize: 14 },
  rangeTextActive: { color: '#FFFFFF', fontWeight: '600' },
  scrollContent: { flex: 1 },
  card: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  cardTitle: { color: '#8E8E93', fontSize: 14, marginBottom: 8 },
  summaryText: { color: '#FFFFFF', fontSize: 13, marginBottom: 8, lineHeight: 18 },
});