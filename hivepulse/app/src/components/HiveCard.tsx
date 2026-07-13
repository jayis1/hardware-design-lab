/**
 * HiveCard — Individual hive summary card
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { HiveData, ColonyState } from '../types';
import Sparkline from './Sparkline';
import StateBadge from './StateBadge';

export default function HiveCard({ hive, onPress }: {
  hive: HiveData;
  onPress: () => void;
}) {
  const batteryPct = ((hive.battery - 2600) / (3300 - 2600)) * 100;
  const weightTrend = [hive.weight - 0.5, hive.weight - 0.3, hive.weight - 0.1,
                       hive.weight, hive.weight + 0.2]; // Demo trend

  return (
    <TouchableOpacity style={styles.card} onPress={onPress} activeOpacity={0.7}>
      <View style={styles.header}>
        <Text style={styles.name}>{hive.name}</Text>
        <StateBadge state={hive.colonyState} confidence={hive.confidence} />
      </View>

      <View style={styles.body}>
        <View style={styles.leftCol}>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Weight</Text>
            <Text style={styles.statValue}>{hive.weight.toFixed(1)} kg</Text>
          </View>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Temp</Text>
            <Text style={styles.statValue}>
              {hive.temperatures[0]?.toFixed(1) ?? '--'}°C
            </Text>
          </View>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>CO₂</Text>
            <Text style={styles.statValue}>{hive.co2} ppm</Text>
          </View>
          <View style={styles.statRow}>
            <Text style={styles.statLabel}>Traffic</Text>
            <Text style={styles.statValue}>
              ↗{hive.beesIn} ↘{hive.beesOut}
            </Text>
          </View>
        </View>

        <View style={styles.rightCol}>
          <Sparkline data={weightTrend} width={80} height={40} />
          <Text style={styles.sparkLabel}>Weight trend (5d)</Text>
          <View style={styles.powerRow}>
            <Text style={[
              styles.battery,
              { color: batteryPct > 50 ? '#4CAF50' : batteryPct > 20 ? '#FFC107' : '#DC3545' },
            ]}>
              {batteryPct > 0 ? `🔋 ${batteryPct.toFixed(0)}%` : '🔋 --'}
            </Text>
            {hive.solar > 100 && <Text style={styles.solar}>☀️</Text>}
          </View>
        </View>
      </View>

      <Text style={styles.updated}>
        Updated {new Date(hive.lastUpdated * 1000).toLocaleString()}
      </Text>
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1A1A2E',
    borderRadius: 12,
    padding: 14,
    marginBottom: 10,
  },
  header: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  name: { fontSize: 18, fontWeight: 'bold', color: '#FFF' },
  body: { flexDirection: 'row', marginTop: 10 },
  leftCol: { flex: 1 },
  rightCol: { alignItems: 'center' },
  statRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 4 },
  statLabel: { fontSize: 13, color: '#888' },
  statValue: { fontSize: 13, color: '#DDD', fontWeight: '600' },
  sparkLabel: { fontSize: 10, color: '#555', marginTop: 2 },
  powerRow: { flexDirection: 'row', alignItems: 'center', marginTop: 6 },
  battery: { fontSize: 12, fontWeight: '600' },
  solar: { fontSize: 14, marginLeft: 4 },
  updated: { fontSize: 11, color: '#555', marginTop: 8 },
});