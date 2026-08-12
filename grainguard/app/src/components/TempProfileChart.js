/**
 * @file    TempProfileChart.js
 * @brief   Bar chart showing the 9-zone temperature profile along the probe.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function TempProfileChart({ data, width = 320, height = 180 }) {
  // data: array of 9 { zone, tempC, valid }
  const zones = data || Array.from({ length: 9 }, (_, i) => ({
    zone: i, tempC: 0, valid: false,
  }));

  const temps = zones.filter(z => z.valid).map(z => z.tempC);
  const minT = temps.length ? Math.min(...temps) : 0;
  const maxT = temps.length ? Math.max(...temps) : 100;
  const range = maxT - minT || 1;
  const barAreaH = height - 30;  // leave room for labels
  const barWidth = (width - 40) / 9 - 4;

  // Color: blue (cold) -> green -> red (hot)
  const tempColor = (t) => {
    const norm = (t - minT) / range;  // 0..1
    if (norm < 0.5) {
      // blue -> green
      const f = norm * 2;
      const r = Math.round(66 + f * (76 - 66));
      const g = Math.round(165 + f * (175 - 165));
      const b = Math.round(245 + f * (80 - 245));
      return `rgb(${r},${g},${b})`;
    } else {
      // green -> red
      const f = (norm - 0.5) * 2;
      const r = Math.round(76 + f * (211 - 76));
      const g = Math.round(175 + f * (47 - 175));
      const b = Math.round(80 + f * (47 - 80));
      return `rgb(${r},${g},${b})`;
    }
  };

  return (
    <View style={[styles.container, { width, height }]}>
      <View style={styles.barsRow}>
        {zones.map((z, i) => {
          const h = z.valid ? Math.max(8, ((z.tempC - minT) / range) * barAreaH) : 4;
          return (
            <View key={i} style={styles.barCol}>
              <Text style={styles.tempLabel}>
                {z.valid ? z.tempC.toFixed(1) : '--'}
              </Text>
              <View style={[styles.bar, {
                width: barWidth, height: h,
                backgroundColor: z.valid ? tempColor(z.tempC) : '#E0E0E0',
              }]} />
              <Text style={styles.zoneLabel}>{i + 1}</Text>
            </View>
          );
        })}
      </View>
      <Text style={styles.axisLabel}>Zone (1 = top, 9 = bottom) · °C</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flexDirection: 'column' },
  barsRow: { flexDirection: 'row', alignItems: 'flex-end', flex: 1, paddingHorizontal: 20 },
  barCol: { alignItems: 'center', marginRight: 4, flex: 1 },
  tempLabel: { fontSize: 8, color: '#616161', marginBottom: 2 },
  bar: { borderRadius: 3 },
  zoneLabel: { fontSize: 10, color: '#9E9E9E', marginTop: 2 },
  axisLabel: { fontSize: 9, color: '#BDBDBD', textAlign: 'center', marginTop: 4 },
});