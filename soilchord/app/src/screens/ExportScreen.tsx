/**
 * ExportScreen — CSV / JSON export of probe history
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, Share, StyleSheet, Alert } from 'react-native';
import { useStore, getHistory, getProbe, moistureEstimate } from '../store';

const DEPTHS = [0.0, 0.3, 0.6, 0.9, 1.2, 1.5];

export default function ExportScreen() {
  const { probes } = useStore();
  const [probeId, setProbeId] = useState('');
  const [days, setDays] = useState('7');
  const [soil, setSoil] = useState<'sand' | 'loam' | 'silt' | 'clay'>('loam');

  const probe = getProbe(probeId) || probes[0];

  const buildCSV = () => {
    if (!probe) return '';
    const hist = getHistory(probe.id);
    const cutoff = Date.now() - parseInt(days, 10) * 86400000;
    const rows = hist.filter((f) => f.unixTime * 1000 >= cutoff);
    let out = 'seq,iso_time,battery_mv,solar_mv,urgent';
    for (let i = 0; i < 6; i++) out += `,f0_${i}_hz,q_freq_${i},q_time_${i},temp_${i}_c,moist_${i}_pct,alert_${i}`;
    out += '\n';
    for (const f of rows) {
      let line = `${f.seq},${new Date(f.unixTime * 1000).toISOString()},${f.batteryMv},${f.solarMv},${f.urgent}`;
      for (let i = 0; i < 6; i++) {
        const c = f.chords[i];
        const m = moistureEstimate(c.qFreq, soil);
        const a = (c.flags & 0x04) ? 1 : 0;
        line += `,${c.f0Hz.toFixed(2)},${c.qFreq.toFixed(1)},${c.qTime.toFixed(1)},${c.tempC.toFixed(1)},${m.toFixed(1)},${a}`;
      }
      out += line + '\n';
    }
    return out;
  };

  const buildJSON = () => {
    if (!probe) return '';
    const hist = getHistory(probe.id);
    const cutoff = Date.now() - parseInt(days, 10) * 86400000;
    const rows = hist.filter((f) => f.unixTime * 1000 >= cutoff);
    return JSON.stringify({ probe: probe.id, soilType: soil, exportedAt: new Date().toISOString(), frames: rows }, null, 2);
  };

  const shareCSV = async () => {
    const csv = buildCSV();
    if (!csv) { Alert.alert('No data', 'Select a probe with history first.'); return; }
    try { await Share.share({ title: `${probe!.name}.csv`, message: csv }); }
    catch (e: any) { Alert.alert('Export failed', e?.message ?? String(e)); }
  };

  const shareJSON = async () => {
    const json = buildJSON();
    if (!json) { Alert.alert('No data', 'Select a probe with history first.'); return; }
    try { await Share.share({ title: `${probe!.name}.json`, message: json }); }
    catch (e: any) { Alert.alert('Export failed', e?.message ?? String(e)); }
  };

  return (
    <View style={s.wrap}>
      <Text style={s.title}>Export Data</Text>
      <Text style={s.sub}>Export the selected probe's history for GIS or slope-stability analysis.</Text>

      <View style={s.pickerRow}>
        {probes.map((p) => (
          <TouchableOpacity key={p.id} onPress={() => setProbeId(p.id)} style={[s.probeBtn, probeId === p.id && s.probeBtnSel]}>
            <Text style={[s.probeBtnText, probeId === p.id && s.probeBtnTextSel]}>{p.name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <Text style={s.label}>Days to export</Text>
      <TextInput style={s.input} value={days} onChangeText={setDays} keyboardType="numeric" />

      <Text style={s.label}>Soil type (for moisture column)</Text>
      <View style={s.pickerRow}>
        {(['sand', 'loam', 'silt', 'clay'] as const).map((t) => (
          <TouchableOpacity key={t} onPress={() => setSoil(t)} style={[s.probeBtn, soil === t && s.probeBtnSel]}>
            <Text style={[s.probeBtnText, soil === t && s.probeBtnTextSel]}>{t}</Text>
          </TouchableOpacity>
        ))}
      </View>

      <TouchableOpacity style={s.btn} onPress={shareCSV}><Text style={s.btnText}>Export CSV</Text></TouchableOpacity>
      <TouchableOpacity style={s.btn} onPress={shareJSON}><Text style={s.btnText}>Export JSON</Text></TouchableOpacity>
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0d1f17', padding: 12 },
  title: { color: '#e8f5e9', fontSize: 22, fontWeight: 'bold' },
  sub: { color: '#9ccc9c', fontSize: 12, marginBottom: 12 },
  pickerRow: { flexDirection: 'row', flexWrap: 'wrap', marginBottom: 12 },
  probeBtn: { paddingHorizontal: 10, paddingVertical: 6, borderRadius: 8, backgroundColor: '#1a3a2a', margin: 4 },
  probeBtnSel: { backgroundColor: '#8bc34a' },
  probeBtnText: { color: '#e8f5e9', fontSize: 12 },
  probeBtnTextSel: { color: '#0d1f17', fontWeight: 'bold' },
  label: { color: '#c8e6c9', fontSize: 12, marginTop: 8, marginBottom: 4 },
  input: { backgroundColor: '#1a3a2a', color: '#e8f5e9', borderRadius: 8, padding: 10, marginBottom: 8 },
  btn: { backgroundColor: '#2e7d32', borderRadius: 8, padding: 14, alignItems: 'center', marginTop: 8 },
  btnText: { color: '#fff', fontWeight: 'bold' },
});