/*
 * screens/NodeDetail.tsx — per-node detail & per-channel gape trace
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import React, { useEffect, useState } from 'react';
import {
  View, Text, ScrollView, StyleSheet, TouchableOpacity, Alert,
} from 'react-native';
import { LineChart } from 'react-native-chart-kit';

import { MusselWatchClient } from '../api';
import {
  AppConfig, Node, ChannelState, anomalyLabel, batteryPct,
  formatTemp, formatUptime, chargerStateLabel, alertLevelFromScore,
} from '../types';

export function NodeDetail({ route, config }: { route: any; config: AppConfig }) {
  const node: Node = route.params.node;
  const client = new MusselWatchClient(config);
  const [history, setHistory] = useState<number[]>([]);
  const [selected, setSelected] = useState<number>(0);

  useEffect(() => {
    let mounted = true;
    (async () => {
      const h = await client.getChannelHistory(node.nodeId, selected, 4);
      if (mounted) setHistory(h);
    })();
    return () => { mounted = false; };
  }, [node.nodeId, selected, config]);

  const t = node.telemetry;
  const ch = node.channels[selected];

  const doCalibrate = () => {
    Alert.alert(
      'Calibrate Baseline',
      'Hold all mussel shells closed on this node, then send the calibration command. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Calibrate', onPress: async () => {
          await client.requestCalibration(node.nodeId);
          Alert.alert('Sent', 'Calibration command sent. Baseline will update within one sample cycle.');
        }},
      ],
    );
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>{node.label}</Text>
        <Text style={styles.sub}>{node.river} · {node.reach}</Text>
        <Text style={styles.sub}>Node ID: {node.nodeId} · Species: {node.species}</Text>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Node telemetry</Text>
        <View style={styles.grid}>
          <Cell label="Battery" value={t ? `${batteryPct(t.batteryMv)}% (${t.batteryMv} mV)` : '—'} />
          <Cell label="Solar" value={t ? `${t.solarMv} mV` : '—'} />
          <Cell label="Water temp" value={t ? formatTemp(t.waterTempC10, config.temperatureUnitC) : '—'} />
          <Cell label="Charger" value={t ? chargerStateLabel(t.chargerState) : '—'} />
          <Cell label="Uptime" value={t ? formatUptime(t.uptimeS) : '—'} />
          <Cell label="Last seen" value={`${node.lastSeenS}s ago`} />
          <Cell label="Seq #" value={t ? String(t.seq) : '—'} />
          <Cell label="Max anomaly" value={t ? String(t.maxAnomaly) : '—'} />
        </View>
        <TouchableOpacity style={styles.calibBtn} onPress={doCalibrate}>
          <Text style={styles.calibBtnText}>Calibrate Baseline (shells closed)</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Channels</Text>
        <View style={styles.channelRow}>
          {node.channels.map((c) => {
            const level = alertLevelFromScore(c.anomalyScore);
            const bg = level === 'critical' ? '#b00020' :
                       level === 'warning' ? '#c2410c' :
                       level === 'advisory' ? '#b45309' : '#1c4a66';
            return (
              <TouchableOpacity
                key={c.channel}
                onPress={() => setSelected(c.channel)}
                style={[styles.channelPill, { backgroundColor: bg, borderWidth: selected === c.channel ? 2 : 0, borderColor: '#7fcfff' }]}
              >
                <Text style={styles.channelPillText}>{c.channel + 1}</Text>
              </TouchableOpacity>
            );
          })}
        </View>

        {ch && (
          <View style={styles.channelDetail}>
            <Text style={styles.chDetailTitle}>Channel {ch.channel + 1} — {ch.location}</Text>
            <Text style={styles.chDetailSub}>Species: {ch.species}</Text>
            <View style={styles.grid}>
              <Cell label="Gape" value={`${ch.gapeUm} µm`} />
              <Cell label="Activity" value={`${ch.activityScore}/100`} />
              <Cell label="Anomaly" value={anomalyLabel(ch.anomalyFlag)} />
              <Cell label="Score" value={`${ch.anomalyScore}/100`} />
            </View>
          </View>
        )}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Gape trace (last 4 hours, {selected + 1})</Text>
        {history.length > 0 && (
          <LineChart
            data={{
              labels: history.map((_, i) => (i % 30 === 0 ? `${i}m` : '')),
              datasets: [{ data: history, color: () => '#7fcfff', strokeWidth: 1 }],
            }}
            width={340}
            height={180}
            chartConfig={{
              backgroundColor: '#10334a',
              backgroundGradientFrom: '#10334a',
              backgroundGradientTo: '#0a1f2c',
              color: () => '#8aa8b8',
              labelColor: () => '#9fb8c7',
            }}
            bezier
            style={{ borderRadius: 8, marginTop: 8 }}
          />
        )}
      </View>
    </ScrollView>
  );
}

function Cell({ label, value }: { label: string; value: string }) {
  return (
    <View style={styles.cell}>
      <Text style={styles.cellLabel}>{label}</Text>
      <Text style={styles.cellValue}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f2c' },
  header: { padding: 16, backgroundColor: '#0a4d6e' },
  title: { color: '#e0f0f5', fontSize: 20, fontWeight: '700' },
  sub: { color: '#a0c5d5', fontSize: 12, marginTop: 2 },
  section: { padding: 14, borderBottomWidth: 1, borderBottomColor: '#1c4a66' },
  sectionTitle: { color: '#7fcfff', fontSize: 14, fontWeight: '600', marginBottom: 10 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  cell: { width: '48%', paddingVertical: 6 },
  cellLabel: { color: '#8aa8b8', fontSize: 11 },
  cellValue: { color: '#d0e8f2', fontSize: 14, fontWeight: '600', marginTop: 2 },
  calibBtn: {
    marginTop: 12, padding: 10, borderRadius: 6, backgroundColor: '#0a4d6e', alignItems: 'center',
  },
  calibBtnText: { color: '#e0f0f5', fontSize: 13, fontWeight: '600' },
  channelRow: { flexDirection: 'row', flexWrap: 'wrap', gap: 6 },
  channelPill: { width: 34, height: 34, borderRadius: 17, justifyContent: 'center', alignItems: 'center' },
  channelPillText: { color: '#fff', fontSize: 13, fontWeight: '700' },
  channelDetail: { marginTop: 14 },
  chDetailTitle: { color: '#e0f0f5', fontSize: 14, fontWeight: '600' },
  chDetailSub: { color: '#9fb8c7', fontSize: 12, marginTop: 2, marginBottom: 8 },
});