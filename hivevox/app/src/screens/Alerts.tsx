/*
 * screens/Alerts.tsx — Alert feed for hive anomalies
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import React from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, Alert
} from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import type { AlertEntry } from '../App';

type Props = {
  alerts: AlertEntry[];
  onAck: (id: number) => void;
};

const ALERT_ICONS: Record<string, string> = {
  queenless: 'alert-circle',
  swarm_prep: 'git-branch',
  dead: 'close-circle',
  weight_drop: 'trending-down',
  low_battery: 'battery-dead',
  probe_fault: 'thermometer',
};

const ALERT_COLORS: Record<string, string> = {
  queenless: '#f44336',
  swarm_prep: '#ff9800',
  dead: '#9e9e9e',
  weight_drop: '#e91e63',
  low_battery: '#ffc107',
  probe_fault: '#795548',
};

export default function Alerts({ alerts, onAck }: Props) {
  const handleAck = (alert: AlertEntry) => {
    Alert.alert(
      'Acknowledge Alert',
      `Mark "${alert.message}" as reviewed?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Acknowledge', onPress: () => onAck(alert.id) },
      ]
    );
  };

  const renderItem = ({ item }: { item: AlertEntry }) => {
    const icon = ALERT_ICONS[item.type] || 'notifications';
    const color = ALERT_COLORS[item.type] || '#888';
    const time = new Date(item.timestamp * 1000);
    const timeStr = time.toLocaleDateString() + ' ' +
      time.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

    return (
      <TouchableOpacity
        style={[styles.alertCard, { borderLeftColor: color }]}
        onPress={() => handleAck(item)}
        activeOpacity={0.7}
      >
        <View style={styles.alertHeader}>
          <Ionicons name={icon as any} size={22} color={color} />
          <Text style={styles.alertType}>{item.hiveName}</Text>
          <Text style={styles.alertTime}>{timeStr}</Text>
        </View>
        <Text style={styles.alertMessage}>{item.message}</Text>
        <Text style={styles.ackHint}>Tap to acknowledge →</Text>
      </TouchableOpacity>
    );
  };

  if (alerts.length === 0) {
    return (
      <View style={styles.empty}>
        <Ionicons name="checkmark-circle" size={64} color="#4caf50" />
        <Text style={styles.emptyTitle}>All clear!</Text>
        <Text style={styles.emptyText}>
          No active alerts. All your hives are reporting normally.
        </Text>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Alerts ({alerts.length})</Text>
      <FlatList
        data={alerts}
        keyExtractor={(item) => item.id.toString()}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  title: {
    fontSize: 24, fontWeight: '800', color: '#222',
    margin: 16, marginBottom: 8,
  },
  list: { paddingHorizontal: 12, paddingBottom: 20 },
  alertCard: {
    backgroundColor: '#fff', borderRadius: 10, padding: 14,
    marginVertical: 6, borderLeftWidth: 4, elevation: 2,
    shadowColor: '#000', shadowOpacity: 0.08, shadowRadius: 3,
    shadowOffset: { width: 0, height: 1 },
  },
  alertHeader: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  alertType: { fontSize: 15, fontWeight: '700', color: '#333', marginLeft: 8, flex: 1 },
  alertTime: { fontSize: 11, color: '#999' },
  alertMessage: { fontSize: 13, color: '#555', lineHeight: 19 },
  ackHint: { fontSize: 11, color: '#bbb', marginTop: 8, textAlign: 'right' },
  empty: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 40 },
  emptyTitle: { fontSize: 20, fontWeight: '700', color: '#4caf50', marginTop: 12 },
  emptyText: { fontSize: 14, color: '#888', textAlign: 'center', marginTop: 8 },
});