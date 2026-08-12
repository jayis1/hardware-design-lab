/**
 * @file    SRIgauge.js
 * @brief   Circular gauge component for the Spoilage Risk Index (0-100).
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function SRIgauge({ value, size = 48 }) {
  const v = Math.max(0, Math.min(100, value || 0));
  const color = v >= 70 ? '#D32F2F' : v >= 40 ? '#F9A825' : '#388E3C';
  const label = v >= 70 ? 'CRIT' : v >= 40 ? 'CAUT' : 'OK';

  const fontSize = Math.floor(size * 0.32);
  const labelSize = Math.floor(size * 0.18);

  return (
    <View style={[
      styles.gauge,
      {
        width: size, height: size, borderRadius: size / 2,
        backgroundColor: color, borderColor: color,
      },
    ]}>
      <Text style={[styles.value, { fontSize, color: '#fff' }]}>{Math.round(v)}</Text>
      <Text style={[styles.label, { fontSize: labelSize, color: '#fff' }]}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  gauge: {
    alignItems: 'center', justifyContent: 'center',
    borderWidth: 3,
    shadowColor: '#000', shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.2, shadowRadius: 2, elevation: 3,
  },
  value: { fontWeight: 'bold' },
  label: { fontWeight: '600', marginTop: -2 },
});