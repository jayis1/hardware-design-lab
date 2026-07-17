/**
 * DashboardScreen.tsx — paired-device list with status cards.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { useState, useEffect, useCallback } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, RefreshControl } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { useDevices } from '../context/DeviceContext';
import { StridoClient, DeviceInfo, StridoEvent, speciesName } from '../api';

interface CardData {
  id: string; name: string; ip: string;
  info?: DeviceInfo; topSpecies?: string; lastEventT?: number; eventCount24h?: number;
}

export default function DashboardScreen() {
  const { devices, active, setActive } = useDevices();
  const nav = useNavigation<any>();
  const [cards, setCards] = useState<CardData[]>([]);
  const [refreshing, setRefreshing] = useState(false);

  const refresh = useCallback(async () => {
    setRefreshing(true);
    const out: CardData[] = [];
    for (const d of devices) {
      const c = new StridoClient(d.ip);
      let info: DeviceInfo | undefined;
      let topSpecies: string | undefined;
      let lastT: number | undefined;
      let count = 0;
      try {
        info = await c.getInfo();
        const evs = await c.getEvents(0, 50);
        if (evs.length) {
          lastT = evs[evs.length - 1].t;
          const cutoff = Date.now() / 1000 - 24*3600;
          count = evs.filter(e => e.t > cutoff).length;
          /* Top species by count. */
          const tally: Record<number, number> = {};
          evs.forEach(e => { tally[e.sp] = (tally[e.sp]||0)+1; });
          let top = -1, topN = 0;
          for (const k in tally) if (tally[k] > topN) { topN = tally[k]; top = +k; }
          if (top >= 0) topSpecies = speciesName(top);
        }
      } catch { /* device offline */ }
      out.push({ id: d.id, name: d.name, ip: d.ip, info,
                 topSpecies, lastEventT: lastT, eventCount24h: count });
    }
    setCards(out);
    setRefreshing(false);
  }, [devices]);

  useEffect(() => { refresh(); }, [refresh]);

  const renderItem = ({ item }: { item: CardData }) => (
    <TouchableOpacity
      style={styles.card}
      onPress={() => {
        const dev = devices.find(d => d.id === item.id)!;
        setActive(dev);
        nav.navigate('LiveView', { deviceId: dev.id });
      }}
    >
      <View style={styles.cardHeader}>
        <Text style={styles.cardTitle}>{item.name}</Text>
        <View style={[styles.dot, { backgroundColor: item.info?.wifiUp ? '#39d353' : '#888' }]} />
      </View>
      <Text style={styles.cardSub}>{item.ip}  ·  fw {item.info?.fwVersion ?? '?'}</Text>
      <View style={styles.row}>
        <Text style={styles.metric}>🔋 {item.info?.battery ?? '?'}%</Text>
        <Text style={styles.metric}>↑ {item.info?.uptime ?? '?'}s</Text>
        <Text style={styles.metric}>24h: {item.eventCount24h ?? 0}</Text>
      </View>
      <Text style={styles.species}>
        Top: {item.topSpecies ?? '—'}
      </Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <FlatList
        data={cards}
        keyExtractor={(i) => i.id}
        renderItem={renderItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} tintColor="#39d353" />}
        ListEmptyComponent={<Text style={styles.empty}>No devices paired yet. Open Settings to add a Stridophone.</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 12 },
  card: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginBottom: 10,
          borderWidth: 1, borderColor: '#2a2a3e' },
  cardHeader: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  cardTitle: { color: '#e0e0e0', fontSize: 17, fontWeight: '600' },
  dot: { width: 10, height: 10, borderRadius: 5 },
  cardSub: { color: '#888', fontSize: 12, marginTop: 4 },
  row: { flexDirection: 'row', gap: 16, marginTop: 8 },
  metric: { color: '#c0c0d0', fontSize: 13 },
  species: { color: '#39d353', fontSize: 14, marginTop: 8, fontStyle: 'italic' },
  empty: { color: '#888', textAlign: 'center', marginTop: 40 },
});