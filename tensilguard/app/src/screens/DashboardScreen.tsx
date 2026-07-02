/**
 * DashboardScreen — overview of all TensilGuard nodes on the structure
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Shows a colour-coded list of every monitored cable, with the two tension
 * estimates (T_mag and T_vib), their disagreement, battery, and AE count.
 * Tapping a row navigates to NodeDetail.
 */
import React, { useEffect, useState } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, RefreshControl } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { useStore } from '../store';
import type { RootStackParamList } from '../../App';
import { FLAG_DT_ALARM, FLAG_AE_ALARM, FLAG_LOW_BATT } from '../proto';

type Nav = NativeStackNavigationProp<RootStackParamList, 'Dashboard'>;

function statusColour(flags: number, dtKn: number, batteryPct: number): string {
  if (flags & FLAG_AE_ALARM) return '#d32f2f';       // red — wire break
  if (flags & FLAG_DT_ALARM || dtKn > 5) return '#f57c00'; // amber — disagreement
  if (flags & FLAG_LOW_BATT || batteryPct < 15) return '#f57c00';
  return '#388e3c';                                  // green — OK
}

export default function DashboardScreen() {
  const { state, refreshNodes } = useStore();
  const navigation = useNavigation<Nav>();
  const [refreshing, setRefreshing] = useState(false);

  const onRefresh = async () => {
    setRefreshing(true);
    await refreshNodes();
    setRefreshing(false);
  };

  useEffect(() => { refreshNodes(); }, [refreshNodes]);

  const renderItem = ({ item }: { item: typeof state.nodes[0] }) => {
    const colour = statusColour(item.flags, item.dtKn, item.batteryPct);
    return (
      <TouchableOpacity
        style={[styles.row, { borderLeftColor: colour }]}
        onPress={() => navigation.navigate('NodeDetail', { nodeId: item.nodeId })}
      >
        <View style={styles.rowLeft}>
          <Text style={styles.label}>{item.label}</Text>
          <Text style={styles.sublabel}>{item.cableId} · {item.nodeId}</Text>
          <Text style={styles.sublabel}>
            T_mag {item.tMagKn.toFixed(1)} kN · T_vib {item.tVibKn.toFixed(1)} kN · ΔT {item.dtKn.toFixed(1)} kN
          </Text>
          <Text style={styles.sublabel}>
            f₁ {item.f1Hz.toFixed(2)} Hz · {item.tempC.toFixed(1)}°C · AE {item.aeCount}
          </Text>
        </View>
        <View style={styles.rowRight}>
          <View style={[styles.dot, { backgroundColor: colour }]} />
          <Text style={styles.batt}>{item.batteryPct.toFixed(0)}%</Text>
          <Text style={styles.rssi}>{item.rssiDb} dBm</Text>
        </View>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Structure Overview</Text>
        <Text style={styles.headerSub}>
          {state.nodes.length} nodes · {state.liveConnected ? '● live' : '○ offline'}
        </Text>
      </View>
      <FlatList
        data={state.nodes}
        keyExtractor={(n) => n.nodeId}
        renderItem={renderItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
        ListEmptyComponent={<Text style={styles.empty}>No nodes found. Is the gateway reachable?</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a' },
  header: { padding: 16, borderBottomWidth: 1, borderBottomColor: '#1a2a3a' },
  headerTitle: { color: '#e8f0f8', fontSize: 18, fontWeight: 'bold' },
  headerSub: { color: '#8a9ab0', fontSize: 13, marginTop: 4 },
  row: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    backgroundColor: '#142535', marginHorizontal: 12, marginVertical: 4,
    padding: 12, borderRadius: 8, borderLeftWidth: 4,
  },
  rowLeft: { flex: 1 },
  rowRight: { alignItems: 'center', minWidth: 60 },
  label: { color: '#e8f0f8', fontSize: 15, fontWeight: '600' },
  sublabel: { color: '#8a9ab0', fontSize: 12, marginTop: 2 },
  dot: { width: 12, height: 12, borderRadius: 6, marginBottom: 4 },
  batt: { color: '#e8f0f8', fontSize: 11 },
  rssi: { color: '#8a9ab0', fontSize: 10, marginTop: 2 },
  empty: { color: '#8a9ab0', textAlign: 'center', marginTop: 40, fontSize: 14 },
});