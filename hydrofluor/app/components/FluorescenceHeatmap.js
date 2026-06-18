// components/FluorescenceHeatmap.js — 6×4 excitation×emission heatmap
// Author: jayis1  License: MIT
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { theme } from '../utils/theme';

// Props: data = number[6][4] (raw fluorescence net uV, scaled to 0..1)
export default function FluorescenceHeatmap({ data }) {
  const ex = ['255', '280', '365', '470', '590', '660'];
  const det = ['360', '430', '650', '685'];
  return (
    <View style={styles.wrap}>
      <Text style={styles.title}>Fluorescence Matrix (excitation × emission)</Text>
      <View style={styles.row}>
        <View style={styles.corner} />
        {det.map(d => <Text key={d} style={styles.hdr}>{d}nm</Text>)}
      </View>
      {ex.map((e, i) => (
        <View key={e} style={styles.row}>
          <Text style={styles.hdr}>{e}nm</Text>
          {det.map((d, j) => {
            const v = data ? data[i][j] : 0;
            const c = lerpColor(v);
            return (
              <View key={d} style={[styles.cell, { backgroundColor: c }]}>
                <Text style={styles.cellText}>{Math.round((v || 0) * 100)}</Text>
              </View>
            );
          })}
        </View>
      ))}
    </View>
  );
}

function lerpColor(t) {
  t = Math.max(0, Math.min(1, t));
  // interpolate from fluorLow → fluorHigh
  const a = hexToRgb(theme.colors.fluorLow);
  const b = hexToRgb(theme.colors.fluorHigh);
  const r = Math.round(a.r + (b.r - a.r) * t);
  const g = Math.round(a.g + (b.g - a.g) * t);
  const bb = Math.round(a.b + (b.b - a.b) * t);
  return `rgb(${r},${g},${bb})`;
}
function hexToRgb(h) {
  const n = parseInt(h.slice(1), 16);
  return { r: (n >> 16) & 255, g: (n >> 8) & 255, b: n & 255 };
}

const styles = StyleSheet.create({
  wrap: { marginTop: theme.spacing.md, padding: theme.spacing.sm, backgroundColor: theme.colors.surface, borderRadius: theme.radius.md },
  title: { color: theme.colors.text, fontSize: 13, fontWeight: '600', marginBottom: 6, fontFamily: theme.fonts.mono },
  row: { flexDirection: 'row' },
  corner: { width: 48, height: 20 },
  hdr: { width: 56, color: theme.colors.textDim, fontSize: 11, textAlign: 'center', fontFamily: theme.fonts.mono },
  cell: { width: 56, height: 36, alignItems: 'center', justifyContent: 'center', margin: 1, borderRadius: 3 },
  cellText: { color: '#fff', fontSize: 11, fontFamily: theme.fonts.mono },
});