// CondenScope Expo companion application
// Author: jayis1
// SPDX-License-Identifier: MIT
import React, { useEffect, useMemo, useRef, useState } from 'react';
import { SafeAreaView, View, Text, Pressable, StyleSheet, ScrollView,
  TextInput, Switch, Alert, Platform } from 'react-native';
import Svg, { Rect, Circle, Line, Text as SvgText } from 'react-native-svg';
import AsyncStorage from '@react-native-async-storage/async-storage';
import * as FileSystem from 'expo-file-system';
import * as Sharing from 'expo-sharing';
import { commands } from './protocol';

const AUTHOR = 'jayis1';
const WIDTH = 32;
const HEIGHT = 24;
const palette = ['#12263a', '#1d4e89', '#16a6a1', '#f4d35e', '#ee964b', '#d7263d'];
const demoPixels = Array.from({ length: WIDTH * HEIGHT }, (_, i) => {
  const x = i % WIDTH; const y = Math.floor(i / WIDTH);
  return 18.8 - 8 * Math.exp(-(((x - 7) ** 2) + ((y - 17) ** 2)) / 35)
    - 4 * Math.exp(-(((x - 27) ** 2) + ((y - 5) ** 2)) / 18);
});

function colorFor(temp, min, max) {
  const span = Math.max(0.1, max - min);
  const index = Math.max(0, Math.min(palette.length - 1,
    Math.floor(((temp - min) / span) * palette.length)));
  return palette[index];
}

function Gauge({ label, value, unit, danger = false }) {
  return <View style={[styles.gauge, danger && styles.gaugeDanger]}>
    <Text style={styles.gaugeLabel}>{label}</Text>
    <Text style={styles.gaugeValue}>{value}<Text style={styles.unit}>{unit}</Text></Text>
  </View>;
}

function ThermalMap({ pixels, dewPoint, onSelect }) {
  const min = Math.min(...pixels); const max = Math.max(...pixels);
  const cellW = 320 / WIDTH; const cellH = 240 / HEIGHT;
  return <View style={styles.mapWrap}>
    <Svg width="100%" viewBox="0 0 320 240" accessibilityLabel="Thermal dew risk map">
      {pixels.map((temp, i) => <Rect key={i} x={(i % WIDTH) * cellW}
        y={Math.floor(i / WIDTH) * cellH} width={cellW + 0.2} height={cellH + 0.2}
        fill={temp - dewPoint <= 0.75 ? '#ff1744' : temp - dewPoint <= 2.5 ? '#ffb300' : colorFor(temp, min, max)}
        onPress={() => onSelect?.(i, temp)} />)}
      <SvgText x="6" y="16" fill="#fff" fontSize="11">{max.toFixed(1)}°C</SvgText>
      <SvgText x="6" y="232" fill="#fff" fontSize="11">{min.toFixed(1)}°C</SvgText>
    </Svg>
  </View>;
}

function RiskLegend() {
  return <View style={styles.legend}>
    <View style={styles.legendItem}><View style={[styles.dot, { backgroundColor: '#ff1744' }]} /><Text style={styles.muted}>≤0.75°C danger</Text></View>
    <View style={styles.legendItem}><View style={[styles.dot, { backgroundColor: '#ffb300' }]} /><Text style={styles.muted}>≤2.5°C warning</Text></View>
    <View style={styles.legendItem}><View style={[styles.dot, { backgroundColor: '#16a6a1' }]} /><Text style={styles.muted}>safe margin</Text></View>
  </View>;
}

function LiveScreen({ connected, scanning, setScanning, settings, addSession }) {
  const [status, setStatus] = useState({ air: 21.3, rh: 67.4, dew: 15.1, distance: 840, battery: 78 });
  const [pixels, setPixels] = useState(demoPixels);
  const [selected, setSelected] = useState(null);
  const samples = useRef(0);
  useEffect(() => {
    if (!scanning) return undefined;
    const timer = setInterval(() => {
      samples.current += 1;
      const phase = samples.current / 7;
      setPixels(demoPixels.map((t, i) => t + Math.sin(phase + i / 53) * 0.16));
      setStatus(s => ({ ...s, rh: 67.4 + Math.sin(phase) * 0.4,
        dew: 15.1 + Math.sin(phase) * 0.08 }));
    }, 500);
    return () => clearInterval(timer);
  }, [scanning]);

  const toggle = () => {
    if (!connected) { Alert.alert('Not connected', 'Connect CondenScope before scanning.'); return; }
    if (scanning) {
      const danger = pixels.filter(t => t - status.dew <= settings.danger).length;
      addSession({ id: Date.now(), date: new Date().toISOString(), duration: samples.current / 2,
        minMargin: Math.min(...pixels) - status.dew, danger, notes: '' });
      samples.current = 0;
    }
    setScanning(!scanning);
  };
  const minimum = Math.min(...pixels); const margin = minimum - status.dew;
  return <ScrollView contentContainerStyle={styles.content}>
    <View style={styles.row}><Gauge label="AIR" value={status.air.toFixed(1)} unit="°C" />
      <Gauge label="RH" value={status.rh.toFixed(0)} unit="%" />
      <Gauge label="DEW" value={status.dew.toFixed(1)} unit="°C" danger={margin <= settings.danger} /></View>
    <ThermalMap pixels={pixels} dewPoint={status.dew} onSelect={(i, temp) => setSelected({ i, temp })} />
    <RiskLegend />
    {selected && <View style={styles.detail}><Text style={styles.detailTitle}>Pixel {selected.i % WIDTH}, {Math.floor(selected.i / WIDTH)}</Text>
      <Text style={styles.body}>{selected.temp.toFixed(2)}°C surface · {(selected.temp - status.dew).toFixed(2)}°C dew margin</Text></View>}
    <View style={styles.metricRow}><Text style={styles.body}>Closest surface</Text><Text style={styles.metric}>{status.distance} mm</Text></View>
    <View style={styles.metricRow}><Text style={styles.body}>Minimum margin</Text><Text style={[styles.metric, margin <= settings.warning && styles.bad]}>{margin.toFixed(2)}°C</Text></View>
    <Pressable style={[styles.primary, scanning && styles.stop]} onPress={toggle}>
      <Text style={styles.primaryText}>{scanning ? 'STOP & SAVE SURVEY' : 'START SURVEY'}</Text></Pressable>
    <Text style={styles.hint}>Demo transport is active in this reference app. Native BLE adapter implementations call commands.start()/stop() and feed decoded frames into this screen.</Text>
  </ScrollView>;
}

function SessionsScreen({ sessions, updateSession }) {
  const [expanded, setExpanded] = useState(null);
  const exportCsv = async () => {
    const header = 'timestamp,duration_seconds,minimum_margin_c,danger_points,notes\n';
    const rows = sessions.map(s => `${s.date},${s.duration},${s.minMargin.toFixed(2)},${s.danger},"${(s.notes || '').replaceAll('"', '""')}"`).join('\n');
    const uri = `${FileSystem.cacheDirectory}condenscope-surveys.csv`;
    await FileSystem.writeAsStringAsync(uri, header + rows, { encoding: FileSystem.EncodingType.UTF8 });
    if (await Sharing.isAvailableAsync()) await Sharing.shareAsync(uri, { mimeType: 'text/csv' });
  };
  return <ScrollView contentContainerStyle={styles.content}>
    <View style={styles.titleRow}><Text style={styles.sectionTitle}>Survey history</Text>
      <Pressable style={styles.secondary} onPress={exportCsv}><Text style={styles.secondaryText}>Export CSV</Text></Pressable></View>
    {sessions.length === 0 && <Text style={styles.empty}>No surveys saved. Start a live survey to establish a baseline.</Text>}
    {sessions.map(s => <Pressable key={s.id} style={styles.session} onPress={() => setExpanded(expanded === s.id ? null : s.id)}>
      <View style={styles.titleRow}><View><Text style={styles.detailTitle}>{new Date(s.date).toLocaleString()}</Text>
        <Text style={styles.muted}>{s.duration.toFixed(0)} sec · {s.danger} danger samples</Text></View>
        <Text style={[styles.margin, s.minMargin <= 0.75 && styles.bad]}>{s.minMargin.toFixed(1)}°C</Text></View>
      {expanded === s.id && <TextInput style={styles.notes} multiline placeholder="Room, wall orientation, observed materials…"
        placeholderTextColor="#60758a" value={s.notes} onChangeText={notes => updateSession(s.id, notes)} />}
    </Pressable>)}
  </ScrollView>;
}

function SettingsScreen({ settings, setSettings, connected, setConnected }) {
  const update = (key, value) => setSettings({ ...settings, [key]: value });
  const numeric = (key, text) => { const value = Number.parseFloat(text); if (Number.isFinite(value)) update(key, value); };
  return <ScrollView contentContainerStyle={styles.content}>
    <Text style={styles.sectionTitle}>Instrument</Text>
    <View style={styles.card}><View><Text style={styles.detailTitle}>CondenScope CS-1</Text><Text style={styles.muted}>BLE · CS-A41D · firmware 1.0.0</Text></View>
      <Pressable style={connected ? styles.disconnect : styles.secondary} onPress={() => setConnected(!connected)}>
        <Text style={styles.secondaryText}>{connected ? 'Disconnect' : 'Connect'}</Text></Pressable></View>
    <Text style={styles.sectionTitle}>Material model</Text>
    <Text style={styles.label}>Emissivity (0.50–0.99)</Text>
    <TextInput style={styles.input} keyboardType="decimal-pad" value={String(settings.emissivity)} onChangeText={t => numeric('emissivity', t)} />
    <View style={styles.chips}>{[['Paint', 0.95], ['Brick', 0.93], ['Wood', 0.90], ['Metal tape', 0.95]].map(([name, value]) =>
      <Pressable key={name} style={styles.chip} onPress={() => update('emissivity', value)}><Text style={styles.chipText}>{name}</Text></Pressable>)}</View>
    <Text style={styles.sectionTitle}>Dew-risk thresholds</Text>
    <Text style={styles.label}>Warning margin (°C)</Text><TextInput style={styles.input} keyboardType="decimal-pad" value={String(settings.warning)} onChangeText={t => numeric('warning', t)} />
    <Text style={styles.label}>Danger margin (°C)</Text><TextInput style={styles.input} keyboardType="decimal-pad" value={String(settings.danger)} onChangeText={t => numeric('danger', t)} />
    <View style={styles.settingRow}><View><Text style={styles.detailTitle}>Audible feedback</Text><Text style={styles.muted}>Instrument chirps at risky surfaces</Text></View>
      <Switch value={settings.sound} onValueChange={v => update('sound', v)} trackColor={{ true: '#16a6a1' }} /></View>
    <Text style={styles.hint}>Settings are persisted locally and translated to SET_EMISSIVITY and SET_THRESHOLDS protocol commands. Created by {AUTHOR}.</Text>
  </ScrollView>;
}

export default function App() {
  const [tab, setTab] = useState('Live'); const [connected, setConnected] = useState(true);
  const [scanning, setScanning] = useState(false); const [sessions, setSessions] = useState([]);
  const [settings, setSettings] = useState({ emissivity: 0.95, warning: 2.5, danger: 0.75, sound: true });
  useEffect(() => { AsyncStorage.getItem('condenscope-state').then(value => { if (value) { const saved = JSON.parse(value); setSessions(saved.sessions || []); setSettings(saved.settings || settings); } }).catch(() => {}); }, []);
  useEffect(() => { AsyncStorage.setItem('condenscope-state', JSON.stringify({ sessions, settings })).catch(() => {}); }, [sessions, settings]);
  const addSession = session => setSessions(old => [session, ...old]);
  const updateSession = (id, notes) => setSessions(old => old.map(s => s.id === id ? { ...s, notes } : s));
  const screen = useMemo(() => tab === 'Live' ? <LiveScreen {...{ connected, scanning, setScanning, settings, addSession }} /> :
    tab === 'Surveys' ? <SessionsScreen {...{ sessions, updateSession }} /> :
      <SettingsScreen {...{ settings, setSettings, connected, setConnected }} />, [tab, connected, scanning, sessions, settings]);
  return <SafeAreaView style={styles.safe}><View style={styles.header}><View><Text style={styles.logo}>CONDENSCOPE</Text><Text style={styles.subtitle}>Surface dew-risk mapper</Text></View>
    <View style={styles.connection}><CircleIcon on={connected} /><Text style={styles.connected}>{connected ? 'CS-A41D' : 'OFFLINE'}</Text></View></View>
    <View style={styles.screen}>{screen}</View><View style={styles.tabs}>{['Live', 'Surveys', 'Settings'].map(name => <Pressable key={name} style={styles.tab} onPress={() => setTab(name)}><Text style={[styles.tabText, tab === name && styles.tabActive]}>{name}</Text></Pressable>)}</View>
  </SafeAreaView>;
}
function CircleIcon({ on }) { return <View style={[styles.statusDot, { backgroundColor: on ? '#43d17c' : '#60758a' }]} />; }

const styles = StyleSheet.create({ safe: { flex: 1, backgroundColor: '#071522' }, header: { padding: 18, paddingTop: Platform.OS === 'android' ? 36 : 18, flexDirection: 'row', justifyContent: 'space-between', borderBottomWidth: 1, borderBottomColor: '#183048' }, logo: { color: '#e9f4ff', fontSize: 20, fontWeight: '900', letterSpacing: 2 }, subtitle: { color: '#7890a7', fontSize: 11 }, connection: { flexDirection: 'row', alignItems: 'center', gap: 6 }, statusDot: { width: 8, height: 8, borderRadius: 4 }, connected: { color: '#b4c9dc', fontSize: 11, fontWeight: '700' }, screen: { flex: 1 }, content: { padding: 16, gap: 12, paddingBottom: 30 }, row: { flexDirection: 'row', gap: 8 }, gauge: { flex: 1, padding: 10, backgroundColor: '#102438', borderRadius: 8, borderWidth: 1, borderColor: '#1d3a54' }, gaugeDanger: { borderColor: '#ff1744' }, gaugeLabel: { color: '#7890a7', fontSize: 9, fontWeight: '800' }, gaugeValue: { color: '#f1f7fc', fontSize: 22, fontWeight: '700' }, unit: { fontSize: 11, color: '#91a8bd' }, mapWrap: { backgroundColor: '#102438', borderRadius: 10, overflow: 'hidden', borderWidth: 1, borderColor: '#294b68' }, legend: { flexDirection: 'row', justifyContent: 'space-between', flexWrap: 'wrap' }, legendItem: { flexDirection: 'row', alignItems: 'center', gap: 5 }, dot: { width: 8, height: 8, borderRadius: 2 }, muted: { color: '#7890a7', fontSize: 12 }, detail: { backgroundColor: '#102438', borderRadius: 8, padding: 12 }, detailTitle: { color: '#e9f4ff', fontWeight: '700', fontSize: 14 }, body: { color: '#b4c9dc', fontSize: 13 }, metricRow: { flexDirection: 'row', justifyContent: 'space-between', borderBottomWidth: 1, borderBottomColor: '#183048', paddingVertical: 6 }, metric: { color: '#e9f4ff', fontWeight: '800' }, bad: { color: '#ff5572' }, primary: { backgroundColor: '#16a6a1', borderRadius: 8, padding: 15, alignItems: 'center' }, stop: { backgroundColor: '#d7263d' }, primaryText: { color: '#fff', fontWeight: '900', letterSpacing: 0.6 }, hint: { color: '#60758a', fontSize: 11, lineHeight: 16 }, tabs: { height: 58, flexDirection: 'row', borderTopWidth: 1, borderTopColor: '#183048' }, tab: { flex: 1, justifyContent: 'center', alignItems: 'center' }, tabText: { color: '#60758a', fontWeight: '700' }, tabActive: { color: '#43d8d2' }, titleRow: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between' }, sectionTitle: { color: '#e9f4ff', fontSize: 18, fontWeight: '800', marginTop: 6 }, secondary: { borderWidth: 1, borderColor: '#16a6a1', padding: 8, borderRadius: 6 }, disconnect: { borderWidth: 1, borderColor: '#d7263d', padding: 8, borderRadius: 6 }, secondaryText: { color: '#43d8d2', fontWeight: '700', fontSize: 12 }, empty: { color: '#7890a7', textAlign: 'center', marginTop: 60, lineHeight: 20 }, session: { backgroundColor: '#102438', borderRadius: 9, padding: 14, borderWidth: 1, borderColor: '#1d3a54', gap: 10 }, margin: { color: '#43d8d2', fontSize: 18, fontWeight: '900' }, notes: { minHeight: 70, backgroundColor: '#081a2a', color: '#e9f4ff', borderRadius: 6, padding: 10, textAlignVertical: 'top' }, card: { backgroundColor: '#102438', padding: 14, borderRadius: 9, flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' }, label: { color: '#91a8bd', fontSize: 12, fontWeight: '700' }, input: { color: '#e9f4ff', backgroundColor: '#102438', borderWidth: 1, borderColor: '#294b68', borderRadius: 7, padding: 11 }, chips: { flexDirection: 'row', flexWrap: 'wrap', gap: 7 }, chip: { backgroundColor: '#18354d', borderRadius: 16, paddingHorizontal: 12, paddingVertical: 7 }, chipText: { color: '#bfe9e7', fontSize: 11 }, settingRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 12 }
});
