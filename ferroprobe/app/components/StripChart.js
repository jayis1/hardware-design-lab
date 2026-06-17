/**
 * StripChart.js — Time-Series Strip Chart
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Renders a scrolling time-series chart of magnetic field data
 * using pure React Native Views (no chart library dependency
 * for the core rendering, uses a simple SVG-like approach with
 * positioned dots).
 */

import React, { useMemo } from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function StripChart({ data, color = '#2196F3', height = 120 }) {
  // Compute min/max for auto-scaling
  const { min, max, range } = useMemo(() => {
    if (data.length === 0) return { min: 40000, max: 60000, range: 20000 };
    const values = data.map((d) => d.y);
    const min = Math.min(...values);
    const max = Math.max(...values);
    const padding = (max - min) * 0.1 || 1000;
    return {
      min: min - padding,
      max: max + padding,
      range: (max - min) + 2 * padding,
    };
  }, [data]);

  // Render data points as positioned dots
  const points = useMemo(() => {
    if (data.length === 0) return [];
    return data.map((d, i) => {
      const x = (i / Math.max(data.length - 1, 1)) * 100; // 0-100%
      const y = ((max - d.y) / range) * 100; // 0-100% (inverted: high values at top)
      return { x, y };
    });
  }, [data, min, max, range]);

  // Build an SVG-like polyline using a series of thin Views
  // For performance, we limit to 100 visible points
  const visiblePoints = points.slice(-100);

  // Average value for display
  const avg = data.length > 0
    ? data.reduce((s, d) => s + d.y, 0) / data.length
    : 0;

  return (
    <View style={[styles.container, { height }]}>
      {/* Grid lines */}
      <View style={[styles.gridLine, { top: '25%' }]} />
      <View style={[styles.gridLine, { top: '50%' }]} />
      <View style={[styles.gridLine, { top: '75%' }]} />

      {/* Y-axis labels */}
      <Text style={[styles.yLabel, { top: '5%' }]}>
        {(max / 1000).toFixed(1)} µT
      </Text>
      <Text style={[styles.yLabel, { top: '50%' }]}>
        {((min + max) / 2000).toFixed(1)} µT
      </Text>
      <Text style={[styles.yLabel, { bottom: '5%' }]}>
        {(min / 1000).toFixed(1)} µT
      </Text>

      {/* Data points */}
      <View style={styles.plotArea}>
        {visiblePoints.map((p, i) => {
          const next = visiblePoints[i + 1];
          if (!next) return null;
          // Draw a line segment from p to next
          const dx = next.x - p.x;
          const dy = next.y - p.y;
          const length = Math.sqrt(dx * dx + dy * dy) * 2; // Scale for display
          const angle = Math.atan2(dy, dx) * 180 / Math.PI;
          return (
            <View
              key={i}
              style={[
                styles.lineSegment,
                {
                  left: `${p.x}%`,
                  top: `${p.y}%`,
                  width: `${Math.max(dx * 1.5, 1)}%`,
                  transform: [{ rotate: `${angle}deg` }],
                  backgroundColor: color,
                },
              ]}
            />
          );
        })}
      </View>

      {/* Current value badge */}
      {data.length > 0 && (
        <View style={styles.currentValue}>
          <Text style={styles.currentValueText}>
            {(data[data.length - 1].y / 1000).toFixed(2)} µT
          </Text>
        </View>
      )}

      {/* Average line */}
      {data.length > 0 && (
        <View
          style={[
            styles.avgLine,
            {
              top: `${((max - avg) / range) * 100}%`,
            },
          ]}
        >
          <Text style={styles.avgLabel}>avg</Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
    position: 'relative',
    overflow: 'hidden',
  },
  plotArea: {
    position: 'absolute',
    left: '8%',
    right: '2%',
    top: 0,
    bottom: 0,
  },
  gridLine: {
    position: 'absolute',
    left: '8%',
    right: '2%',
    height: 1,
    backgroundColor: '#1a1a2e',
  },
  yLabel: {
    position: 'absolute',
    left: 4,
    color: '#666',
    fontSize: 9,
    fontFamily: 'monospace',
  },
  lineSegment: {
    position: 'absolute',
    height: 1.5,
    transformOrigin: 'left center',
  },
  currentValue: {
    position: 'absolute',
    right: 8,
    top: 8,
    backgroundColor: 'rgba(33,150,243,0.3)',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 4,
  },
  currentValueText: {
    color: '#2196F3',
    fontSize: 12,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  avgLine: {
    position: 'absolute',
    left: '8%',
    right: '2%',
    height: 1,
    backgroundColor: 'rgba(255,235,59,0.3)',
    borderStyle: 'dashed',
  },
  avgLabel: {
    position: 'absolute',
    right: 0,
    top: -10,
    color: 'rgba(255,235,59,0.5)',
    fontSize: 8,
  },
});