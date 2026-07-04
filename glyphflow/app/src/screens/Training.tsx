/**
 * Training.tsx — collect labeled samples and push new weights to the band
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, FlatList } from 'react-native';
import { bleClient } from '../components/BleManager';
import { StrokeCanvas } from '../components/StrokeCanvas';

interface Sample { label: string; pts: { x: number; y: number }[]; }

export function TrainingScreen() {
  const [label, setLabel] = useState('');
  const [recording, setRecording] = useState(false);
  const [samples, setSamples] = useState<Sample[]>([]);
  const [current, setCurrent] = useState<{ x: number; y: number }[]>([]);
  const [status, setStatus] = useState('idle');

  useEffect(() => {
    const unsub = bleClient.onTrajectory((p) => {
      if (recording) setCurrent(p);
    });
    return () => unsub();
  }, [recording]);

  const start = async () => {
    if (!label) return;
    await bleClient.sendTrainingCommand('start');
    setRecording(true);
    setStatus('recording…');
    setCurrent([]);
  };

  const save = async () => {
    if (current.length === 0) return;
    setSamples(s => [...s, { label, pts: current }]);
    setRecording(false);
    setStatus(`saved sample for "${label}"`);
    setCurrent([]);
  };

  const retrain = async () => {
    // A real implementation runs the same TCN in JS, fits, quantizes to int8,
    // and pushes the new blob. Here we just send a placeholder command.
    setStatus('training in-app…');
    const blob = new Int8Array(12024).fill(0);
    await bleClient.pushWeights(blob);
    setStatus('pushed new weights to band');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Training Mode</Text>
      <Text style={styles.status}>Status: {status}</Text>

      <View style={styles.row}>
        <TextInput
          style={styles.input}
          placeholder="Label (e.g. 'a', '7', '?')"
          placeholderTextColor="#555"
          value={label}
          onChangeText={setLabel}
        />
        <TouchableOpacity style={styles.btn} onPress={recording ? save : start}>
          <Text style={styles.btnText}>{recording ? 'Save' : 'Start'}</Text>
        </TouchableOpacity>
      </View>

      <StrokeCanvas points={current} />

      <Text style={styles.subtitle}>Collected samples ({samples.length})</Text>
      <FlatList
        data={samples}
        keyExtractor={(_, i) => i.toString()}
        renderItem={({ item }) => (
          <View style={styles.sampleRow}>
            <Text style={styles.sampleLabel}>{item.label}</Text>
            <Text style={styles.sampleMeta}>{item.pts.length} pts</Text>
          </View>
        )}
      />

      <TouchableOpacity style={styles.trainBtn} onPress={retrain}>
        <Text style={styles.trainBtnText}>Retrain & Push to Band</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#fff', fontSize: 18, fontWeight: '600', marginBottom: 4 },
  status: { color: '#7cc4ff', fontSize: 13, marginBottom: 12 },
  row: { flexDirection: 'row', gap: 8, marginBottom: 8 },
  input: { flex: 1, borderWidth: 1, borderColor: '#333', borderRadius: 8, padding: 10, color: '#fff' },
  btn: { backgroundColor: '#1a1a26', paddingHorizontal: 16, paddingVertical: 10, borderRadius: 8 },
  btnText: { color: '#7cc4ff', fontSize: 14 },
  subtitle: { color: '#aaa', fontSize: 14, marginTop: 12, marginBottom: 6 },
  sampleRow: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8, borderBottomColor: '#222', borderBottomWidth: 1 },
  sampleLabel: { color: '#fff', fontFamily: 'monospace', fontSize: 16 },
  sampleMeta: { color: '#666', fontSize: 12 },
  trainBtn: { backgroundColor: '#2563eb', padding: 14, borderRadius: 10, marginTop: 16, alignItems: 'center' },
  trainBtnText: { color: '#fff', fontSize: 15, fontWeight: '600' },
});