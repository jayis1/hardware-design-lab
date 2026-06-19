// AscanPlot.js — A-Scan Waveform Plot Component
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Renders a single A-scan (reflectivity vs. depth) as a waveform plot.
// Shows amplitude on the horizontal axis and depth on the vertical axis,
// with optional peak markers for detected interfaces.
//

import React, { useMemo } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import { detectPeaks } from '../utils/gprMath';

const { width } = Dimensions.get('window');

export default function AscanPlot({ trace, depths, threshold = 0.3, epsR = 6 }) {
  const plotWidth = width - 60;
  const plotHeight = 400;

  const peaks = useMemo(() => {
    if (!trace || trace.length === 0) return [];
    return detectPeaks(trace, threshold).slice(0, 5);
  }, [trace, threshold]);

  if (!trace || trace.length === 0) {
    return (
      <View style={styles.emptyContainer}>
        <Text style={styles.emptyText}>No A-scan selected.</Text>
        <Text style={styles.hint}>Tap a column in the Radargram to inspect.</Text>
      </View>
    );
  }

  const maxVal = Math.max(...trace.map(v => Math.abs(v)), 0.001);
  const center = plotWidth / 2;

  // Build waveform path points
  const points = trace.map((v, i) => {
    const x = center + (v / maxVal) * (center - 5);
    const y = (i / trace.length) * plotHeight;
    return { x, y, v };
  });

  // Create SVG-like polyline using nested Views
  return (
    <View style={styles.container}>
      <Text style={styles.title}>A-Scan Profile</Text>
      <View style={[styles.plot, { width: plotWidth, height: plotHeight }]}>
        {/* Center line */}
        <View style={[styles.centerLine, { left: center, height: plotHeight }]} />
        {/* Waveform */}
        {points.map((p, i) => {
          if (i === 0) return null;
          const prev = points[i - 1];
          const dy = p.y - prev.y;
          const dx = p.x - prev.x;
          const len = Math.sqrt(dx * dx + dy * dy);
          const angle = Math.atan2(dy, dx) * 180 / Math.PI;
          return (
            <View
              key={`s${i}`}
              style={{
                position: 'absolute',
                left: prev.x,
                top: prev.y,
                width: len,
                height: 1.5,
                backgroundColor: '#00ff88',
                transform: [{ rotate: `${angle}deg` }],
                transformOrigin: '0 0',
              }}
            />
          );
        })}
        {/* Peak markers */}
        {peaks.map((pk, i) => {
          const y = (pk.index / trace.length) * plotHeight;
          const depth = depths ? depths[pk.index] : pk.index * 0.1;
          return (
            <View key={`pk${i}`} style={[styles.peakMarker, { top: y }]}>
              <View style={styles.peakDot} />
              <Text style={styles.peakLabel}>
                {depth ? depth.toFixed(2) : pk.index}m
              </Text>
            </View>
          );
        })}
      </View>
      {/* Depth axis */}
      <View style={styles.depthLabels}>
        {depths && depths.length > 0 && [0, 0.25, 0.5, 0.75, 1.0].map((frac, i) => {
          const idx = Math.floor(frac * (depths.length - 1));
          return (
            <Text key={`d${i}`} style={styles.depthText}>
              {depths[idx].toFixed(2)} m
            </Text>
          );
        })}
      </View>
      <Text style={styles.info}>
        Peaks detected: {peaks.length} | Max amplitude: {maxVal.toFixed(3)} V
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
    padding: 15,
  },
  title: {
    color: '#0ff',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 10,
  },
  plot: {
    backgroundColor: '#000',
    borderWidth: 1,
    borderColor: '#333',
    position: 'relative',
    overflow: 'hidden',
  },
  centerLine: {
    position: 'absolute',
    width: 1,
    backgroundColor: '#333',
    top: 0,
  },
  peakMarker: {
    position: 'absolute',
    left: 5,
    flexDirection: 'row',
    alignItems: 'center',
  },
  peakDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
    backgroundColor: '#ff0',
  },
  peakLabel: {
    color: '#ff0',
    fontSize: 10,
    marginLeft: 4,
    fontFamily: 'monospace',
  },
  depthLabels: {
    position: 'absolute',
    right: 15,
    top: 40,
    height: 400,
    justifyContent: 'space-between',
  },
  depthText: {
    color: '#0ff',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  info: {
    color: '#888',
    fontSize: 12,
    marginTop: 10,
    fontFamily: 'monospace',
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#0a0a1a',
  },
  emptyText: {
    color: '#888',
    fontSize: 16,
  },
  hint: {
    color: '#555',
    fontSize: 12,
    marginTop: 8,
  },
});