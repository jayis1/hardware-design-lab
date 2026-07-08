/*
 * VitalCard.js — Reusable card component for a single vital sign.
 *
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function VitalCard({ label, value, unit, color, sublabel }) {
  return (
    <View style={[styles.card, { borderLeftColor: color || '#2e7d32' }]}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>
        {value}
        {unit ? <Text style={styles.unit}> {unit}</Text> : null}
      </Text>
      {sublabel ? <Text style={styles.sublabel}>{sublabel}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#f5f5f5',
    borderRadius: 8,
    padding: 12,
    marginVertical: 6,
    marginHorizontal: 4,
    flex: 1,
    borderLeftWidth: 4,
    elevation: 1,
  },
  label: {
    fontSize: 11,
    color: '#666',
    textTransform: 'uppercase',
    letterSpacing: 0.5,
  },
  value: {
    fontSize: 22,
    fontWeight: 'bold',
    color: '#1b3a1b',
    marginTop: 2,
  },
  unit: {
    fontSize: 13,
    color: '#888',
    fontWeight: 'normal',
  },
  sublabel: {
    fontSize: 11,
    color: color || '#666',
    marginTop: 2,
  },
});