/*
 * FingerprintScreen.tsx — visualise the optical + EIS fingerprint
 * Author: jayis1
 */
import React, { useState } from 'react';
import { View, Text, ScrollView, StyleSheet } from 'react-native';

// Demo fingerprint data (a real reading arrives over BLE).
const WAVELENGTHS = [255, 280, 365, 470, 590, 660, 850, 940];
const DEMO_OPTICAL = [0.02, 0.05, 0.12, 0.08, 0.21, 0.14, 0.32, 0.41];
const DEMO_EIS_RE  = [3200, 3100, 2900, 2600, 2200, 1700, 1200, 800, 500, 350,
                      300, 280, 260, 250, 245, 240, 238, 236, 235, 234];
const DEMO_EIS_IM  = [120, 180, 240, 300, 360, 420, 460, 480, 470, 440,
                      400, 360, 320, 280, 250, 220, 200, 180, 165, 150];

export default function FingerprintScreen() {
  const [opt] = useState(DEMO_OPTICAL);
  const [re]  = useState(DEMO_EIS_RE);
  const [im]  = useState(DEMO_EIS_IM);

  const maxOpt = Math.max(...opt, 0.01);
  const maxEis = Math.max(...re, ...im, 1);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Fingerprint</Text>

      <Text style={styles.subtitle}>Optical absorbance (8 wavelengths)</Text>
      <View style={styles.bars}>
        {opt.map((v, i) => (
          <View key={i} style={styles.barCol}>
            <View style={[styles.bar, { height: (v / maxOpt) * 120 }]} />
            <Text style={styles.barLabel}>{WAVELENGTHS[i]}</Text>
          </View>
        ))}
      </View>

      <Text style={styles.subtitle}>Impedance Nyquist plot</Text>
      <View style={styles.plot}>
        {re.map((r, i) => {
          const x = (r / maxEis) * 240;
          const y = 120 - (im[i] / maxEis) * 120;
          return <View key={i} style={[styles.point, { left: x, top: y }]} />;
        })}
      </View>
      <Text style={styles.axis}>← Z' (real)   | -Z'' (imag) ↑</Text>

      <Text style={styles.footer}>
        Author: jayis1 — live data arrives from the device over BLE when a
        measurement completes. This screen shows a representative
        reading for illustration.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#fafafa' },
  title:     { fontSize: 24, fontWeight: '700', marginTop: 12, color: '#1e88e5' },
  subtitle:  { fontSize: 14, fontWeight: '600', marginTop: 20, marginBottom: 8, color: '#444' },
  bars:      { flexDirection: 'row', height: 160, alignItems: 'flex-end' },
  barCol:    { flex: 1, alignItems: 'center' },
  bar:       { width: 18, backgroundColor: '#1e88e5', marginBottom: 4, borderRadius: 3 },
  barLabel:  { fontSize: 9, color: '#666' },
  plot:      { height: 140, backgroundColor: '#fff', borderWidth: 1, borderColor: '#ddd', position: 'relative' },
  point:     { position: 'absolute', width: 6, height: 6, borderRadius: 3, backgroundColor: '#d32f2f' },
  axis:      { fontSize: 11, color: '#777', marginTop: 4 },
  footer:    { fontSize: 12, color: '#888', marginTop: 24, lineHeight: 18 },
});