/**
 * GrowthTracker.js — Longitudinal root growth tracking
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useMemo } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Svg, Line, Circle, Text as SvgText, Path, Rect } from 'react-native-svg';

const Colors = {
  bg: '#0a1628', card: '#162640', accent: '#3b82f6', text: '#e2e8f0',
  textDim: '#94a3b8', success: '#10b981', warning: '#f59e0b',
};

// Sample growth data (biomass index over weeks)
const sampleData = [
  { week: 1, biomass: 0.12, moisture: 32, temp: 18 },
  { week: 2, biomass: 0.18, moisture: 30, temp: 19 },
  { week: 3, biomass: 0.25, moisture: 28, temp: 21 },
  { week: 4, biomass: 0.32, moisture: 26, temp: 22 },
  { week: 5, biomass: 0.41, moisture: 25, temp: 24 },
  { week: 6, biomass: 0.48, moisture: 27, temp: 25 },
  { week: 7, biomass: 0.55, moisture: 30, temp: 26 },
  { week: 8, biomass: 0.61, moisture: 33, temp: 26 },
  { week: 9, biomass: 0.68, moisture: 35, temp: 25 },
  { week: 10, biomass: 0.74, moisture: 38, temp: 24 },
  { week: 11, biomass: 0.78, moisture: 40, temp: 23 },
  { week: 12, biomass: 0.82, moisture: 42, temp: 22 },
];

function GrowthChart({ data, width, height, metric, color, label, unit }) {
  const padding = { top: 20, right: 20, bottom: 30, left: 40 };
  const chartW = width - padding.left - padding.right;
  const chartH = height - padding.top - padding.bottom;

  const values = data.map(d => d[metric]);
  const minVal = Math.min(...values);
  const maxVal = Math.max(...values);
  const valRange = maxVal - minVal || 1;

  const points = data.map((d, i) => ({
    x: padding.left + (i / (data.length - 1)) * chartW,
    y: padding.top + chartH - ((d[metric] - minVal) / valRange) * chartH,
    val: d[metric],
    week: d.week,
  }));

  const pathD = points.map((p, i) =>
    `${i === 0 ? 'M' : 'L'} ${p.x} ${p.y}`
  ).join(' ');

  // Y axis ticks
  const yTicks = [0, 0.25, 0.5, 0.75, 1].map(frac => ({
    y: padding.top + chartH - frac * chartH,
    val: minVal + frac * valRange,
  }));

  return (
    <View style={styles.chartCard}>
      <Text style={styles.chartTitle}>{label}</Text>
      <Svg width={width} height={height}>
        {/* Grid lines */}
        {yTicks.map((t, i) => (
          <Line
            key={i}
            x1={padding.left}
            y1={t.y}
            x2={width - padding.right}
            y2={t.y}
            stroke="#1e3a5f"
            strokeWidth="0.5"
          />
        ))}

        {/* Y axis labels */}
        {yTicks.map((t, i) => (
          <SvgText
            key={i}
            x={padding.left - 8}
            y={t.y + 4}
            fontSize="9"
            fill={Colors.textDim}
            textAnchor="end"
          >
            {t.val.toFixed(2)}
          </SvgText>
        ))}

        {/* Data line */}
        <Path d={pathD} stroke={color} strokeWidth="2" fill="none" />

        {/* Data points */}
        {points.map((p, i) => (
          <Circle
            key={i}
            cx={p.x}
            cy={p.y}
            r="3"
            fill={color}
            stroke={Colors.bg}
            strokeWidth="1"
          />
        ))}

        {/* X axis labels (weeks) */}
        {points.map((p, i) => (
          i % 2 === 0 ? (
            <SvgText
              key={i}
              x={p.x}
              y={height - 8}
              fontSize="8"
              fill={Colors.textDim}
              textAnchor="middle"
            >
              W{p.week}
            </SvgText>
          ) : null
        ))}

        {/* Unit label */}
        <SvgText
          x={padding.left}
          y={padding.top - 6}
          fontSize="9"
          fill={Colors.textDim}
        >
          {unit}
        </SvgText>
      </Svg>
    </View>
  );
}

export function GrowthTracker({ navigation }) {
  const [selectedMetric, setSelectedMetric] = useState('biomass');

  const metrics = [
    { key: 'biomass', label: 'Root Biomass Index', unit: '(relative)', color: '#10b981' },
    { key: 'moisture', label: 'Soil Moisture', unit: '%', color: '#3b82f6' },
    { key: 'temp', label: 'Soil Temperature', unit: '°C', color: '#f59e0b' },
  ];

  const currentMetric = metrics.find(m => m.key === selectedMetric);

  // Compute growth rate
  const growthRate = useMemo(() => {
    if (sampleData.length < 2) return 0;
    const first = sampleData[0].biomass;
    const last = sampleData[sampleData.length - 1].biomass;
    const weeks = sampleData.length;
    return ((last - first) / weeks * 100).toFixed(1);
  }, []);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Growth Tracking</Text>
      <Text style={styles.subtitle}>Longitudinal root biomass development</Text>

      {/* Summary stats */}
      <View style={styles.statsRow}>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Growth Rate</Text>
          <Text style={styles.statValue}>{growthRate}%/wk</Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Current</Text>
          <Text style={styles.statValue}>
            {sampleData[sampleData.length - 1].biomass.toFixed(2)}
          </Text>
        </View>
        <View style={styles.statCard}>
          <Text style={styles.statLabel}>Duration</Text>
          <Text style={styles.statValue}>{sampleData.length}w</Text>
        </View>
      </View>

      {/* Metric selector */}
      <View style={styles.metricRow}>
        {metrics.map(m => (
          <TouchableOpacity
            key={m.key}
            style={[styles.metricBtn, selectedMetric === m.key && styles.metricBtnActive]}
            onPress={() => setSelectedMetric(m.key)}>
            <Text
              style={[styles.metricBtnText,
                selectedMetric === m.key && styles.metricBtnTextActive]}>
              {m.label.split(' ')[0]}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Chart */}
      <GrowthChart
        data={sampleData}
        width={340}
        height={200}
        metric={currentMetric.key}
        color={currentMetric.color}
        label={currentMetric.label}
        unit={currentMetric.unit}
      />

      {/* Interventions log */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Interventions</Text>
        <View style={styles.intervention}>
          <Text style={styles.intDate}>Week 4</Text>
          <Text style={styles.intText}>Irrigation: 20mm drip</Text>
        </View>
        <View style={styles.intervention}>
          <Text style={styles.intDate}>Week 6</Text>
          <Text style={styles.intText}>Fertilizer: NPK 15-15-15</Text>
        </View>
        <View style={styles.intervention}>
          <Text style={styles.intDate}>Week 9</Text>
          <Text style={styles.intText}>Pruning: 30% canopy</Text>
        </View>
      </View>

      <Text style={styles.footer}>© 2026 jayis1 · MIT</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.bg, padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: Colors.text },
  subtitle: { fontSize: 14, color: Colors.textDim, marginBottom: 16 },
  statsRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 12 },
  statCard: { backgroundColor: Colors.card, borderRadius: 10, padding: 12, flex: 1, marginHorizontal: 4, alignItems: 'center' },
  statLabel: { fontSize: 10, color: Colors.textDim },
  statValue: { fontSize: 18, color: Colors.text, fontWeight: 'bold', marginTop: 4 },
  metricRow: { flexDirection: 'row', marginBottom: 12 },
  metricBtn: { flex: 1, backgroundColor: Colors.card, padding: 10, borderRadius: 8, marginHorizontal: 4, alignItems: 'center' },
  metricBtnActive: { backgroundColor: Colors.accent },
  metricBtnText: { fontSize: 12, color: Colors.textDim },
  metricBtnTextActive: { color: Colors.text, fontWeight: 'bold' },
  chartCard: { backgroundColor: Colors.card, borderRadius: 12, padding: 12, marginBottom: 12 },
  chartTitle: { fontSize: 14, color: Colors.text, fontWeight: '600', marginBottom: 8 },
  card: { backgroundColor: Colors.card, borderRadius: 12, padding: 16, marginBottom: 12 },
  cardTitle: { fontSize: 16, color: Colors.text, fontWeight: 'bold', marginBottom: 12 },
  intervention: { flexDirection: 'row', paddingVertical: 6, borderBottomWidth: 0.5, borderBottomColor: '#1e3a5f' },
  intDate: { fontSize: 12, color: Colors.warning, fontWeight: '600', width: 60 },
  intText: { fontSize: 13, color: Colors.text, flex: 1 },
  footer: { fontSize: 11, color: Colors.textDim, textAlign: 'center', paddingVertical: 16 },
});