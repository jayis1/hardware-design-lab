/**
 * Device.tsx — battery, firmware, signal, "find my band"
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */

import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';
import { bleClient } from '../components/BleManager';

export function DeviceScreen() {
  const [battery, setBattery] = useState(0);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    bleClient.scanAndConnect().then(() => setConnected(true)).catch(() => {});
    const unsub = bleClient.onBattery((pct) => setBattery(pct));
    return () => unsub();
  }, []);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>GlyphFlow Band</Text>

      <View style={styles.card}>
        <Text style={styles.cardLabel}>Connection</Text>
        <Text style={styles.cardValue}>{connected ? 'Connected' : 'Disconnected'}</Text>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardLabel}>Battery</Text>
        <View style={styles.battRow}>
          <View style={styles.battBar}>
            <View style={[styles.battFill, { width: `${battery}%`, backgroundColor: battery > 20 ? '#4ade80' : '#ef4444' }]} />
          </View>
          <Text style={styles.battText}>{battery}%</Text>
        </View>
      </View>

      <View style={styles.card}>
        <Text style={styles.cardLabel}>Firmware</Text>
        <Text style={styles.cardValue}>GlyphFlow 1.0 (jayis1)</Text>
      </View>

      <TouchableOpacity style={styles.btn} onPress={() => bleClient.findMyBand().catch(() => {})}>
        <Text style={styles.btnText}>Find My Band</Text>
      </TouchableOpacity>

      <TouchableOpacity style={[styles.btn, { marginTop: 8 }]} onPress={() => bleClient.disconnect().then(() => setConnected(false))}>
        <Text style={styles.btnText}>Disconnect</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a12', padding: 16 },
  title: { color: '#fff', fontSize: 20, fontWeight: '600', marginBottom: 16 },
  card: { backgroundColor: '#14141f', borderRadius: 10, padding: 14, marginBottom: 10 },
  cardLabel: { color: '#666', fontSize: 12, marginBottom: 4 },
  cardValue: { color: '#fff', fontSize: 15 },
  battRow: { flexDirection: 'row', alignItems: 'center' },
  battBar: { flex: 1, height: 12, backgroundColor: '#222', borderRadius: 6, marginRight: 10, overflow: 'hidden' },
  battFill: { height: '100%' },
  battText: { color: '#fff', fontSize: 14, fontFamily: 'monospace' },
  btn: { backgroundColor: '#1a1a26', padding: 14, borderRadius: 10, alignItems: 'center', marginTop: 8 },
  btnText: { color: '#7cc4ff', fontSize: 15 },
});