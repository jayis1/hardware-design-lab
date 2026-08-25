/**
 * TrendPanel.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

function renderBars(points, color) {
  return points.map((point, index) => {
    const height = Math.max(6, Math.min(64, point));
    return <View key={`${color}-${index}`} style={[styles.bar, { height, backgroundColor: color }]} />;
  });
}

export default function TrendPanel({ title, points, color = '#60a5fa', note = '' }) {
  return (
    <View style={styles.panel}>
      <Text style={styles.title}>{title}</Text>
      <View style={styles.graph}>{renderBars(points, color)}</View>
      {note ? <Text style={styles.note}>{note}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {
    backgroundColor: '#0f172a',
    borderRadius: 16,
    padding: 14,
    marginBottom: 14
  },
  title: {
    color: '#e5e7eb',
    fontWeight: '700',
    marginBottom: 12,
    fontSize: 16
  },
  graph: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    justifyContent: 'space-between',
    height: 72
  },
  bar: {
    width: '7.2%',
    borderRadius: 6
  },
  note: {
    color: '#94a3b8',
    marginTop: 10,
    fontSize: 12
  }
});
