/**
 * LiveViewScreen.tsx — live spectrogram, DOA plot, species roster.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { useState, useEffect, useContext } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { useRoute } from '@react-navigation/native';
import { useDevices } from '../context/DeviceContext';
import { StridoClient, SpectrogramRow, StridoEvent, speciesName } from '../api';
import SpectrogramView from '../components/SpectrogramView';
import DoaPolarPlot from '../components/DoaPolarPlot';

export default function LiveViewScreen() {
  const route = useRoute<any>();
  const { devices, client } = useDevices();
  const [rows, setRows] = useState<SpectrogramRow[]>([]);
  const [events, setEvents] = useState<StridoEvent[]>([]);
  const [doaHist, setDoaHist] = useState<number[]>(new Array(16).fill(0));
  const [info, setInfo] = useState<any>(null);

  useEffect(() => {
    if (!client) return;
    client.getInfo().then(setInfo).catch(() => {});
    client.openStream(
      (e) => {
        setEvents((prev) => [...prev.slice(-40), e]);
        setDoaHist((prev) => {
          const next = [...prev];
          next[e.az % 16] = (next[e.az % 16] || 0) + 1;
          return next;
        });
      },
      (s) => setRows((prev) => [...prev.slice(-200), s])
    );
    return () => client.closeStream();
  }, [client]);

  const recent = events.slice(-8);

  return (
    <ScrollView style={styles.container}>
      <View style={styles.headerRow}>
        <Text style={styles.h1}>{info?.name ?? 'Live'}</Text>
        <Text style={styles.battery}>🔋 {info?.battery ?? '?'}%</Text>
      </View>

      <Text style={styles.section}>Spectrogram</Text>
      <SpectrogramView rows={rows} />

      <Text style={styles.section}>Direction of arrival</Text>
      <DoaPolarPlot hist={doaHist} />

      <Text style={styles.section}>Recent detections</Text>
      {recent.map((e) => (
        <View key={e.id} style={styles.eventRow}>
          <Text style={styles.eventSpecies}>{e.species ?? speciesName(e.sp)}</Text>
          <Text style={styles.eventMeta}>conf {e.conf}% · az {e.az} · T {e.T?.toFixed(1)}°C</Text>
        </View>
      ))}
      {recent.length === 0 && <Text style={styles.empty}>Listening…</Text>}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 12 },
  headerRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  h1: { color: '#e0e0e0', fontSize: 22, fontWeight: 'bold' },
  battery: { color: '#39d353', fontSize: 14 },
  section: { color: '#aaa', fontSize: 14, marginTop: 18, marginBottom: 6, fontWeight: '600' },
  eventRow: { backgroundColor: '#1a1a2e', borderRadius: 8, padding: 10, marginBottom: 6 },
  eventSpecies: { color: '#39d353', fontSize: 15, fontStyle: 'italic' },
  eventMeta: { color: '#888', fontSize: 12, marginTop: 2 },
  empty: { color: '#888', textAlign: 'center', marginTop: 20 },
});