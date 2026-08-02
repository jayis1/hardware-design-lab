/*
 * ScanScreen.tsx — main measurement screen
 * Author: jayis1
 */
import React, { useState, useEffect } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { hydra, HydraResult } from '../ble';
import ResultCard from '../components/ResultCard';

export default function ScanScreen() {
  const [connected, setConnected] = useState(false);
  const [result, setResult] = useState<HydraResult | null>(null);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    const unsub = hydra.onResult(r => setResult(r));
    return () => unsub();
  }, []);

  const connect = async () => {
    setScanning(true);
    try {
      await hydra.scanAndConnect();
      setConnected(true);
    } catch (e: any) {
      Alert.alert('Scan failed', e?.message ?? 'timeout');
    } finally {
      setScanning(false);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.header}>HydraScan</Text>
      <Text style={styles.sub}>Pocket Liquid Fingerprinting · jayis1</Text>

      <View style={styles.status}>
        <View style={[styles.dot, { backgroundColor: connected ? '#2e7d32' : '#bbb' }]} />
        <Text style={styles.statusText}>
          {scanning ? 'Scanning for device…' : connected ? 'Connected' : 'Not connected'}
        </Text>
      </View>

      {!connected && (
        <TouchableOpacity style={styles.button} onPress={connect} disabled={scanning}>
          <Text style={styles.buttonText}>{scanning ? '…' : 'Connect Device'}</Text>
        </TouchableOpacity>
      )}

      {connected && (
        <Text style={styles.hint}>
          Fill the cuvette with 0.5 mL, dip the electrode, and short-press the
          device button. Results appear here within ~4 seconds.
        </Text>
      )}

      {result && <ResultCard result={result} />}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#fafafa' },
  header:    { fontSize: 28, fontWeight: '700', marginTop: 24, color: '#1e88e5' },
  sub:       { fontSize: 14, color: '#777', marginBottom: 24 },
  status:    { flexDirection: 'row', alignItems: 'center', marginBottom: 16 },
  dot:       { width: 10, height: 10, borderRadius: 5, marginRight: 8 },
  statusText: { fontSize: 16 },
  button:    { backgroundColor: '#1e88e5', padding: 14, borderRadius: 10, alignItems: 'center' },
  buttonText:{ color: '#fff', fontSize: 16, fontWeight: '600' },
  hint:      { fontSize: 13, color: '#555', marginVertical: 16, lineHeight: 20 },
});