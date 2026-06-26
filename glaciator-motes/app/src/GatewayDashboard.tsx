// src/GatewayDashboard.tsx — live event feed + array map
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, RefreshControl,
} from 'react-native';
import { Card, Title, Paragraph, Chip, IconButton } from 'react-native-paper';
import { useNavigation } from '@react-navigation/native';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';
import type { RootStackParamList } from '../App';

interface SeismicEvent {
  id: string;
  nodeId: number;
  tsUtcMs: number;
  type: string;
  magnitude: number;
  correlScore: number;
  lat: number;
  lon: number;
}

interface NodeHealth {
  nodeId: number;
  vbatMv: number;
  solarPct: number;
  tempC: number;
  lastSeenS: number;
  sampleRate: number;
  timingSkewUs: number;
  lat: number;
  lon: number;
}

// --- Mock data (in production: streamed from gateway mote over USB-C) ---
const MOCK_EVENTS: SeismicEvent[] = [
  { id: 'e1', nodeId: 7,  tsUtcMs: Date.now() - 12000, type: 'Crevasse snap',     magnitude: -0.5, correlScore: 0.88, lat: 78.92, lon: -11.93 },
  { id: 'e2', nodeId: 12, tsUtcMs: Date.now() - 65000, type: 'Basal slide',       magnitude:  0.8, correlScore: 0.92, lat: 78.91, lon: -11.95 },
  { id: 'e3', nodeId: 3,  tsUtcMs: Date.now() - 180000, type: 'Calving thump',    magnitude:  2.1, correlScore: 0.76, lat: 78.90, lon: -11.90 },
  { id: 'e4', nodeId: 18, tsUtcMs: Date.now() - 420000, type: 'Subglacial tremor', magnitude: 0.3, correlScore: 0.64, lat: 78.93, lon: -11.97 },
  { id: 'e5', nodeId: 9,  tsUtcMs: Date.now() - 900000, type: 'Icequake (local)', magnitude:  1.4, correlScore: 0.81, lat: 78.92, lon: -11.92 },
];

const MOCK_NODES: NodeHealth[] = [
  { nodeId: 1,  vbatMv: 3320, solarPct: 68, tempC: -12, lastSeenS: 2,  sampleRate: 200, timingSkewUs: 12, lat: 78.92, lon: -11.93 },
  { nodeId: 3,  vbatMv: 3280, solarPct: 65, tempC: -14, lastSeenS: 5,  sampleRate: 200, timingSkewUs: 28, lat: 78.91, lon: -11.94 },
  { nodeId: 7,  vbatMv: 3350, solarPct: 72, tempC: -10, lastSeenS: 1,  sampleRate: 200, timingSkewUs:  8, lat: 78.93, lon: -11.92 },
  { nodeId: 9,  vbatMv: 3180, solarPct: 58, tempC: -18, lastSeenS: 12, sampleRate: 200, timingSkewUs: 45, lat: 78.90, lon: -11.96 },
  { nodeId: 12, vbatMv: 3300, solarPct: 70, tempC: -11, lastSeenS: 3,  sampleRate: 200, timingSkewUs: 15, lat: 78.92, lon: -11.98 },
  { nodeId: 18, vbatMv: 3260, solarPct: 62, tempC: -15, lastSeenS: 8,  sampleRate: 200, timingSkewUs: 33, lat: 78.94, lon: -11.95 },
];

function timeAgo(tsMs: number): string {
  const s = Math.floor((Date.now() - tsMs) / 1000);
  if (s < 60) return `${s}s ago`;
  if (s < 3600) return `${Math.floor(s / 60)}m ago`;
  return `${Math.floor(s / 3600)}h ago`;
}

function magColor(m: number): string {
  if (m < 0) return '#6c757d';
  if (m < 1) return '#17a2b8';
  if (m < 2) return '#ffc107';
  return '#dc3545';
}

export default function GatewayDashboard() {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();
  const [events, setEvents] = useState<SeismicEvent[]>(MOCK_EVENTS);
  const [nodes, setNodes] = useState<NodeHealth[]>(MOCK_NODES);
  const [refreshing, setRefreshing] = useState(false);

  const refresh = useCallback(() => {
    setRefreshing(true);
    // In production: query gateway mote for latest events + health
    setTimeout(() => {
      setEvents(MOCK_EVENTS);
      setNodes(MOCK_NODES);
      setRefreshing(false);
    }, 800);
  }, []);

  // Auto-refresh every 5 s in production
  useEffect(() => {
    const id = setInterval(refresh, 5000);
    return () => clearInterval(id);
  }, [refresh]);

  const totalNodes = nodes.length;
  const avgVbat = Math.round(nodes.reduce((a, n) => a + n.vbatMv, 0) / totalNodes);
  const avgSolar = Math.round(nodes.reduce((a, n) => a + n.solarPct, 0) / totalNodes);
  const eventsLastHour = events.filter((e) => Date.now() - e.tsUtcMs < 3600000).length;

  const renderEvent = ({ item }: { item: SeismicEvent }) => (
    <TouchableOpacity onPress={() => navigation.navigate('EventDetail', { eventId: item.id })}>
      <Card style={styles.eventCard}>
        <Card.Content style={styles.eventRow}>
          <View style={styles.eventLeft}>
            <Text style={styles.eventType}>{item.type}</Text>
            <Text style={styles.eventMeta}>
              Node #{item.nodeId} · {timeAgo(item.tsUtcMs)}
            </Text>
            <Text style={styles.eventMeta}>
              xcorr={item.correlScore.toFixed(2)} · {item.lat.toFixed(4)}°, {item.lon.toFixed(4)}°
            </Text>
          </View>
          <Chip style={[styles.magChip, { backgroundColor: magColor(item.magnitude) }]}>
            Mv {item.magnitude.toFixed(1)}
          </Chip>
        </Card.Content>
      </Card>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      {/* Summary cards */}
      <View style={styles.summaryRow}>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{totalNodes}</Title>
            <Paragraph style={styles.summaryLbl}>Active nodes</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{eventsLastHour}</Title>
            <Paragraph style={styles.summaryLbl}>Events / hr</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{(avgVbat / 1000).toFixed(2)}V</Title>
            <Paragraph style={styles.summaryLbl}>Avg battery</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{avgSolar}%</Title>
            <Paragraph style={styles.summaryLbl}>Avg solar</Paragraph>
          </Card.Content>
        </Card>
      </View>

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <IconButton icon="map" color="#0d6efd" onPress={() => {}} />
        <IconButton icon="heart-pulse" color="#28a745" onPress={() => navigation.navigate('Health')} />
        <IconButton icon="cog" color="#ffc107" onPress={() => navigation.navigate('Config')} />
        <IconButton icon="plus" color="#17a2b8" onPress={() => navigation.navigate('Deploy')} />
      </View>

      {/* Event feed */}
      <Text style={styles.sectionHeader}>Recent seismic events</Text>
      <FlatList
        data={events}
        keyExtractor={(item) => item.id}
        renderItem={renderEvent}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1220', paddingTop: 8 },
  summaryRow: { flexDirection: 'row', paddingHorizontal: 8, marginBottom: 4 },
  summaryCard: { flex: 1, backgroundColor: '#141b2d', marginHorizontal: 4 },
  summaryVal: { color: '#0d6efd', fontSize: 22, textAlign: 'center' },
  summaryLbl: { color: '#9fb0d0', fontSize: 11, textAlign: 'center' },
  actionRow: { flexDirection: 'row', justifyContent: 'center', paddingVertical: 4 },
  sectionHeader: { color: '#e6eaf2', fontSize: 16, fontWeight: 'bold', paddingHorizontal: 16, paddingTop: 8 },
  list: { paddingHorizontal: 8, paddingBottom: 16 },
  eventCard: { backgroundColor: '#141b2d', marginBottom: 8 },
  eventRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  eventLeft: { flex: 1 },
  eventType: { color: '#e6eaf2', fontSize: 16, fontWeight: '600' },
  eventMeta: { color: '#7a8aa8', fontSize: 12, marginTop: 2 },
  magChip: { borderRadius: 12, paddingHorizontal: 4 },
});