/**
 * AlertsScreen — push-notification feed of urgent uplinks
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { useStore } from '../store';

function severityColour(s: string): string {
  if (s === 'critical') return '#d32f2f';
  if (s === 'warning') return '#f57c00';
  return '#4fc3f7';
}

export default function AlertsScreen() {
  const { state, clearAlerts } = useStore();

  const renderItem = ({ item }: { item: typeof state.alerts[0] }) => {
    const c = severityColour(item.severity);
    return (
      <View style={[styles.row, { borderLeftColor: c }]}>
        <View style={styles.rowLeft}>
          <Text style={styles.time}>{new Date(item.unixTime * 1000).toLocaleString()}</Text>
          <Text style={[styles.msg, { color: c }]}>{item.message}</Text>
          <Text style={styles.node}>Node: {item.nodeId}</Text>
        </View>
        <Text style={[styles.badge, { color: c, borderColor: c }]}>{item.severity}</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Alerts</Text>
        <TouchableOpacity onPress={clearAlerts}>
          <Text style={styles.clearBtn}>Clear</Text>
        </TouchableOpacity>
      </View>
      <FlatList
        data={state.alerts}
        keyExtractor={(a, i) => a.id ?? `alert-${i}`}
        renderItem={renderItem}
        ListEmptyComponent={<Text style={styles.empty}>No alerts. All quiet.</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a' },
  header: { flexDirection: 'row', justifyContent: 'space-between', padding: 16, borderBottomWidth: 1, borderBottomColor: '#1a2a3a' },
  title: { color: '#e8f0f8', fontSize: 18, fontWeight: 'bold' },
  clearBtn: { color: '#4fc3f7', fontSize: 14 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', backgroundColor: '#142535', marginHorizontal: 12, marginVertical: 4, padding: 12, borderRadius: 8, borderLeftWidth: 4 },
  rowLeft: { flex: 1 },
  time: { color: '#8a9ab0', fontSize: 11 },
  msg: { fontSize: 14, fontWeight: '500', marginVertical: 4 },
  node: { color: '#6a8aa0', fontSize: 11 },
  badge: { fontSize: 10, borderWidth: 1, borderRadius: 4, padding: 4, textTransform: 'capitalize' },
  empty: { color: '#8a9ab0', textAlign: 'center', marginTop: 40, fontSize: 14 },
});