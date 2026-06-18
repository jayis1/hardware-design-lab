/**
 * WaveformChart.js — Scrolling Waveform Display
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * A lightweight canvas-free waveform renderer using React Native Views.
 * Displays the last N samples of plant action potential data with
 * threshold lines.
 */

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';

export default function WaveformChart({ data, color, threshold, height, width }) {
  const w = width || Dimensions.get('window').width - 64;
  const h = height || 150;

  if (!data || data.length === 0) {
    return (
      <View style={[styles.emptyContainer, { width: w, height: h }]}>
        <Text style={styles.emptyText}>Waiting for data...</Text>
      </View>
    );
  }

  // Find min/max for auto-scaling
  const maxVal = Math.max(...data.map(Math.abs), threshold || 50);
  const scale = (h / 2) / (maxVal * 1.2);
  const midY = h / 2;
  const stepX = w / Math.max(data.length - 1, 1);

  // Build SVG-like path points
  const points = data.map((val, i) => {
    const x = i * stepX;
    const y = midY - val * scale;
    return { x, y: Math.max(0, Math.min(h, y)) };
  });

  // Create path string for a polyline
  const pathD = points.map((p, i) =>
    `${i === 0 ? 'M' : 'L'} ${p.x.toFixed(1)} ${p.y.toFixed(1)}`
  ).join(' ');

  // Threshold lines
  const threshY1 = midY - threshold * scale;
  const threshY2 = midY + threshold * scale;

  return (
    <View style={[styles.container, { width: w, height: h }]}>
      {/* Y-axis labels */}
      <View style={styles.yAxis}>
        <Text style={styles.yLabel}>{Math.round(maxVal)}µV</Text>
        <Text style={styles.yLabel}>0</Text>
        <Text style={styles.yLabel}>−{Math.round(maxVal)}µV</Text>
      </View>

      {/* Plot area */}
      <View style={[styles.plotArea, { width: w - 30, height: h }]}>
        {/* Threshold lines */}
        {threshold && (
          <>
            <View style={[styles.thresholdLine, { top: threshY1, width: w - 30 }]} />
            <View style={[styles.thresholdLine, { top: threshY2, width: w - 30 }]} />
          </>
        )}

        {/* Center line */}
        <View style={[styles.centerLine, { top: midY, width: w - 30 }]} />

        {/* Waveform — rendered as positioned bars */}
        <View style={styles.waveform}>
          {points.map((p, i) => (
            <View
              key={i}
              style={{
                position: 'absolute',
                left: p.x,
                top: Math.min(p.y, midY),
                width: Math.max(stepX, 1),
                height: Math.abs(p.y - midY),
                backgroundColor: color,
              }}
            />
          ))}
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    backgroundColor: '#0d1b0d',
    borderRadius: 8,
    overflow: 'hidden',
  },
  yAxis: {
    width: 30,
    justifyContent: 'space-between',
    paddingVertical: 4,
    paddingRight: 4,
  },
  yLabel: {
    fontSize: 8,
    color: '#555',
    textAlign: 'right',
  },
  plotArea: {
    position: 'relative',
    backgroundColor: '#0a150a',
  },
  thresholdLine: {
    position: 'absolute',
    height: 1,
    backgroundColor: '#ff9800',
    opacity: 0.4,
  },
  centerLine: {
    position: 'absolute',
    height: 1,
    backgroundColor: '#333',
  },
  waveform: {
    position: 'absolute',
    width: '100%',
    height: '100%',
  },
  emptyContainer: {
    backgroundColor: '#0a150a',
    borderRadius: 8,
    alignItems: 'center',
    justifyContent: 'center',
  },
  emptyText: {
    color: '#555',
    fontSize: 14,
  },
});