// src/components/SpectrumChart.tsx — 16-band spectrum bar chart
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface SpectrumChartProps {
  bands: number[];        // reflectance values (0–1.0)
  wavelengths: number[];  // wavelength labels in nm
}

// Color mapping: visible spectrum approximation
function wavelengthToColor(nm: number): string {
  if (nm < 470) return '#7e57c2';  // blue/violet
  if (nm < 500) return '#5c6bc0';  // blue
  if (nm < 550) return '#26a69a';  // green
  if (nm < 580) return '#9ccc65';  // yellow-green
  if (nm < 620) return '#ffca28';  // yellow
  if (nm < 680) return '#ff7043';  // orange/red
  if (nm < 720) return '#ef5350';  // red
  if (nm < 800) return '#ec407a';  // red-edge
  return '#ab47bc';  // NIR
}

export default function SpectrumChart({ bands, wavelengths }: SpectrumChartProps) {
  const maxVal = Math.max(0.6, ...bands);

  return (
    <View style={styles.container}>
      {/* Y-axis labels */}
      <View style={styles.chartArea}>
        <View style={styles.yAxis}>
          <Text style={styles.yLabel}>{maxVal.toFixed(2)}</Text>
          <Text style={styles.yLabel}>{(maxVal * 0.5).toFixed(2)}</Text>
          <Text style={styles.yLabel}>0.00</Text>
        </View>

        {/* Bars */}
        <View style={styles.barsContainer}>
          {bands.map((val, i) => {
            const heightPct = (val / maxVal) * 100;
            const color = wavelengthToColor(wavelengths[i]);
            return (
              <View key={i} style={styles.barColumn}>
                <View style={styles.barTrack}>
                  <View
                    style={[
                      styles.barFill,
                      {
                        height: `${Math.min(100, heightPct)}%`,
                        backgroundColor: color,
                      },
                    ]}
                  />
                </View>
                <Text style={styles.xLabel}>{wavelengths[i]}</Text>
              </View>
            );
          })}
        </View>
      </View>

      {/* Axis title */}
      <Text style={styles.axisTitle}>Reflectance vs Wavelength (nm)</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { paddingVertical: 8 },
  chartArea: { flexDirection: 'row', height: 140 },
  yAxis: { width: 35, justifyContent: 'space-between', paddingVertical: 2 },
  yLabel: { color: '#81c784', fontSize: 9, textAlign: 'right' },
  barsContainer: { flex: 1, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'flex-end', paddingHorizontal: 4 },
  barColumn: { flex: 1, alignItems: 'center', marginHorizontal: 1 },
  barTrack: { width: '100%', height: 110, justifyContent: 'flex-end', backgroundColor: '#0a1f0a', borderRadius: 2, overflow: 'hidden' },
  barFill: { width: '100%', borderRadius: 2 },
  xLabel: { color: '#b0bec5', fontSize: 7, marginTop: 2 },
  axisTitle: { color: '#81c784', fontSize: 10, textAlign: 'center', marginTop: 4 },
});