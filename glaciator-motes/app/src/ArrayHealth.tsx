// src/ArrayHealth.tsx — per-node health table
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, FlatList, RefreshControl } from 'react-native';
import { Card, Title, Paragraph } from 'react-native-paper';

interface NodeHealth {
  nodeId: number;
  vbatMv: number;
  solarPct: number;
  tempC: number;
  lastSeenS: number;
  sampleRate: number;
  timingSkewUs: number;
  state: string;
  eventsToday: number;
}

const INITIAL_NODES: NodeHealth[] = [
  { nodeId: 1,  vbatMv: 3320, solarPct: 68, tempC: -12, lastSeenS: 2,  sampleRate: 200, timingSkewUs: 12,  state: 'NORMAL',   eventsToday: 47 },
  { nodeId: 3,  vbatMv: 3280, solarPct: 65, tempC: -14, lastSeenS: 5,  sampleRate: 200, timingSkewUs: 28,  state: 'NORMAL',   eventsToday: 62 },
  { nodeId: 7,  vbatMv: 3350, solarPct: 72, tempC: -10, lastSeenS: 1,  sampleRate: 200, timingSkewUs:  8,  state: 'NORMAL',   eventsToday: 51 },
  { nodeId: 9,  vbatMv: 3180, solarPct: 58, tempC: -18, lastSeenS: 12, sampleRate: 200, timingSkewUs: 45,  state: 'DEGRADED', eventsToday: 23 },
  { nodeId: 12, vbatMv: 3300, solarPct: 70, tempC: -11, lastSeenS: 3,  sampleRate: 200, timingSkewUs: 15,  state: 'NORMAL',   eventsToday: 38 },
  { nodeId: 18, vbatMv: 3260, solarPct: 62, tempC: -15, lastSeenS: 8,  sampleRate: 200, timingSkewUs: 33,  state: 'NORMAL',   eventsToday: 44 },
  { nodeId: 22, vbatMv: 3050, solarPct: 41, tempC: -22, lastSeenS: 45, sampleRate: 60,  timingSkewUs: 120, state: 'SURVIVAL', eventsToday:  3 },
];

function stateColor(s: string): string {
  switch (s) {
    case 'NORMAL':   return '#28a745';
    case 'DEGRADED': return '#ffc107';
    case 'SURVIVAL': return '#dc3545';
    default:         return '#6c757d';
  }
}

export default function ArrayHealth() {
  const [nodes, setNodes] = useState<NodeHealth[]>(INITIAL_NODES);
  const [refreshing, setRefreshing] = useState(false);

  const refresh = useCallback(() => {
    setRefreshing(true);
    setTimeout(() => {
      // In production: pull from gateway's health table
      setNodes(INITIAL_NODES.map(n => ({
        ...n,
        lastSeenS: Math.max(0, n.lastSeenS + Math.floor(Math.random() * 4 - 2)),
        vbatMv: Math.max(2900, n.vbatMv + Math.floor(Math.random() * 20 - 10)),
        solarPct: Math.max(0, Math.min(100, n.solarPct + Math.floor(Math.random() * 10 - 5))),
      })));
      setRefreshing(false);
    }, 600);
  }, []);

  useEffect(() => {
    const id = setInterval(refresh, 10000);
    return () => clearInterval(id);
  }, [refresh]);

  const renderItem = ({ item }: { item: NodeHealth }) => (
    <Card style={styles.card}>
      <Card.Content style={styles.row}>
        <View style={styles.nodeBox}>
          <Text style={styles.nodeId}>#{item.nodeId}</Text>
          <Text style={[styles.state, { color: stateColor(item.state) }]}>{item.state}</Text>
        </View>
        <View style={styles.metrics}>
          <Text style={styles.metric}>Vbat: <Text style={styles.val}>{(item.vbatMv / 1000).toFixed(2)} V</Text></Text>
          <Text style={styles.metric}>Solar: <Text style={styles.val}>{item.solarPct}%</Text></Text>
          <Text style={styles.metric}>Temp: <Text style={styles.val}>{item.tempC}°C</Text></Text>
        </View>
        <View style={styles.metrics}>
          <Text style={styles.metric}>Last seen: <Text style={styles.val}>{item.lastSeenS}s</Text></Text>
          <Text style={styles.metric}>Rate: <Text style={styles.val}>{item.sampleRate} SPS</Text></Text>
          <Text style={styles.metric}>Skew: <Text style={styles.val}>±{item.timingSkewUs} µs</Text></Text>
        </View>
        <View style={styles.events}>
          <Text style={styles.eventCount}>{item.eventsToday}</Text>
          <Text style={styles.eventLbl}>events today</Text>
        </View>
      </Card.Content>
    </Card>
  );

  return (
    <View style={styles.container}>
      <View style={styles.headerRow}>
        <Title style={styles.header}>Array Health ({nodes.length} motes)</Title>
        <Paragraph style={styles.sub}>Refreshes every 10 s</Paragraph>
      </View>
      <FlatList
        data={nodes}
        keyExtractor={(item) => String(item.nodeId)}
        renderItem={renderItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1220', padding: 8 },
  headerRow: { paddingHorizontal: 8, paddingBottom: 8 },
  header: { color: '#e6eaf2' },
  sub: { color: '#7a8aa8', fontSize: 12 },
  list: { paddingBottom: 16 },
  card: { backgroundColor: '#141b2d', marginBottom: 6 },
  row: { flexDirection: 'row', alignItems: 'center' },
  nodeBox: { width: 60, alignItems: 'center' },
  nodeId: { color: '#0d6efd', fontSize: 20, fontWeight: 'bold' },
  state: { fontSize: 10, fontWeight: '600', marginTop: 2 },
  metrics: { flex: 1, paddingHorizontal: 8 },
  metric: { color: '#9fb0d0', fontSize: 12, marginVertical: 1 },
  val: { color: '#e6eaf2', fontWeight: '600' },
  events: { alignItems: 'center', paddingHorizontal: 8 },
  eventCount: { color: '#17a2b8', fontSize: 24, fontWeight: 'bold' },
  eventLbl: { color: '#7a8aa8', fontSize: 10 },
});