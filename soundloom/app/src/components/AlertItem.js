/**
 * AlertItem.js — Alert list item component
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

const SEVERITY_COLORS = {
  high: '#F44336',
  medium: '#FF9800',
  low: '#FFC107',
};

const TIME_AGO = (ts) => {
  const diff = Date.now() - ts;
  const h = Math.floor(diff / 3600000);
  const m = Math.floor((diff % 3600000) / 60000);
  const d = Math.floor(h / 24);
  if (d > 0) return `${d}d ago`;
  if (h > 0) return `${h}h ago`;
  return `${m}m ago`;
};

export function AlertItem({ alert, typeInfo, onDismiss, onAction, onViewPod }) {
  const color = typeInfo?.color || '#757575';
  const sevColor = SEVERITY_COLORS[alert.severity] || '#FFC107';

  return (
    <View style={[styles.card, { borderLeftColor: color }]}>
      <View style={styles.header}>
        <Text style={styles.icon}>{typeInfo?.icon || '⚠️'}</Text>
        <View style={styles.headerInfo}>
          <Text style={styles.title}>{typeInfo?.title || 'Alert'}</Text>
          <Text style={styles.time}>{TIME_AGO(alert.timestamp)}</Text>
        </View>
        <View style={[styles.severityBadge, { backgroundColor: sevColor }]}>
          <Text style={styles.severityText}>{alert.severity}</Text>
        </View>
      </View>
      <Text style={styles.message}>{alert.message}</Text>
      <Text style={styles.podName}>📍 {alert.podName || alert.podId}</Text>
      <View style={styles.actions}>
        <TouchableOpacity style={[styles.actionBtn, { backgroundColor: color }]} onPress={onAction}>
          <Text style={styles.actionText}>Take Action</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, { backgroundColor: '#757575' }]} onPress={onViewPod}>
          <Text style={styles.actionText}>View Pod</Text>
        </TouchableOpacity>
        <TouchableOpacity style={[styles.actionBtn, { backgroundColor: '#E0E0E0' }]} onPress={onDismiss}>
          <Text style={[styles.actionText, { color: '#757575' }]}>Dismiss</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#FFFFFF', borderRadius: 10, padding: 14, marginBottom: 8, borderLeftWidth: 4, elevation: 1 },
  header: { flexDirection: 'row', alignItems: 'center' },
  icon: { fontSize: 28 },
  headerInfo: { flex: 1, marginLeft: 10 },
  title: { fontSize: 16, fontWeight: 'bold', color: '#424242' },
  time: { fontSize: 11, color: '#9E9E9E', marginTop: 2 },
  severityBadge: { paddingHorizontal: 8, paddingVertical: 3, borderRadius: 8 },
  severityText: { fontSize: 10, color: '#FFFFFF', fontWeight: 'bold', textTransform: 'uppercase' },
  message: { fontSize: 13, color: '#616161', marginTop: 8, lineHeight: 18 },
  podName: { fontSize: 12, color: '#2E7D32', marginTop: 5, fontWeight: 'bold' },
  actions: { flexDirection: 'row', marginTop: 10, justifyContent: 'flex-end' },
  actionBtn: { paddingHorizontal: 12, paddingVertical: 7, borderRadius: 6, marginLeft: 8 },
  actionText: { fontSize: 12, color: '#FFFFFF', fontWeight: 'bold' },
});