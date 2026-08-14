/*
 * screens/NetworkDashboard.tsx — overview of all MusselWatch nodes
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import React, { useEffect, useState, useCallback } from 'react';
import {
  View, Text, FlatList, StyleSheet, TouchableOpacity, RefreshControl,
} from 'react-native';
import { useNavigation } from '@react-navigation/native';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';

import { MusselWatchClient } from '../api';
import {
  AppConfig, Node, ChannelState, anomalyLabel, batteryPct,
  formatTemp, formatUptime, chargerStateLabel, alertLevelFromScore,
} from '../types';

type NavProp = NativeStackNavigationProp<{ NodeDetail: { node: Node } }>;

export function NetworkDashboard({ config }: { config: AppConfig }) {
  const [nodes, setNodes] = useState<Node[]>([]);
  const [refreshing, setRefreshing] = useState(true);
  const navigation = useNavigation<NavProp>();
  const client = new MusselWatchClient(config);

  const refresh = useCallback(async () => {
    setRefreshing(true);
    try {
      const n = await client.getNodes();
      setNodes(n);
    } finally {
      setRefreshing(false);
    }
  }, [config]);

  useEffect(() => {
    refresh();
    const id = setInterval(refresh, config.pollIntervalS * 1000);
    return () => clearInterval(id);
  }, [refresh, config.pollIntervalS]);

  const renderItem = ({ item }: { item: Node }) => {
    const t = item.telemetry;
    const batt = t ? batteryPct(t.batteryMv) : 0;
    const level = t ? alertLevelFromScore(t.maxAnomaly) : 'nominal';
    const levelColor = levelColorFor(level);
    const activeChs = item.channels.filter((c) => c.anomalyFlag > 0);

    return (
      <TouchableOpacity
        style={styles.card}
        onPress={() => navigation.navigate('NodeDetail', { node: item })}
      >
        <View style={styles.cardHeader}>
          <Text style={styles.nodeLabel}>{item.label}</Text>
          <View style={[styles.levelBadge, { backgroundColor: levelColor }]}>
            <Text style={styles.levelText}>{level.toUpperCase()}</Text>
          </View>
        </View>
        <Text style={styles.nodeSub}>
          {item.river} · {item.reach}
        </Text>
        <Text style={styles.nodeSub}>Species: {item.species}</Text>

        <View style={styles.metricRow}>
          <Metric label="Battery" value={`${batt}%`} />
          <Metric label="Solar" value={t ? `${t.solarMv} mV` : '—'} />
          <Metric label="Water" value={t ? formatTemp(t.waterTempC10, config.temperatureUnitC) : '—'} />
          <Metric label="Charger" value={t ? chargerStateLabel(t.chargerState) : '—'} />
        </View>

        <View style={styles.metricRow}>
          <Metric label="Channels" value={`${item.channels.length}/8`} />
          <Metric label="Max anomaly" value={`${t?.maxAnomaly ?? 0}`} />
          <Metric label="Uptime" value={t ? formatUptime(t.uptimeS) : '—'} />
          <Metric label="Last seen" value={`${item.lastSeenS}s`} />
        </View>

        {activeChs.length > 0 && (
          <View style={styles.alertRow}>
            <Text style={styles.alertText}>
              ⚠ {activeChs.length} channel(s) in anomaly: {activeChs.map((c) => anomalyLabel(c.anomalyFlag)).join(', ')}
            </Text>
          </View>
        )}
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Bivalve Biosensor Network</Text>
        <Text style={styles.headerSub}>Monitoring {nodes.length} nodes</Text>
      </View>
      <FlatList
        data={nodes}
        keyExtractor={(n) => n.nodeId}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
      />
    </View>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.metric}>
      <Text style={styles.metricLabel}>{label}</Text>
      <Text style={styles.metricValue}>{value}</Text>
    </View>
  );
}

function levelColorFor(level: string): string {
  switch (level) {
    case 'critical': return '#b00020';
    case 'warning':  return '#c2410c';
    case 'advisory': return '#b45309';
    default:          return '#2e7d32';
  }
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f2c' },
  header: { padding: 16, backgroundColor: '#0a4d6e' },
  headerTitle: { color: '#e0f0f5', fontSize: 18, fontWeight: '700' },
  headerSub: { color: '#a0c5d5', fontSize: 13, marginTop: 2 },
  list: { padding: 12 },
  card: {
    backgroundColor: '#10334a', borderRadius: 10, padding: 14, marginBottom: 12,
    borderWidth: 1, borderColor: '#1c4a66',
  },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  nodeLabel: { color: '#e0f0f5', fontSize: 17, fontWeight: '600' },
  nodeSub: { color: '#9fb8c7', fontSize: 12, marginTop: 3 },
  levelBadge: { borderRadius: 4, paddingHorizontal: 8, paddingVertical: 2 },
  levelText: { color: '#fff', fontSize: 10, fontWeight: '700' },
  metricRow: { flexDirection: 'row', justifyContent: 'space-between', marginTop: 10 },
  metric: { flex: 1, alignItems: 'center' },
  metricLabel: { color: '#8aa8b8', fontSize: 10 },
  metricValue: { color: '#d0e8f2', fontSize: 13, fontWeight: '600', marginTop: 2 },
  alertRow: {
    marginTop: 10, padding: 8, borderRadius: 6, backgroundColor: '#3a1208',
  },
  alertText: { color: '#f5b8a0', fontSize: 12 },
});