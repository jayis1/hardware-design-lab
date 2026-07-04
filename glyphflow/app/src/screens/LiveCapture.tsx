/**
 * LiveCapture.tsx — default screen: shows the live text stream from the band
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView } from 'react-native';
import { bleClient } from '../components/BleManager';

export function LiveCaptureScreen() {
  const [text, setText] = useState('');
  const [connected, setConnected] = useState(false);
  const [penActive, setPenActive] = useState(false);

  useEffect(() => {
    // Try to auto-connect on mount.
    bleClient.scanAndConnect().then(() => setConnected(true)).catch(() => {});
    const unsub = bleClient.onCharacter((ch) => {
      // The band emits HID scancodes; we translate the common ones.
      const map: Record<number, string> = {
        4: 'a', 5: 'b', 6: 'c', 7: 'd', 8: 'e', 9: 'f', 10: 'g', 11: 'h',
        12: 'i', 13: 'j', 14: 'k', 15: 'l', 16: 'm', 17: 'n', 18: 'o', 19: 'p',
        20: 'q', 21: 'r', 22: 's', 23: 't', 24: 'u', 25: 'v', 26: 'w', 27: 'x',
        28: 'y', 29: 'z',
        30: '1', 31: '2', 32: '3', 33: '4', 34: '5', 35: '6', 36: '7', 37: '8',
        38: '9', 39: '0',
        40: '\n', 42: '⌫', 44: ' ',
      };
      const code = typeof ch === 'number' ? ch : (ch as any).code;
      const c = map[code];
      if (!c) return;
      if (c === '⌫') setText(t => t.slice(0, -1));
      else if (c === '\n') setText(t => t + '\n');
      else setText(t => t + c);
    });
    return () => { unsub(); };
  }, []);

  // "Pen" indicator: while we don't get pen-state from BLE directly in this
  // reference UI, we treat any character arrival as pen-active for 1.5 s.
  useEffect(() => {
    if (text.length === 0) return;
    setPenActive(true);
    const t = setTimeout(() => setPenActive(false), 1500);
    return () => clearTimeout(t);
  }, [text]);

  return (
    <View style={styles.container}>
      <View style={styles.statusRow}>
        <View style={[styles.dot, { backgroundColor: connected ? (penActive ? '#4ade80' : '#7cc4ff') : '#ef4444' }]} />
        <Text style={styles.status}>
          {connected ? (penActive ? 'Writing…' : 'Ready — write in the air') : 'Searching for band…'}
        </Text>
      </View>

      <ScrollView style={styles.scroll} contentContainerStyle={styles.scrollContent}>
        <Text style={styles.text}>{text || <Text style={styles.placeholder}>Your air-written text will appear here.</Text>}</Text>
      </ScrollView>

      <View style={styles.toolbar}>
        <TouchableOpacity style={styles.btn} onPress={() => setText('')}>
          <Text style={styles.btnText}>Clear</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.btn} onPress={() => setText(t => t.slice(0, -1))}>
          <Text style={styles.btnText}>⌫ Backspace</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  statusRow: { flexDirection: 'row', alignItems: 'center', marginBottom: 12 },
  dot: { width: 10, height: 10, borderRadius: 5, marginRight: 8 },
  status: { color: '#ccc', fontSize: 14 },
  scroll: { flex: 1, borderWidth: 1, borderColor: '#222', borderRadius: 12, marginBottom: 12 },
  scrollContent: { padding: 14 },
  text: { color: '#e8e8f0', fontSize: 20, fontFamily: 'monospace', lineHeight: 28 },
  placeholder: { color: '#555', fontStyle: 'italic' },
  toolbar: { flexDirection: 'row', gap: 8 },
  btn: { backgroundColor: '#1a1a26', paddingHorizontal: 16, paddingVertical: 10, borderRadius: 8 },
  btnText: { color: '#7cc4ff', fontSize: 14 },
});