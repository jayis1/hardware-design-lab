/*
 * screens/Alerts.tsx — list of recent anomaly alerts from all nodes
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import React, { useEffect, useState, useCallback } from 'react';
import {
  View, Text, FlatList, StyleSheet, TouchableOpacity, RefreshControl, Alert,
} from 'react-native';

import { MusselWatchClient } from '../api';
import {
  AppConfig, AlertEvent, anomalyLabel, alertLevelFromScore, formatTemp,
} from '../types';

export function AlertsScreen({ config }: { config: AppConfig }) {
  const [alerts, setAlerts] = useState<AlertEvent[]>([]);
  const [refreshing, setRefreshing] = useState(true);
  const client = new MusselWatchClient(config);

  const refresh = useCallback(async () => {
    setRefreshing(true);
    try {
      const a = await client.getAlerts();
      a.sort((x, y) => y.timestamp - x.timestamp);
      setAlerts(a);
    } finally {
      setRefreshing(false);
    }
  }, [config]);

  useEffect(() => {
    refresh();
    const id = setInterval(refresh, config.pollIntervalS * 1000);
    return () => clearInterval(id);
  }, [refresh, config.pollIntervalS]);

  const ack = (alert: AlertEvent) => {
    Alert.prompt(
      'Acknowledge alert',
      'Add a note (optional):',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Acknowledge', onPress: async (note?: string) => {
          await client.acknowledgeAlert(alert.id, note ?? '');
          refresh();
        }},
      ],
    );
  };

  const renderItem = ({ item }: { item: AlertEvent }) => {
    const level = item.level;
    const color = level === 'critical' ? '#b00020' :
                  level === 'warning' ? '#c2410c' :
                  level === 'advisory' ? '#b45309' : '#2e7d32';
    const ageMin = Math.round((Date.now() - item.timestamp) / 60000);

    return (
      <View style={[styles.card, { borderLeftColor: color }]}>
        <View style={styles.cardHeader}>
          <Text style={styles.levelText}>{level.toUpperCase()}</Text>
          <Text style={styles.ageText}>{ageMin} min ago</Text>
        </View>
        <Text style={styles.nodeText}>Node {item.nodeId} · Channel {item.channel + 1}</Text>
        <Text style={styles.anomalyText}>{anomalyLabel(item.anomalyFlag)}</Text>
        <Text style={styles.noteText}>{item.note}</Text>
        <View style={styles.metricRow}>
          <Mini label="Gape" value={`${item.gapeUm} µm`} />
          <Mini label="Activity" value={`${item.activityScore}`} />
          <Mini label="Water" value={formatTemp(item.waterTempC10, config.temperatureUnitC)} />
          <Mini label="State" value={item.acknowledged ? 'ACK' : 'OPEN'} />
        </View>
        {!item.acknowledged && (
          <TouchableOpacity style={styles.ackBtn} onPress={() => ack(item)}>
            <Text style={styles.ackBtnText}>Acknowledge</Text>
          </TouchableOpacity>
        )}
      </View>
    );
  };

  const openCount = alerts.filter((a) => !a.acknowledged).length;

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Anomaly Alerts</Text>
        <Text style={styles.headerSub}>
          {openCount} open · {alerts.length} total
        </Text>
      </View>
      <FlatList
        data={alerts}
        keyExtractor={(a) => a.id}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
      />
    </View>
  );
}

function Mini({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.mini}>
      <Text style={styles.miniLabel}>{label}</Text>
      <Text style={styles.miniValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f2c' },
  header: { padding: 16, backgroundColor: '#0a4d6e' },
  headerTitle: { color: '#e0f0f5', fontSize: 18, fontWeight: '700' },
  headerSub: { color: '#a0c5d5', fontSize: 13, marginTop: 2 },
  list: { padding: 12 },
  card: {
    backgroundColor: '#10334a', borderRadius: 8, padding: 12, marginBottom: 10,
    borderLeftWidth: 4,
  },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between' },
  levelText: { color: '#f5b8a0', fontSize: 11, fontWeight: '700' },
  ageText: { color: '#8aa8b8', fontSize: 11 },
  nodeText: { color: '#d0e8f2', fontSize: 14, fontWeight: '600', marginTop: 6 },
  anomalyText: { color: '#7fcfff', fontSize: 13, marginTop: 2 },
  noteText: { color: '#9fb8c7', fontSize: 12, marginTop: 4, fontStyle: 'italic' },
  metricRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 10 },
  mini: { alignItems: 'center' },
  miniLabel: { color: '#8aa8b8', fontSize: 10 },
  miniValue: { color: '#d0e8f2', fontSize: 12, fontWeight: '600', marginTop: 2 },
  ackBtn: {
    marginTop: 10, padding: 8, borderRadius: 6, backgroundColor: '#0a4d6e', alignItems: 'center',
  },
  ackBtnText: { color: '#e0f0f5', fontSize: 12, fontWeight: '600' },
});