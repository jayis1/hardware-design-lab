/**
 * DashboardScreen — site overview, per-probe cards
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useEffect } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, RefreshControl } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { useStore, riskScoreForProbe } from '../store';
import { RootStackParamList } from '../../App';

type Nav = NativeStackNavigationProp<RootStackParamList, 'Dashboard'>;

function riskColor(s: number) {
  if (s >= 70) return '#d32f2f';
  if (s >= 35) return '#fbc02d';
  return '#388e3c';
}

export default function DashboardScreen() {
  const { probes, refresh } = useStore();
  const nav = useNavigation<Nav>();
  const [refreshing, setRefreshing] = React.useState(false);

  useEffect(() => { refresh(); }, []);

  const onRefresh = async () => {
    setRefreshing(true);
    await refresh();
    setRefreshing(false);
  };

  if (probes.length === 0) {
    return (
      <View style={s.empty}>
        <Text style={s.emptyText}>No probes registered yet.</Text>
        <Text style={s.emptySub}>Pair a probe from the gateway, then pull to refresh.</Text>
      </View>
    );
  }

  return (
    <FlatList
      style={s.list}
      data={probes}
      keyExtractor={(p) => p.id}
      refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
      renderItem={({ item }) => {
        const score = riskScoreForProbe(item.id);
        return (
          <TouchableOpacity
            style={s.card}
            onPress={() => nav.navigate('ProbeDetail', { probeId: item.id })}
          >
            <View style={s.cardHeader}>
              <Text style={s.cardTitle}>{item.name}</Text>
              <View style={[s.riskPill, { backgroundColor: riskColor(score) }]}>
                <Text style={s.riskPillText}>{score}</Text>
              </View>
            </View>
            <Text style={s.cardSub}>{item.site}</Text>
            <View style={s.cardRow}>
              <Text style={s.cardMetric}>Batt {(item.batteryMv / 1000).toFixed(2)} V</Text>
              <Text style={s.cardMetric}>Seq {item.lastSeq}</Text>
              {item.urgent ? <Text style={s.urgentTag}>URGENT</Text> : null}
            </View>
          </TouchableOpacity>
        );
      }}
    />
  );
}

const s = StyleSheet.create({
  list: { flex: 1, backgroundColor: '#0d1f17' },
  empty: { flex: 1, alignItems: 'center', justifyContent: 'center', backgroundColor: '#0d1f17' },
  emptyText: { color: '#e8f5e9', fontSize: 18, marginBottom: 8 },
  emptySub: { color: '#9ccc9c', fontSize: 13 },
  card: {
    margin: 12, padding: 16, borderRadius: 12, backgroundColor: '#1a3a2a',
    shadowColor: '#000', shadowOpacity: 0.3, shadowRadius: 6, elevation: 4,
  },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  cardTitle: { color: '#e8f5e9', fontSize: 18, fontWeight: 'bold' },
  cardSub: { color: '#9ccc9c', fontSize: 12, marginTop: 2 },
  cardRow: { flexDirection: 'row', marginTop: 12, gap: 12, alignItems: 'center' },
  cardMetric: { color: '#c8e6c9', fontSize: 12 },
  urgentTag: { color: '#fff', backgroundColor: '#d32f2f', paddingHorizontal: 6, paddingVertical: 2, borderRadius: 4, fontSize: 11, fontWeight: 'bold' },
  riskPill: { width: 36, height: 36, borderRadius: 18, alignItems: 'center', justifyContent: 'center' },
  riskPillText: { color: '#fff', fontWeight: 'bold', fontSize: 14 },
});