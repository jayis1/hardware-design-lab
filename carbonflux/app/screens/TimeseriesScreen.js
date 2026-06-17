/**
 * TimeseriesScreen.js — Historical flux over configurable time window.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {useState, useContext} from 'react';
import {View, ScrollView, Text, StyleSheet, TouchableOpacity, Dimensions} from 'react-native';
import {LineChart} from 'react-native-chart-kit';
import BLEContext from '../components/BLEContext';

const screenWidth = Dimensions.get('window').width;
const WINDOWS = ['1h', '1d', '1w', '1m'];

const TimeseriesScreen = () => {
  const {deviceData} = useContext(BLEContext);
  const [window, setWindow] = useState('1d');

  // Mock historical data — in production this comes from BLE flash dump
  const generateMockHistory = () => {
    const pts = window === '1h' ? 60 : window === '1d' ? 48 : window === '1w' ? 336 : 720;
    return Array.from({length: pts}, (_, i) => ({
      time: i,
      flux: 1.5 + Math.sin(i * 0.1) * 0.8 + Math.random() * 0.3,
      co2: 420 + Math.sin(i * 0.05) * 15,
      temp: 22 + Math.sin(i * 0.02) * 4,
    }));
  };

  const history = generateMockHistory();

  const chartData = {
    labels: history
      .filter((_, i) => i % Math.floor(history.length / 6) === 0)
      .map(d => `${d.time}h`),
    datasets: [
      {
        data: history.map(d => d.flux),
        color: (opacity = 1) => `rgba(76, 175, 80, ${opacity})`,
        strokeWidth: 2,
      },
    ],
  };

  const chartConfig = {
    backgroundColor: '#1a1a2e',
    backgroundGradientFrom: '#1a1a2e',
    backgroundGradientTo: '#16213e',
    decimalPlaces: 1,
    color: (opacity = 1) => `rgba(255, 255, 255, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(200, 200, 200, ${opacity})`,
    style: {borderRadius: 8},
  };

  const avgFlux = history.reduce((s, d) => s + d.flux, 0) / history.length;
  const maxFlux = Math.max(...history.map(d => d.flux));
  const minFlux = Math.min(...history.map(d => d.flux));
  const totalCO2 = history.reduce((s, d) => s + d.co2, 0);

  return (
    <ScrollView style={styles.container}>
      {/* Time window selector */}
      <View style={styles.windowRow}>
        {WINDOWS.map(w => (
          <TouchableOpacity
            key={w}
            style={[styles.windowBtn, window === w && styles.windowBtnActive]}
            onPress={() => setWindow(w)}>
            <Text style={[styles.windowText, window === w && styles.windowTextActive]}>
              {w}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Flux chart */}
      <LineChart
        data={chartData}
        width={screenWidth - 16}
        height={220}
        chartConfig={chartConfig}
        bezier
        style={styles.chart}
      />

      {/* Statistics */}
      <View style={styles.statsGrid}>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Avg Flux</Text>
          <Text style={styles.statValue}>{avgFlux.toFixed(2)}</Text>
          <Text style={styles.statUnit}>µmol/m²/s</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Max Flux</Text>
          <Text style={[styles.statValue, {color: '#FF9800'}]}>{maxFlux.toFixed(2)}</Text>
          <Text style={styles.statUnit}>µmol/m²/s</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Min Flux</Text>
          <Text style={[styles.statValue, {color: '#2196F3'}]}>{minFlux.toFixed(2)}</Text>
          <Text style={styles.statUnit}>µmol/m²/s</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Samples</Text>
          <Text style={styles.statValue}>{history.length}</Text>
          <Text style={styles.statUnit}>measurements</Text>
        </View>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  windowRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    marginVertical: 8,
  },
  windowBtn: {
    paddingHorizontal: 20,
    paddingVertical: 8,
    marginHorizontal: 4,
    borderRadius: 16,
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#333',
  },
  windowBtnActive: {
    backgroundColor: '#4CAF50',
    borderColor: '#4CAF50',
  },
  windowText: {
    color: '#888',
    fontSize: 14,
  },
  windowTextActive: {
    color: '#fff',
    fontWeight: '700',
  },
  chart: {
    borderRadius: 8,
    marginHorizontal: 8,
  },
  statsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    margin: 8,
  },
  statCard: {
    width: '48%',
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    margin: '1%',
    borderWidth: 1,
    borderColor: '#333',
    alignItems: 'center',
  },
  statLabel: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  statValue: {
    color: '#e0e0e0',
    fontSize: 24,
    fontWeight: '700',
    fontFamily: 'monospace',
    marginVertical: 4,
  },
  statUnit: {
    color: '#666',
    fontSize: 11,
  },
});

export default TimeseriesScreen;