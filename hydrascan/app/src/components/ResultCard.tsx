/*
 * ResultCard.tsx — renders the latest scan result
 * Author: jayis1
 */
import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { HydraResult } from '../ble';

export default function ResultCard({ result }: { result: HydraResult }) {
  const pct = Math.round(result.confidence * 100);
  return (
    <View style={styles.card}>
      <Text style={styles.label}>IDENTITY</Text>
      <Text style={styles.name}>{result.name}</Text>

      <View style={styles.row}>
        <Text style={styles.label}>Confidence</Text>
        <Text style={styles.value}>{pct}%</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Temperature</Text>
        <Text style={styles.value}>{result.tempC.toFixed(1)} °C</Text>
      </View>

      {result.adulterant ? (
        <View style={[styles.flag, styles.adulterant]}>
          <Text style={styles.flagText}>
            ⚠ ADULTERATION: {Math.round(result.adulterantRatio * 100)}%
          </Text>
        </View>
      ) : (
        <View style={[styles.flag, styles.ok]}>
          <Text style={styles.flagText}>✓ OK</Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card:       { backgroundColor: '#fff', padding: 18, borderRadius: 14, elevation: 3 },
  label:      { fontSize: 11, color: '#888', letterSpacing: 1.5, marginTop: 12 },
  name:       { fontSize: 26, fontWeight: '700', marginTop: 4 },
  row:        { flexDirection: 'row', justifyContent: 'space-between', marginTop: 8 },
  value:      { fontSize: 16, fontWeight: '600' },
  flag:       { padding: 12, borderRadius: 8, marginTop: 16, alignItems: 'center' },
  adulterant: { backgroundColor: '#ffebee' },
  ok:         { backgroundColor: '#e8f5e9' },
  flagText:   { fontSize: 16, fontWeight: '700' },
});