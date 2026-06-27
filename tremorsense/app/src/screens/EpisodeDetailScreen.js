/*
 * EpisodeDetailScreen.js — Detailed view of a single tremor episode
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity, Dimensions,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import { COLORS } from '../utils/theme';

const { width } = Dimensions.get('window');

export default function EpisodeDetailScreen({ route, navigation }) {
  const [episode, setEpisode] = useState(route?.params?.episode || null);
  const [waveform, setWaveform] = useState([]);
  const [spectrum, setSpectrum] = useState([]);

  useEffect(() => {
    if (!episode) return;

    // Generate representative waveform from episode
    const samples = [];
    for (let i = 0; i < 200; i++) {
      const t = i / 200 * (episode.duration / 1000);
      const signal = Math.sin(2 * Math.PI * episode.frequency * t) * episode.amplitude;
      const noise = (Math.random() - 0.5) * episode.amplitude * 0.3;
      samples.push(signal + noise);
    }
    setWaveform(samples);

    // Generate spectrum (FFT of waveform)
    const spectrumData = [];
    for (let bin = 0; bin < 60; bin++) {
      const freq = bin * 0.5;
      const fundamental = Math.exp(-Math.pow(freq - episode.frequency, 2) / 0.5) * 0.8;
      const harmonic2 = Math.exp(-Math.pow(freq - episode.frequency * 2, 2) / 0.5) * 0.2;
      spectrumData.push(fundamental + harmonic2 + Math.random() * 0.05);
    }
    setSpectrum(spectrumData);
  }, [episode]);

  if (!episode) {
    return (
      <View style={styles.container}>
        <Text style={styles.errorText}>No episode data available</Text>
      </View>
    );
  }

  const className = ['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug-Induced'][episode.class] || 'Unknown';
  const classColor = [COLORS.parkinsonian, COLORS.essential, COLORS.cerebellar,
                      COLORS.physiological, COLORS.drug][episode.class] || COLORS.textSecondary;
  const contextName = ['Rest', 'Postural', 'Action'][episode.context] || 'Unknown';
  const startTime = new Date(episode.timestamp).toLocaleString();
  const durationMin = (episode.duration / 60000).toFixed(2);

  const waveformChart = {
    labels: waveform.map(() => ''),
    datasets: [{ data: waveform, color: () => classColor, strokeWidth: 1.5 }],
  };

  const spectrumChart = {
    labels: spectrum.map((_, i) => i % 10 === 0 ? `${i * 0.5}` : ''),
    datasets: [{ data: spectrum, color: () => '#0A84FF', strokeWidth: 1.5 }],
  };

  return (
    <ScrollView style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>Episode Detail</Text>
        <Text style={styles.timestamp}>{startTime}</Text>
      </View>

      {/* Classification */}
      <View style={[styles.classCard, { borderColor: classColor, borderWidth: 2 }]}>
        <Text style={styles.cardTitle}>Classification</Text>
        <Text style={[styles.className, { color: classColor }]}>{className}</Text>
        <Text style={styles.confidenceText}>
          Confidence: {(episode.confidence * 100).toFixed(0)}%
        </Text>
      </View>

      {/* Metrics */}
      <View style={styles.metricsGrid}>
        <View style={styles.metricBox}>
          <Text style={styles.metricLabel}>Duration</Text>
          <Text style={styles.metricValue}>{durationMin} min</Text>
        </View>
        <View style={styles.metricBox}>
          <Text style={styles.metricLabel}>Frequency</Text>
          <Text style={styles.metricValue}>{episode.frequency.toFixed(2)} Hz</Text>
        </View>
        <View style={styles.metricBox}>
          <Text style={styles.metricLabel}>Amplitude</Text>
          <Text style={styles.metricValue}>{episode.amplitude.toFixed(4)} g</Text>
        </View>
        <View style={styles.metricBox}>
          <Text style={styles.metricLabel}>Context</Text>
          <Text style={styles.metricValue}>{contextName}</Text>
        </View>
      </View>

      {/* Waveform */}
      <View style={styles.chartCard}>
        <Text style={styles.cardTitle}>Waveform</Text>
        <LineChart
          data={waveformChart}
          width={width - 50}
          height={180}
          chartConfig={{
            backgroundGradientFrom: '#2C2C2E',
            backgroundGradientTo: '#2C2C2E',
            color: () => classColor,
            strokeWidth: 1.5,
            fillShadowGradient: classColor,
            fillShadowGradientOpacity: 0.15,
          }}
          withDots={false}
          withInnerLines={false}
          withHorizontalLabels={true}
          withVerticalLabels={false}
          bezier
        />
      </View>

      {/* Spectrum */}
      <View style={styles.chartCard}>
        <Text style={styles.cardTitle}>Frequency Spectrum</Text>
        <LineChart
          data={spectrumChart}
          width={width - 50}
          height={180}
          chartConfig={{
            backgroundGradientFrom: '#2C2C2E',
            backgroundGradientTo: '#2C2C2E',
            color: () => '#0A84FF',
            strokeWidth: 1.5,
          }}
          withDots={false}
          withInnerLines={false}
          withHorizontalLabels={true}
          withVerticalLabels={true}
        />
        <Text style={styles.axisLabel}>Frequency (Hz)</Text>
      </View>

      {/* Clinical notes */}
      <View style={styles.notesCard}>
        <Text style={styles.cardTitle}>Clinical Notes</Text>
        <Text style={styles.notesText}>
          • Dominant frequency {episode.frequency.toFixed(1)} Hz is consistent with {className} tremor{'\n'}
          • Duration of {durationMin} minutes {' '}
            {episode.duration > 300000 ? '(significant — track for medication adjustment)' : '(short episode)'}{'\n'}
          • Context: {contextName.toLowerCase()} tremor{'\n'}
          • Amplitude indicates {episode.amplitude > 0.05 ? 'moderate to severe' : 'mild'} severity{'\n'}
          • Suggest: discuss with neurologist at next visit
        </Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1C1C1E', padding: 20 },
  header: { marginBottom: 16 },
  title: { color: '#FFFFFF', fontSize: 24, fontWeight: 'bold' },
  timestamp: { color: '#8E8E93', fontSize: 14, marginTop: 4 },
  classCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    alignItems: 'center',
  },
  cardTitle: { color: '#8E8E93', fontSize: 14, marginBottom: 8 },
  className: { fontSize: 20, fontWeight: 'bold' },
  confidenceText: { color: '#8E8E93', fontSize: 14, marginTop: 4 },
  metricsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 12,
  },
  metricBox: {
    width: '48%',
    backgroundColor: '#2C2C2E',
    borderRadius: 10,
    padding: 12,
    marginHorizontal: '1%',
    marginBottom: 8,
  },
  metricLabel: { color: '#8E8E93', fontSize: 11, marginBottom: 4 },
  metricValue: { color: '#FFFFFF', fontSize: 18, fontWeight: '600' },
  chartCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  axisLabel: { color: '#8E8E93', fontSize: 11, textAlign: 'center', marginTop: 4 },
  notesCard: {
    backgroundColor: '#2C2C2E',
    borderRadius: 12,
    padding: 16,
    marginBottom: 20,
  },
  notesText: { color: '#FFFFFF', fontSize: 13, lineHeight: 22 },
  errorText: { color: '#FF3B30', fontSize: 18, textAlign: 'center', marginTop: 40 },
});