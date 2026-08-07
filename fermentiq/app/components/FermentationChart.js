/**
 * FermentationChart.js — Time-Series Chart Component
 *
 * Reusable line chart component for displaying fermentation sensor
 * trends over time. Wraps react-native-chart-kit's LineChart with
 * FermenTiq-specific styling and defaults.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import { LineChart } from 'react-native-chart-kit';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const SCREEN_WIDTH = Dimensions.get('window').width;

export default function FermentationChart({
  title,
  icon,
  iconColor = '#D4A017',
  data,
  unit = '',
  height = 180,
  yMin,
  yMax,
}) {
  if (!data || !data.datasets || data.datasets.length === 0) {
    return (
      <View style={styles.empty}>
        <Icon name="chart-line-variant" size={32} color="#444" />
        <Text style={styles.emptyText}>No data</Text>
      </View>
    );
  }

  const chartConfig = {
    backgroundColor: '#16213e',
    backgroundGradientFrom: '#16213e',
    backgroundGradientTo: '#16213e',
    color: (opacity = 1) => `rgba(224, 224, 224, ${opacity * 0.3})`,
    labelColor: (opacity = 1) => `rgba(160, 160, 160, ${opacity})`,
    strokeWidth: 2,
    barRadius: 1,
    propsForDots: { r: 2, strokeWidth: 1 },
    decimalPlaces: 2,
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Icon name={icon} size={18} color={iconColor} />
        <Text style={styles.title}>{title}</Text>
        {unit ? <Text style={styles.unit}>({unit})</Text> : null}
      </View>
      <LineChart
        data={data}
        width={SCREEN_WIDTH - 50}
        height={height}
        chartConfig={chartConfig}
        bezier
        style={styles.chart}
        yAxisLabel=""
        yAxisSuffix={unit ? ` ${unit}` : ""}
        segments={4}
        yMin={yMin}
        yMax={yMax}
        withVerticalLabels={true}
        withHorizontalLabels={true}
        withShadow={false}
        withInnerLines={true}
        withOuterLines={false}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { marginVertical: 6 },
  header: { flexDirection: 'row', alignItems: 'center', marginBottom: 6 },
  title: { color: '#e0e0e0', fontSize: 14, marginLeft: 6, fontWeight: '600' },
  unit: { color: '#888', fontSize: 12, marginLeft: 4 },
  chart: { borderRadius: 8, marginVertical: 4 },
  empty: { alignItems: 'center', padding: 20, height: 100, justifyContent: 'center' },
  emptyText: { color: '#555', fontSize: 14, marginTop: 8 },
});