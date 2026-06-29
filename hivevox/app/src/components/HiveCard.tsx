/*
 * components/HiveCard.tsx — Individual hive summary card
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import type { HiveData } from '../App';
import { stateName, stateColor, formatWeight, formatTemp,
         formatRelativeTime, formatBattery } from './StateUtils';

type Props = {
  hive: HiveData;
  onPress: (deveui: string) => void;
};

export default function HiveCard({ hive, onPress }: Props) {
  const color = stateColor(hive.state);
  const hasAlert = hive.state === 1 || hive.state === 2 || hive.state === 4;

  return (
    <TouchableOpacity
      style={[styles.card, { borderLeftColor: color }]}
      onPress={() => onPress(hive.deveui)}
      activeOpacity={0.7}
    >
      <View style={styles.header}>
        <Text style={styles.name} numberOfLines={1}>{hive.name}</Text>
        {hasAlert && (
          <View style={[styles.badge, { backgroundColor: color }]}>
            <Ionicons name="warning" size={12} color="white" />
          </View>
        )}
      </View>
      <View style={styles.stateRow}>
        <View style={[styles.stateDot, { backgroundColor: color }]} />
        <Text style={styles.state}>{stateName(hive.state)}</Text>
      </View>
      <View style={styles.metrics}>
        <Metric icon="thermometer" value={formatTemp(hive.broodTempC)} />
        <Metric icon="scale" value={formatWeight(hive.weightG)} />
        <Metric icon="musical-notes" value={hive.dominantHz + ' Hz'} />
      </View>
      <View style={styles.footer}>
        <Text style={styles.lastSeen}>{formatRelativeTime(hive.lastSeen)}</Text>
        <Text style={styles.battery}>Battery {formatBattery(hive.batteryMv)}</Text>
      </View>
    </TouchableOpacity>
  );
}

function Metric({ icon, value }: { icon: string; value: string }) {
  return (
    <View style={styles.metric}>
      <Ionicons name={icon as any} size={14} color="#888" />
      <Text style={styles.metricValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginHorizontal: 12,
    marginVertical: 6,
    borderLeftWidth: 4,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1,
    shadowRadius: 3,
    elevation: 2,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  name: { fontSize: 16, fontWeight: '700', color: '#222', flex: 1 },
  badge: {
    width: 18, height: 18, borderRadius: 9,
    justifyContent: 'center', alignItems: 'center',
  },
  stateRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  stateDot: { width: 8, height: 8, borderRadius: 4, marginRight: 6 },
  state: { fontSize: 13, color: '#555', fontWeight: '500' },
  metrics: {
    flexDirection: 'row', justifyContent: 'space-between', marginBottom: 8,
  },
  metric: { flexDirection: 'row', alignItems: 'center' },
  metricValue: { fontSize: 12, color: '#666', marginLeft: 3 },
  footer: {
    flexDirection: 'row', justifyContent: 'space-between',
    borderTopWidth: 0.5, borderTopColor: '#eee', paddingTop: 6,
  },
  lastSeen: { fontSize: 11, color: '#999' },
  battery: { fontSize: 11, color: '#999' },
});