/**
 * FluxChart.js — Realtime CO₂ flux chart component.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays a line chart of CO₂ concentration over time during
 * accumulation, with flux rate annotation. Uses react-native-chart-kit.
 */

import React from 'react';
import {View, Text, StyleSheet, Dimensions} from 'react-native';
import {LineChart} from 'react-native-chart-kit';

const screenWidth = Dimensions.get('window').width;

const FluxChart = ({data, fluxRate, rSquared}) => {
  if (!data || data.length < 2) {
    return (
      <View style={styles.container}>
        <Text style={styles.placeholder}>No data — waiting for measurement...</Text>
      </View>
    );
  }

  const labels = data.map((_, i) => {
    if (i % Math.max(1, Math.floor(data.length / 6)) === 0) {
      return `${i * 2}s`;  // 2-second intervals
    }
    return '';
  });

  const chartData = {
    labels: labels,
    datasets: [
      {
        data: data,
        color: (opacity = 1) => `rgba(76, 175, 80, ${opacity})`,
        strokeWidth: 2,
      },
    ],
  };

  const chartConfig = {
    backgroundColor: '#1a1a2e',
    backgroundGradientFrom: '#1a1a2e',
    backgroundGradientTo: '#16213e',
    decimalPlaces: 0,
    color: (opacity = 1) => `rgba(255, 255, 255, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(200, 200, 200, ${opacity})`,
    style: {borderRadius: 8},
    propsForDots: {r: '2', strokeWidth: '1', stroke: '#4CAF50'},
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>CO₂ Accumulation</Text>
      <LineChart
        data={chartData}
        width={screenWidth - 32}
        height={200}
        chartConfig={chartConfig}
        bezier
        style={styles.chart}
      />
      <View style={styles.statsRow}>
        <Text style={styles.stat}>
          dC/dt: {fluxRate ? fluxRate.toFixed(4) : '—'} ppm/s
        </Text>
        <Text style={styles.stat}>
          R²: {rSquared ? rSquared.toFixed(4) : '—'}
        </Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#1a1a2e',
    borderRadius: 8,
    padding: 12,
    marginVertical: 8,
    marginHorizontal: 8,
  },
  title: {
    color: '#4CAF50',
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 8,
  },
  chart: {
    borderRadius: 8,
  },
  statsRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 8,
  },
  stat: {
    color: '#ccc',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  placeholder: {
    color: '#666',
    textAlign: 'center',
    padding: 20,
    fontStyle: 'italic',
  },
});

export default FluxChart;