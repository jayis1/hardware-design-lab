/*
 * src/screens/Dashboard.tsx — Fleet dashboard for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useEffect, useState, useCallback } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, RefreshControl } from 'react-native';
import { Card, IconButton, Badge, ActivityIndicator } from 'react-native-paper';
import { useNavigation } from '@react-navigation/native';
import { StackNavigationProp } from '@react-navigation/stack';
import { DewarReading, STATE_COLOR, RootStackParamList } from '../types';
import { getFleet, openAlarmStream } from '../cloud/Api';

type Nav = StackNavigationProp<RootStackParamList, 'Dashboard'>;

export default function Dashboard() {
  const [fleet, setFleet] = useState<DewarReading[]>([]);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [alarmCount, setAlarmCount] = useState(0);
  const nav = useNavigation<Nav>();

  const load = useCallback(async () => {
    try {
      const f = await getFleet();
      setFleet(f);
      setAlarmCount(f.filter((d) => d.state === 'WARN' || d.state === 'CRITICAL').length);
    } catch (e) {
      console.warn('fleet load failed', e);
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    load();
    const close = openAlarmStream((_id, _state, _reason) => {
      setAlarmCount((c) => c + 1);
      load();   // refresh on any alarm event
    });
    return close;
  }, [load]);

  const onRefresh = () => { setRefreshing(true); load(); };

  if (loading) {
    return (
      <View style={s.center}><ActivityIndicator size="large" />
        <Text style={s.loadingText}>Loading fleet…</Text></View>
    );
  }

  return (
    <View style={s.wrap}>
      <View style={s.headerRow}>
        <Text style={s.h1}>{fleet.length} Dewars monitored</Text>
        <TouchableOpacity onPress={() => nav.navigate('AlertInbox')}>
          <Badge visible={alarmCount > 0} size={28} style={s.badge}>
            {alarmCount}
          </Badge>
          <IconButton icon="bell-outline" size={28} color="#F1F5F9" />
        </TouchableOpacity>
      </View>

      <FlatList
        data={fleet}
        keyExtractor={(d) => d.dewarId}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={onRefresh} />}
        renderItem={({ item }) => <DewarCard d={item} onPress={() =>
          nav.navigate('DewarDetail', { dewarId: item.dewarId })} />}
      />

      <TouchableOpacity style={s.fab} onPress={() => nav.navigate('Commissioning')}>
        <IconButton icon="plus" size={28} color="#F1F5F9" />
        <Text style={s.fabText}>Commission</Text>
      </TouchableOpacity>
    </View>
  );
}

function DewarCard({ d, onPress }: { d: DewarReading; onPress: () => void }) {
  const color = STATE_COLOR[d.state];
  const ago = Math.round((Date.now() / 1000 - d.lastHeartbeatEpoch) / 60);
  return (
    <TouchableOpacity onPress={onPress}>
      <Card style={[s.card, { borderLeftColor: color, borderLeftWidth: 6 }]}>
        <Card.Title
          title={d.label || d.dewarId}
          subtitle={`${d.serial}  •  last seen ${ago} min ago`}
          left={(props) => <IconButton {...props} icon="thermometer" color={color} />}
          right={(props) => (
            <View style={s.row}>
              <Text style={[s.stateLabel, { color }]}>{d.state}</Text>
              <IconButton {...props} icon="chevron-right" />
            </View>
          )}
        />
        <Card.Content>
          <View style={s.metricRow}>
            <Metric label="LN2" value={`${d.levelPct.toFixed(1)}%`} warn={d.levelPct < 35} />
            <Metric label="Rate" value={`${d.levelRatePerHour.toFixed(2)}%/h`} warn={d.levelRatePerHour > 1} />
            <Metric label="Tilt" value={`${d.tiltDeg.toFixed(1)}°`} warn={d.tiltDeg > 8} />
            <Metric label="Batt" value={`${d.battPct.toFixed(0)}%`} warn={d.battPct < 20} />
          </View>
          {d.reason !== 'none' && (
            <Text style={[s.reason, { color }]}>⚠ {d.reason.replace(/_/g, ' ')}</Text>
          )}
          {!d.mainsPresent && <Text style={s.mainsLost}>⚡ mains lost — on battery</Text>}
          {d.lidOpen && <Text style={s.lidOpen}>🚪 lid open</Text>}
        </Card.Content>
      </Card>
    </TouchableOpacity>
  );
}

function Metric({ label, value, warn }: { label: string; value: string; warn?: boolean }) {
  return (
    <View style={s.metric}>
      <Text style={s.metricLabel}>{label}</Text>
      <Text style={[s.metricValue, warn && s.metricWarn]}>{value}</Text>
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 12 },
  center: { flex: 1, backgroundColor: '#0F172A', alignItems: 'center', justifyContent: 'center' },
  loadingText: { color: '#94A3B8', marginTop: 8 },
  headerRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 },
  h1: { color: '#F1F5F9', fontSize: 18, fontWeight: 'bold' },
  badge: { position: 'absolute', top: -4, right: 8, zIndex: 2, backgroundColor: '#DC2626' },
  row: { flexDirection: 'row', alignItems: 'center' },
  card: { backgroundColor: '#1E293B', marginBottom: 10, borderRadius: 8 },
  stateLabel: { fontWeight: 'bold', fontSize: 14, letterSpacing: 1 },
  metricRow: { flexDirection: 'row', justifyContent: 'space-between', marginBottom: 6 },
  metric: { alignItems: 'center', flex: 1 },
  metricLabel: { color: '#94A3B8', fontSize: 11, textTransform: 'uppercase' },
  metricValue: { color: '#F1F5F9', fontSize: 16, fontWeight: '600' },
  metricWarn: { color: '#EA580C' },
  reason: { marginTop: 4, fontSize: 13, fontWeight: '600' },
  mainsLost: { color: '#EAB308', fontSize: 12, marginTop: 4 },
  lidOpen: { color: '#EA580C', fontSize: 12, marginTop: 2 },
  fab: { position: 'absolute', bottom: 20, right: 20, backgroundColor: '#0066CC',
         borderRadius: 30, flexDirection: 'row', alignItems: 'center',
         paddingHorizontal: 12, elevation: 4 },
  fabText: { color: '#F1F5F9', fontWeight: 'bold' },
});