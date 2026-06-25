/**
 * PodCard.js — Pod summary card for dashboard list
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { SVIGauge } from './SVIGauge';

function sviColor(svi) {
  if (svi >= 75) return '#4CAF50';
  if (svi >= 50) return '#FFEB3B';
  if (svi >= 30) return '#FF9800';
  return '#F44336';
}

export function PodCard({ pod, onPress }) {
  const color = sviColor(pod.svi || 0);

  return (
    <TouchableOpacity style={styles.card} onPress={onPress} activeOpacity={0.7}>
      <SVIGauge value={pod.svi || 0} size={70} />
      <View style={styles.info}>
        <Text style={styles.name}>{pod.name || pod.id}</Text>
        <Text style={styles.id}>ID: {pod.id}</Text>
        <View style={styles.statsRow}>
          <Text style={styles.statLabel}>SVI: </Text>
          <Text style={[styles.statValue, { color }]}>{pod.svi || 0}</Text>
          <Text style={styles.statLabel}>  |  Battery: </Text>
          <Text style={styles.statValue}>{pod.batteryPct || '--'}%</Text>
        </View>
      </View>
      <Text style={styles.chevron}>›</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  card: { flexDirection: 'row', alignItems: 'center', padding: 12, backgroundColor: '#F5F5F5', borderRadius: 10, marginBottom: 8, elevation: 1 },
  info: { flex: 1, marginLeft: 12 },
  name: { fontSize: 16, fontWeight: 'bold', color: '#424242' },
  id: { fontSize: 11, color: '#9E9E9E', marginTop: 2 },
  statsRow: { flexDirection: 'row', alignItems: 'center', marginTop: 6 },
  statLabel: { fontSize: 13, color: '#757575' },
  statValue: { fontSize: 14, fontWeight: 'bold', color: '#424242' },
  chevron: { fontSize: 24, color: '#BDBDBD', marginLeft: 5 },
});