/**
 * SpeciesDetailScreen.tsx — per-species card with reference + audio player.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, Image, TouchableOpacity, ActivityIndicator } from 'react-native';
import { useRoute, useNavigation } from '@react-navigation/native';
import { useDevices } from '../context/DeviceContext';
import { StridoClient, StridoEvent, speciesName } from '../api';
import { Audio } from 'expo-av';

const REFERENCE: Record<number, { habitat: string; call: string; threat: string }> = {
  8:  { habitat: 'Subterranean termite — structural timber, soil contact.',
        call: 'Low-amplitude impulsive head-banging at ~5-10 Hz bursts; soldier tremulation.',
        threat: 'Severe structural damage; urgent treatment advised.' },
  11: { habitat: 'Old-house borer larva in seasoned softwood structural timbers.',
        call: 'Rasping larval feeding clicks in the 200-800 Hz band, ~1-2/s.',
        threat: 'Compromises load-bearing beams; monitor population growth rate.' },
  13: { habitat: 'Granary weevil — stored whole grain, silos, bins.',
        call: 'High-rate stridulation near grain surface, 5-12 kHz peak.',
        threat: 'Direct commodity loss; fumigate when density exceeds 1 insect/kg.' },
  15: { habitat: 'Red flour beetle — stored flour, processed grain.',
        call: 'Continuous low-level clicks 3-8 kHz, aggregated in hotspots.',
        threat: 'Rapid population growth in warm grain; cool/aerate grain immediately.' },
  23: { habitat: 'Webbing clothes moth — wool, silk, feathers in dark storage.',
        call: 'Faint wing-fanning clicks 6-10 kHz during mating.',
        threat: 'Damages textiles/collections; vacuum + freeze infested items.' },
};

export default function SpeciesDetailScreen() {
  const route = useRoute<any>();
  const nav = useNavigation<any>();
  const { client } = useDevices();
  const [events, setEvents] = useState<StridoEvent[]>([]);
  const [loading, setLoading] = useState(true);
  const [sound, setSound] = useState<Audio.Sound | null>(null);

  const speciesId = route.params?.speciesId ?? 0;
  const name = speciesName(speciesId);
  const ref = REFERENCE[speciesId];

  useEffect(() => {
    if (!client) return;
    client.getEvents(0, 200).then((all) => {
      setEvents(all.filter(e => e.sp === speciesId));
      setLoading(false);
    }).catch(() => setLoading(false));
    return () => { sound?.unloadAsync(); };
  }, [client, speciesId]);

  const playLatest = async () => {
    if (!client || events.length === 0) return;
    const latest = events[events.length - 1];
    const url = await client.getWavUrl(latest.id);
    const { sound: s } = await Audio.Sound.createAsync({ uri: url });
    setSound(s);
    await s.playAsync();
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>{name}</Text>
      {ref && (
        <View style={styles.card}>
          <Text style={styles.label}>Habitat</Text><Text style={styles.body}>{ref.habitat}</Text>
          <Text style={styles.label}>Call signature</Text><Text style={styles.body}>{ref.call}</Text>
          <Text style={styles.label}>Threat assessment</Text><Text style={styles.body}>{ref.threat}</Text>
        </View>
      )}
      <Text style={styles.section}>Observed ({events.length})</Text>
      {loading ? <ActivityIndicator color="#39d353" /> :
        events.slice(-10).reverse().map((e) => (
          <View key={e.id} style={styles.eventRow}>
            <Text style={styles.eventT}>#{e.id} · conf {e.conf}% · az {e.az} · {e.T?.toFixed(1)}°C / {e.RH?.toFixed(0)}%</Text>
          </View>
        ))
      }
      <TouchableOpacity style={styles.playBtn} onPress={playLatest}>
        <Text style={styles.playBtnText}>▶ Play latest WAV snippet</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 16 },
  h1: { color: '#39d353', fontSize: 22, fontWeight: 'bold', fontStyle: 'italic' },
  card: { backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginTop: 12 },
  label: { color: '#aaa', fontSize: 12, marginTop: 8 },
  body: { color: '#e0e0e0', fontSize: 14 },
  section: { color: '#aaa', fontSize: 14, marginTop: 18, marginBottom: 6 },
  eventRow: { backgroundColor: '#1a1a2e', borderRadius: 6, padding: 8, marginBottom: 4 },
  eventT: { color: '#c0c0d0', fontSize: 12 },
  playBtn: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 14, marginTop: 18, alignItems: 'center' },
  playBtnText: { color: '#39d353', fontSize: 15, fontWeight: '600' },
});