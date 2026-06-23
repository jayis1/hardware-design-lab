/**
 * EnviroScreen.js — Environmental data view for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Displays soil moisture, temperature, and CO₂ time-series charts
 * correlated with fungal activity.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import { useBle } from '../utils/BleContext';
import EnviroCard from '../components/EnviroCard';

const SCREEN_WIDTH = 320;

export default function EnviroScreen() {
  const { env, epoch, classLabels } = useBle();
  const [history, setHistory] = useState({
    moisture: [],
    temp: [],
    co2: [],
    spikeRate: [],
    labels: [],
  });

  // Accumulate history points (simulated in absence of real data).
  useEffect(() => {
    if (env) {
      setHistory((prev) => {
        const maxPoints = 30;
        const label = new Date().toLocaleTimeString().slice(0, 5);
        return {
          moisture: [...prev.moisture.slice(-maxPoints + 1), env.moisturePct],
          temp: [...prev.temp.slice(-maxPoints + 1), env.tempC],
          co2: [...prev.co2.slice(-maxPoints + 1), env.co2Ppm],
          spikeRate: [...prev.spikeRate.slice(-maxPoints + 1), epoch?.spikeRate || 0],
          labels: [...prev.labels.slice(-maxPoints + 1), label],
        };
      });
    }
  }, [env, epoch]);

  // Simulate initial data if not connected.
  useEffect(() => {
    if (history.moisture.length === 0) {
      const mockLabels = Array.from({ length: 10 }, (_, i) => `${i}m`);
      setHistory({
        moisture: [35, 36, 37, 35, 34, 33, 32, 33, 34, 35],
        temp: [22, 22.1, 22.3, 22.5, 22.8, 23.0, 23.1, 22.9, 22.7, 22.5],
        co2: [410, 420, 435, 450, 480, 520, 540, 510, 490, 460],
        spikeRate: [2, 3, 5, 8, 12, 15, 14, 10, 6, 3],
        labels: mockLabels,
      });
    }
  }, []);

  const chartConfig = {
    backgroundGradientFrom: '#1a2a1a',
    backgroundGradientTo: '#1a2a1a',
    color: (opacity = 1) => `rgba(76, 175, 80, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(170, 170, 170, ${opacity})`,
    strokeWidth: 2,
    propsForDots: { r: '2', strokeWidth: '1', stroke: '#4CAF50' },
  };

  const emptyLabels = history.labels.map((_, i) => (i % 3 === 0 ? history.labels[i] : ''));

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Environmental Monitor</Text>
      </View>

      <ScrollView style={styles.content}>
        {/* Current values */}
        <View style={styles.cardsRow}>
          <EnviroCard
            title="Soil Moisture"
            value={env ? `${env.moisturePct.toFixed(1)}%` : '—'}
            unit="VWC"
            color="#2196F3"
            icon="💧"
          />
          <EnviroCard
            title="Temperature"
            value={env ? `${env.tempC.toFixed(1)}°C` : '—'}
            unit="soil"
            color="#FF9800"
            icon="🌡️"
          />
          <EnviroCard
            title="CO₂"
            value={env ? `${env.co2Ppm}` : '—'}
            unit="ppm"
            color="#9C27B0"
            icon="🫧"
          />
        </View>

        {/* Epoch correlation */}
        {epoch && (
          <View style={styles.epochCard}>
            <Text style={styles.epochTitle}>
              Activity: {classLabels[epoch.classLabel] || 'Unknown'}
            </Text>
            <Text style={styles.epochDetail}>
              Spike Rate: {epoch.spikeRate}/s • Propagation: {epoch.propagationCount}
            </Text>
            <Text style={styles.epochDetail}>
              Mean Amplitude: {epoch.meanAmplitudeUv} µV
            </Text>
            <Text style={styles.epochDetail}>
              ISI CV: {(epoch.isiCvX100 / 100).toFixed(2)}
            </Text>
          </View>
        )}

        {/* Moisture chart */}
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>Soil Moisture (% VWC)</Text>
          <LineChart
            data={{ labels: emptyLabels, datasets: [{ data: history.moisture }] }}
            width={SCREEN_WIDTH}
            height={120}
            chartConfig={chartConfig}
            bezier
            style={styles.chart}
          />
        </View>

        {/* Temperature chart */}
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>Soil Temperature (°C)</Text>
          <LineChart
            data={{ labels: emptyLabels, datasets: [{ data: history.temp }] }}
            width={SCREEN_WIDTH}
            height={120}
            chartConfig={{
              ...chartConfig,
              color: (opacity = 1) => `rgba(255, 152, 0, ${opacity})`,
            }}
            bezier
            style={styles.chart}
          />
        </View>

        {/* CO₂ chart */}
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>CO₂ (ppm)</Text>
          <LineChart
            data={{ labels: emptyLabels, datasets: [{ data: history.co2 }] }}
            width={SCREEN_WIDTH}
            height={120}
            chartConfig={{
              ...chartConfig,
              color: (opacity = 1) => `rgba(156, 39, 176, ${opacity})`,
            }}
            bezier
            style={styles.chart}
          />
        </View>

        {/* Spike rate correlation */}
        <View style={styles.chartContainer}>
          <Text style={styles.chartTitle}>Fungal Spike Rate (spikes/s)</Text>
          <LineChart
            data={{ labels: emptyLabels, datasets: [{ data: history.spikeRate }] }}
            width={SCREEN_WIDTH}
            height={120}
            chartConfig={{
              ...chartConfig,
              color: (opacity = 1) => `rgba(255, 87, 34, ${opacity})`,
            }}
            bezier
            style={styles.chart}
          />
        </View>
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1a0a' },
  header: { padding: 16, borderBottomWidth: 1, borderBottomColor: '#2a3a2a' },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  content: { flex: 1, padding: 8 },
  cardsRow: { flexDirection: 'row', gap: 8, marginBottom: 12 },
  epochCard: {
    backgroundColor: '#1a2a1a', borderRadius: 12, padding: 16, marginBottom: 12,
  },
  epochTitle: { color: '#4CAF50', fontSize: 16, fontWeight: 'bold', marginBottom: 8 },
  epochDetail: { color: '#aaa', fontSize: 13, marginTop: 4 },
  chartContainer: {
    backgroundColor: '#1a2a1a', borderRadius: 12, padding: 8, marginBottom: 12,
  },
  chartTitle: { color: '#fff', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  chart: { borderRadius: 8 },
});