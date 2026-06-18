/**
 * SapFlowChart.js — Sap Flow Temperature Response Chart
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Displays the temperature response curve after a heat pulse,
 * showing the rise, peak (t_max), and decay phases.
 */

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';

export default function SapFlowChart({ measuring, deltaT, tmax, height, width }) {
  const w = width || Dimensions.get('window').width - 32;
  const h = height || 180;

  // Generate a synthetic temperature curve for visualization
  // Real data would come from the device
  const points = [];
  const totalSeconds = 60;
  const numPoints = 60;

  for (let i = 0; i < numPoints; i++) {
    const t = i;
    let temp;

    if (t < 4) {
      // Pre-pulse baseline
      temp = 0;
    } else if (t < tmax + 4) {
      // Rising phase (exponential rise to peak)
      const riseT = t - 4;
      const peakT = tmax || 15;
      temp = deltaT * (1 - Math.exp(-riseT / (peakT / 3)));
    } else {
      // Decay phase (exponential cooling)
      const decayT = t - tmax - 4;
      temp = deltaT * Math.exp(-decayT / 20);
    }

    points.push({ x: t, y: temp });
  }

  // Scale for chart
  const maxTemp = Math.max(deltaT * 1.5, 2);
  const scaleY = (h - 30) / maxTemp;
  const scaleX = (w - 40) / totalSeconds;
  const midY = h - 20;

  return (
    <View style={[styles.container, { width: w, height: h }]}>
      <View style={styles.chart}>
        {/* Grid lines */}
        {[0, 0.25, 0.5, 0.75, 1.0].map((frac) => (
          <View
            key={frac}
            style={[styles.gridLine, { top: (h - 30) * frac + 10, width: w - 40 }]}
          />
        ))}

        {/* Temperature curve */}
        <View style={styles.curve}>
          {points.map((p, i) => (
            <View
              key={i}
              style={{
                position: 'absolute',
                left: 20 + p.x * scaleX,
                top: midY - p.y * scaleY,
                width: 2,
                height: 2,
                backgroundColor: '#4CAF50',
                borderRadius: 1,
              }}
            />
          ))}
        </View>

        {/* Heat pulse marker */}
        <View style={[styles.pulseMarker, { left: 20 + 4 * scaleX, height: h - 30 }]} />
        <Text style={[styles.pulseLabel, { left: 20 + 4 * scaleX }]}>Pulse</Text>

        {/* t_max marker */}
        {tmax > 0 && (
          <>
            <View style={[styles.tmaxMarker, { left: 20 + (tmax + 4) * scaleX, height: h - 30 }]} />
            <Text style={[styles.tmaxLabel, { left: 20 + (tmax + 4) * scaleX }]}>
              t_max
            </Text>
          </>
        )}

        {/* Status overlay */}
        {measuring && (
          <View style={styles.measuringOverlay}>
            <Text style={styles.measuringText}>⏳ Measuring...</Text>
          </View>
        )}
      </View>

      {/* X-axis labels */}
      <View style={styles.xAxis}>
        <Text style={styles.xLabel}>0s</Text>
        <Text style={styles.xLabel}>15s</Text>
        <Text style={styles.xLabel}>30s</Text>
        <Text style={styles.xLabel}>45s</Text>
        <Text style={styles.xLabel}>60s</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#0a150a',
    borderRadius: 8,
    padding: 4,
  },
  chart: {
    position: 'relative',
    width: '100%',
    height: '100%',
  },
  gridLine: {
    position: 'absolute',
    height: 1,
    backgroundColor: '#1a2e1a',
    left: 20,
  },
  curve: {
    position: 'absolute',
    width: '100%',
    height: '100%',
  },
  pulseMarker: {
    position: 'absolute',
    top: 10,
    width: 2,
    backgroundColor: '#ff6b35',
  },
  pulseLabel: {
    position: 'absolute',
    top: 0,
    fontSize: 9,
    color: '#ff6b35',
    transform: [{ translateX: -12 }],
  },
  tmaxMarker: {
    position: 'absolute',
    top: 10,
    width: 2,
    backgroundColor: '#2196F3',
  },
  tmaxLabel: {
    position: 'absolute',
    top: 0,
    fontSize: 9,
    color: '#2196F3',
    transform: [{ translateX: -16 }],
  },
  measuringOverlay: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    backgroundColor: 'rgba(0,0,0,0.5)',
    alignItems: 'center',
    justifyContent: 'center',
    borderRadius: 8,
  },
  measuringText: {
    color: '#ff9800',
    fontSize: 16,
    fontWeight: 'bold',
  },
  xAxis: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 20,
    marginTop: 4,
  },
  xLabel: {
    fontSize: 9,
    color: '#555',
  },
});