/**
 * Dictionary.tsx — toggle which character sets the band recognizes
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, Switch, TouchableOpacity } from 'react-native';
import { bleClient } from '../components/BleManager';

interface SetDef { key: number; label: string; mask: number; examples: string; }

const SETS: SetDef[] = [
  { key: 0, label: 'Lowercase a–z',      mask: 0x01, examples: 'abcdefghijklmnopqrstuvwxyz' },
  { key: 1, label: 'Uppercase A–Z',      mask: 0x02, examples: 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' },
  { key: 2, label: 'Digits 0–9',         mask: 0x04, examples: '0123456789' },
  { key: 3, label: 'Punctuation',        mask: 0x08, examples: '.,?!\'";:-_' },
  { key: 4, label: 'Gestures',            mask: 0x10, examples: '⌫ ↵ ⇥ ⇧ ↶ ✂ ⧉ ⎘ ⎀ ⨯ ✓' },
];

export function DictionaryScreen() {
  const [mask, setMask] = useState(0x1F);

  const toggle = (bit: number) => {
    const next = mask ^ bit;
    setMask(next);
    bleClient.setActiveSet(next).catch(() => {});
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Active Character Sets</Text>
      <Text style={styles.subtitle}>Toggle which classes the band will emit as HID scancodes.</Text>

      {SETS.map(s => (
        <View key={s.key} style={styles.row}>
          <View style={{ flex: 1 }}>
            <Text style={styles.label}>{s.label}</Text>
            <Text style={styles.examples}>{s.examples}</Text>
          </View>
          <Switch
            value={(mask & s.mask) !== 0}
            onValueChange={() => toggle(s.mask)}
            trackColor={{ false: '#333', true: '#2563eb' }}
            thumbColor="#fff"
          />
        </View>
      ))}

      <TouchableOpacity style={styles.applyBtn} onPress={() => bleClient.setActiveSet(mask).catch(() => {})}>
        <Text style={styles.applyText}>Apply ({mask.toString(2).padStart(5, '0')})</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#fff', fontSize: 18, fontWeight: '600', marginBottom: 4 },
  subtitle: { color: '#666', fontSize: 12, marginBottom: 16 },
  row: { flexDirection: 'row', alignItems: 'center', paddingVertical: 12, borderBottomColor: '#1a1a26', borderBottomWidth: 1 },
  label: { color: '#e8e8f0', fontSize: 15 },
  examples: { color: '#555', fontSize: 11, fontFamily: 'monospace', marginTop: 2 },
  applyBtn: { backgroundColor: '#2563eb', padding: 14, borderRadius: 10, marginTop: 20, alignItems: 'center' },
  applyText: { color: '#fff', fontSize: 15, fontWeight: '600' },
});