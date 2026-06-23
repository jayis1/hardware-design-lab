/**
 * ActivityBadge.js — Activity class indicator badge for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Small colored badge that displays the current fungal activity class
 * label (IDLE, FORAGE, TRANSPORT, STRESS, COMPETE).
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const LABELS = ['IDLE', 'FORAGE', 'TRANSPORT', 'STRESS', 'COMPETE'];
const COLORS = ['#666', '#4CAF50', '#2196F3', '#FF5722', '#FF1744'];
const ICONS = ['😴', '🌱', '📡', '⚠️', '⚔️'];

export default function ActivityBadge({ classLabel }) {
  const label = LABELS[classLabel] || 'UNKNOWN';
  const color = COLORS[classLabel] || '#666';
  const icon = ICONS[classLabel] || '❓';

  return (
    <View style={[styles.badge, { backgroundColor: color }]}>
      <Text style={styles.icon}>{icon}</Text>
      <Text style={styles.label}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row', alignItems: 'center', paddingHorizontal: 10,
    paddingVertical: 4, borderRadius: 12, gap: 4,
  },
  icon: { fontSize: 14 },
  label: { color: '#fff', fontWeight: 'bold', fontSize: 11 },
});