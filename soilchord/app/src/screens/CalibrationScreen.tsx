/**
 * CalibrationScreen — guided baseline calibration wizard
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { useStore, getProbe } from '../store';
import { calibrateBaseline, setIntervalCmd as sendIntervalCmd, forceMeasure } from '../api';

export default function CalibrationScreen() {
  const { probes } = useStore();
  const [selId, setSelId] = useState('');
  const [intervalStr, setIntervalStr] = useState('600');

  const probe = getProbe(selId) || probes[0];

  const calibrate = async () => {
    if (!probe) return;
    const ok = await calibrateBaseline(probe.id);
    Alert.alert(ok ? 'Baseline reset' : 'Failed', ok
      ? 'A calibration downlink was sent. The probe will re-seed its baseline on the next cycle.'
      : 'Could not reach gateway. Check connectivity and retry.');
  };

  const applyInterval = async () => {
    if (!probe) return;
    const s = parseInt(intervalStr, 10);
    if (isNaN(s) || s < 60 || s > 3600) {
      Alert.alert('Invalid interval', 'Enter a value between 60 and 3600 seconds.');
      return;
    }
    const ok = await sendIntervalCmd(probe.id, s);
    Alert.alert(ok ? 'Interval set' : 'Failed', ok ? `Probe will measure every ${s} s.` : 'Gateway unreachable.');
  };

  const force = async () => {
    if (!probe) return;
    const ok = await forceMeasure(probe.id);
    Alert.alert(ok ? 'Forced' : 'Failed', ok ? 'A measurement cycle was requested.' : 'Gateway unreachable.');
  };

  return (
    <View style={s.wrap}>
      <Text style={s.title}>Calibration & Control</Text>
      <Text style={s.sub}>Select a probe to configure.</Text>
      <View style={s.pickerRow}>
        {probes.map((p) => (
          <TouchableOpacity key={p.id} onPress={() => setSelId(p.id)} style={[s.probeBtn, selId === p.id && s.probeBtnSel]}>
            <Text style={[s.probeBtnText, selId === p.id && s.probeBtnTextSel]}>{p.name}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {probe && (
        <View style={s.panel}>
          <Text style={s.section}>Selected: {probe.name}</Text>
          <Text style={s.help}>Reset the anomaly baseline. Use after re-zeroing the probe (e.g., free-air or new install). The probe will discard its rolling EMA statistics and reseed from the next cycle.</Text>
          <TouchableOpacity style={s.btn} onPress={calibrate}><Text style={s.btnText}>Reset Baseline</Text></TouchableOpacity>

          <Text style={s.section}>Measurement interval (s)</Text>
          <TextInput style={s.input} value={intervalStr} onChangeText={setIntervalStr} keyboardType="numeric" />
          <TouchableOpacity style={s.btn} onPress={applyInterval}><Text style={s.btnText}>Set Interval</Text></TouchableOpacity>

          <Text style={s.section}>Force a measurement now</Text>
          <Text style={s.help}>Sends a downlink requesting an immediate cycle, overriding the schedule.</Text>
          <TouchableOpacity style={s.btn} onPress={force}><Text style={s.btnText}>Force Measure</Text></TouchableOpacity>
        </View>
      )}
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
  panel: { backgroundColor: '#1a3a2a', borderRadius: 12, padding: 16 },
  section: { color: '#c8e6c9', fontWeight: 'bold', marginTop: 12, marginBottom: 4 },
  help: { color: '#9ccc9c', fontSize: 11, marginBottom: 8 },
  btn: { backgroundColor: '#2e7d32', borderRadius: 8, padding: 12, alignItems: 'center', marginTop: 4 },
  btnText: { color: '#fff', fontWeight: 'bold' },
  input: { backgroundColor: '#0d1f17', color: '#e8f5e9', borderRadius: 8, padding: 10, marginBottom: 4 },
});