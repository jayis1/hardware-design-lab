/**
 * EventItem.js — Event list item component
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

const CLASS_COLORS = {
  0: '#4CAF50', 1: '#66BB6A', 2: '#8D6E63', 3: '#A1887F', 4: '#FFB74D',
  5: '#F44336', 6: '#26A69A', 7: '#42A5F5', 8: '#FF7043', 9: '#BDBDBD',
};

const CLASS_ICONS = {
  0: '🌱', 1: '💧', 2: '🪱', 3: '🪱', 4: '🦗', 5: '🐛', 6: '🫧', 7: '🌧️', 8: '⚒️', 9: '🔊',
};

export function EventItem({ event, onPress }) {
  const color = CLASS_COLORS[event.classId] || '#999';
  const icon = CLASS_ICONS[event.classId] || '❓';
  const time = new Date(event.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  const date = new Date(event.timestamp).toLocaleDateString([], { month: 'short', day: 'numeric' });
  const depth = Math.abs(event.posZ || 0).toFixed(0);

  return (
    <TouchableOpacity style={styles.item} onPress={onPress} activeOpacity={0.7}>
      <View style={[styles.iconBox, { backgroundColor: color + '22' }]}>
        <Text style={styles.icon}>{icon}</Text>
      </View>
      <View style={styles.info}>
        <Text style={styles.className}>{event.className?.replace(/_/g, ' ') || 'unknown'}</Text>
        <Text style={styles.meta}>
          {date} {time} · {depth}cm depth · {event.confidence?.toFixed(0)}% conf
        </Text>
        <Text style={styles.podRef}>Pod: {event.podId || 'unknown'}</Text>
      </View>
      <Text style={styles.chevron}>›</Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  item: { flexDirection: 'row', alignItems: 'center', padding: 12, backgroundColor: '#FFFFFF', borderRadius: 10, marginBottom: 6, elevation: 1 },
  iconBox: { width: 40, height: 40, borderRadius: 10, alignItems: 'center', justifyContent: 'center' },
  icon: { fontSize: 20 },
  info: { flex: 1, marginLeft: 12 },
  className: { fontSize: 14, fontWeight: 'bold', color: '#424242' },
  meta: { fontSize: 11, color: '#757575', marginTop: 2 },
  podRef: { fontSize: 10, color: '#9E9E9E', marginTop: 2 },
  chevron: { fontSize: 22, color: '#BDBDBD' },
});