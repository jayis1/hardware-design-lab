/**
 * MetricCard.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function MetricCard({ label, value, unit, accent = '#58d68d', subtitle = '' }) {
  return (
    <View style={[styles.card, { borderColor: accent }]}> 
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, { color: accent }]}>{value}<Text style={styles.unit}> {unit}</Text></Text>
      {subtitle ? <Text style={styles.subtitle}>{subtitle}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#111827',
    borderWidth: 1,
    borderRadius: 16,
    padding: 14,
    marginBottom: 12,
    width: '48%'
  },
  label: {
    color: '#9ca3af',
    fontSize: 12,
    marginBottom: 8,
    textTransform: 'uppercase',
    letterSpacing: 1.1
  },
  value: {
    fontSize: 26,
    fontWeight: '700'
  },
  unit: {
    color: '#d1d5db',
    fontSize: 13
  },
  subtitle: {
    color: '#cbd5e1',
    marginTop: 6,
    fontSize: 12
  }
});
