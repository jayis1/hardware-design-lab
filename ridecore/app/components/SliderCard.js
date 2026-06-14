// ============================================================================
// components/SliderCard.js — Reusable slider parameter card
// ============================================================================
import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

export default function SliderCard({ label, value, min, max, step, onChange, unit = '' }) {
  // Generate discrete step positions for quick-tap adjustment
  const steps = [];
  for (let v = min; v <= max; v += step) {
    steps.push(v);
  }

  // Show at most 20 step dots
  const displaySteps = steps.length > 20 ? steps.filter((_, i) => i % Math.ceil(steps.length / 20) === 0) : steps;

  return (
    <View style={styles.card}>
      <View style={styles.header}>
        <Text style={styles.label}>{label}</Text>
        <Text style={styles.value}>{typeof value === 'number' ? value.toFixed(step < 1 ? (step < 0.01 ? 3 : 2) : 0) : value}{unit}</Text>
      </View>
      <View style={styles.track}>
        <View style={[styles.fill, { width: `${((value - min) / (max - min)) * 100}%` }]} />
      </View>
      <View style={styles.stepRow}>
        {displaySteps.map((stepVal, idx) => (
          <TouchableOpacity
            key={idx}
            style={[styles.stepDot, Math.abs(value - stepVal) < step * 0.5 && styles.stepDotActive]}
            onPress={() => onChange(stepVal)}
          />
        ))}
      </View>
      {/* Quick adjust buttons */}
      <View style={styles.adjustRow}>
        <TouchableOpacity style={styles.adjBtn} onPress={() => onChange(Math.max(min, value - step * 10))}>
          <Text style={styles.adjBtnText}>--</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.adjBtn} onPress={() => onChange(Math.max(min, value - step))}>
          <Text style={styles.adjBtnText}>-</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.adjBtn} onPress={() => onChange(Math.min(max, value + step))}>
          <Text style={styles.adjBtnText}>+</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.adjBtn} onPress={() => onChange(Math.min(max, value + step * 10))}>
          <Text style={styles.adjBtnText}>++</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#2A2A4A', borderRadius: 10, padding: 12, marginVertical: 4 },
  header: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  label: { color: '#CCC', fontSize: 13 },
  value: { color: '#FF6B35', fontSize: 13, fontWeight: '600' },
  track: { height: 6, backgroundColor: '#444', borderRadius: 3, overflow: 'hidden' },
  fill: { height: '100%', backgroundColor: '#FF6B35', borderRadius: 3 },
  stepRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 8 },
  stepDot: { width: 6, height: 6, borderRadius: 3, backgroundColor: '#555' },
  stepDotActive: { backgroundColor: '#FF6B35' },
  adjustRow: { flexDirection: 'row', justifyContent: 'space-around', marginTop: 8 },
  adjBtn: { backgroundColor: '#1A1A2E', padding: 8, borderRadius: 6, flex: 1, marginHorizontal: 4, alignItems: 'center' },
  adjBtnText: { color: '#FFF', fontWeight: '600', fontSize: 14 },
});