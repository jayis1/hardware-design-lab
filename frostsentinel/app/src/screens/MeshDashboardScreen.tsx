// src/screens/MeshDashboardScreen.tsx — Mesh overview dashboard
//
// Shows all known mesh nodes with their current RFRI, T_wet, ΔT_rad,
// and AE status, color-coded. Tap a node to see its detail.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState, useCallback } from 'react';
import {
  View, Text, FlatList, StyleSheet, RefreshControl,
  TouchableOpacity, Alert,
} from 'react-native';
import bleManager from '../ble/BleManager';
import db from '../db/database';
import { rfriColor, aeStatusLabel, AE_STATUS_NUCLEATION } from '../ble/protocol';
import type { LiveData } from '../ble/BleManager';

interface MeshNode {
  nodeId: number;
  label: string;
  rfri: number;
  twet: number;
  deltaRad: number;
  aeStatus: number;
  batteryPct: number;
  lastSeen: number;
}

export default function MeshDashboardScreen({ navigation }: any) {
  const [nodes, setNodes] = useState<MeshNode[]>([]);
  const [refreshing, setRefreshing] = useState(false);
  const [liveData, setLiveData] = useState<Map<number, LiveData>>(new Map());

  useEffect(() => {
    loadNodes();

    const unsub = bleManager.on('liveData', (data: LiveData) => {
      setLiveData(prev => {
        const m = new Map(prev);
        m.set(data.nodeId, data);
        return m;
      });
    });
    return unsub;
  }, []);

  const loadNodes = useCallback(async () => {
    try {
      await db.open();
      const dbNodes = await db.getNodes();
      const meshNodes: MeshNode[] = dbNodes.map(n => ({
        nodeId: n.nodeId,
        label: n.label || `Node ${n.nodeId}`,
        rfri: n.lastRfri,
        twet: n.lastTwet,
        deltaRad: n.lastDeltaRad,
        aeStatus: n.lastAeStatus,
        batteryPct: 0,
        lastSeen: n.lastSeen,
      }));
      setNodes(meshNodes);
    } catch (e) {
      console.warn('[MeshDashboard] loadNodes:', e);
    }
  }, []);

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    await loadNodes();
    try {
      await bleManager.requestStatus();
    } catch (e) {
      // not connected
    }
    setRefreshing(false);
  }, [loadNodes]);

  // Merge live data into node list
  const displayNodes: MeshNode[] = nodes.map(n => {
    const live = liveData.get(n.nodeId);
    if (live) {
      return {
        ...n,
        rfri: live.rfri,
        twet: live.twetC,
        deltaRad: live.deltaRadK,
        aeStatus: live.aeStatus,
        batteryPct: live.batteryPct,
        lastSeen: Math.floor(Date.now() / 1000),
      };
    }
    return n;
  });

  // Also include live nodes not in DB yet
  for (const [nodeId, live] of liveData) {
    if (!displayNodes.find(n => n.nodeId === nodeId)) {
      displayNodes.push({
        nodeId,
        label: `Node ${nodeId}`,
        rfri: live.rfri,
        twet: live.twetC,
        deltaRad: live.deltaRadK,
        aeStatus: live.aeStatus,
        batteryPct: live.batteryPct,
        lastSeen: Math.floor(Date.now() / 1000),
      });
    }
  }

  const renderNode = ({ item }: { item: MeshNode }) => {
    const color = rfriColor(item.rfri);
    const isAlert = item.aeStatus === AE_STATUS_NUCLEATION || item.rfri >= 0.85;

    return (
      <TouchableOpacity
        style={[styles.nodeCard, { borderLeftColor: color }]}
        onPress={() => navigation.navigate('Node', { nodeId: item.nodeId })}
      >
        <View style={styles.nodeHeader}>
          <Text style={styles.nodeLabel}>{item.label}</Text>
          {isAlert && <Text style={styles.alertBadge}>⚠ FROST</Text>}
        </View>
        <View style={styles.nodeMetrics}>
          <Metric label="RFRI" value={`${(item.rfri * 100).toFixed(0)}%`} color={color} />
          <Metric label="T_wet" value={`${item.twet.toFixed(1)}°C`}
                  color={item.twet <= 0 ? '#F44336' : '#fff'} />
          <Metric label="ΔT_rad" value={`${item.deltaRad.toFixed(1)}K`} color="#fff" />
          <Metric label="AE" value={aeStatusLabel(item.aeStatus)}
                  color={item.aeStatus === 2 ? '#F44336' : '#ccc'} />
          <Metric label="Bat" value={`${item.batteryPct}%`} color="#aaa" />
        </View>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>FrostSentinel Mesh</Text>
        <Text style={styles.headerSubtitle}>
          {displayNodes.length} node{displayNodes.length !== 1 ? 's' : ''} ·{' '}
          {bleManager.isConnected() ? 'Connected' : 'Offline'}
        </Text>
      </View>
      <FlatList
        data={displayNodes.sort((a, b) => b.rfri - a.rfri)}
        renderItem={renderNode}
        keyExtractor={item => item.nodeId.toString()}
        refreshControl={
          <RefreshControl refreshing={refreshing} onRefresh={onRefresh}
                          tintColor="#2196F3" />
        }
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No nodes found.</Text>
            <Text style={styles.emptyHint}>
              Scan for nodes in the Provisioning tab or ensure BLE is connected.
            </Text>
          </View>
        }
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

function Metric({ label, value, color }: { label: string; value: string; color: string }) {
  return (
    <View style={styles.metric}>
      <Text style={styles.metricLabel}>{label}</Text>
      <Text style={[styles.metricValue, { color }]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1b2a' },
  header: { padding: 15, paddingTop: 30, borderBottomWidth: 1, borderBottomColor: '#1b263b' },
  headerTitle: { fontSize: 20, fontWeight: 'bold', color: '#fff' },
  headerSubtitle: { fontSize: 12, color: '#778da9', marginTop: 4 },
  list: { padding: 10 },
  nodeCard: {
    backgroundColor: '#1b263b',
    borderRadius: 8,
    padding: 12,
    marginBottom: 8,
    borderLeftWidth: 4,
  },
  nodeHeader: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8 },
  nodeLabel: { fontSize: 16, fontWeight: '600', color: '#e0e1dd' },
  alertBadge: {
    fontSize: 11, fontWeight: 'bold', color: '#F44336',
    backgroundColor: 'rgba(244,67,54,0.15)', paddingHorizontal: 6, paddingVertical: 2,
    borderRadius: 4,
  },
  nodeMetrics: { flexDirection: 'row', justifyContent: 'space-between' },
  metric: { alignItems: 'center', flex: 1 },
  metricLabel: { fontSize: 10, color: '#778da9', marginBottom: 2 },
  metricValue: { fontSize: 13, fontWeight: '600' },
  empty: { padding: 40, alignItems: 'center' },
  emptyText: { fontSize: 16, color: '#778da9' },
  emptyHint: { fontSize: 12, color: '#415a77', marginTop: 8, textAlign: 'center' },
});