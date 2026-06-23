/**
 * EnviroCard.js — Environmental sensor data card for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Displays a single environmental sensor reading (moisture, temperature,
 * or CO₂) in a compact card with icon, value, and unit.
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function EnviroCard({ title, value, unit, color, icon }) {
  return (
    <View style={[styles.card, { borderLeftColor: color }]}>
      <Text style={styles.icon}>{icon}</Text>
      <Text style={styles.title}>{title}</Text>
      <Text style={[styles.value, { color }]}>{value}</Text>
      <Text style={styles.unit}>{unit}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1, backgroundColor: '#1a2a1a', borderRadius: 8, padding: 10,
    alignItems: 'center', borderLeftWidth: 3,
  },
  icon: { fontSize: 20, marginBottom: 4 },
  title: { color: '#888', fontSize: 10, fontWeight: 'bold', textAlign: 'center' },
  value: { fontSize: 18, fontWeight: 'bold', marginTop: 4 },
  unit: { color: '#666', fontSize: 10, marginTop: 2 },
});