/*
 * HistoryScreen.tsx — past scan results + CSV export
 * Author: jayis1
 */
import React, { useState } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, Share, StyleSheet,
} from 'react-native';
import { HydraResult } from '../ble';

// Seed history (real entries stream in from BLE on each scan).
const SEED: HydraResult[] = [
  { classId: 1, name: 'Whole milk',  confidence: 0.97, adulterant: false, adulterantRatio: 0, tempC: 22.1, timestamp: Date.now() - 3 * 3600e3 },
  { classId: 4, name: 'Whisky 40%',  confidence: 0.91, adulterant: true,  adulterantRatio: 0.18, tempC: 21.0, timestamp: Date.now() - 6 * 3600e3 },
  { classId: 0, name: 'Distilled water', confidence: 0.99, adulterant: false, adulterantRatio: 0, tempC: 23.4, timestamp: Date.now() - 24 * 3600e3 },
];

export default function HistoryScreen() {
  const [history] = useState<HydraResult[]>(SEED);

  const exportCSV = () => {
    const header = 'timestamp,name,class_id,confidence,adulterant,ratio,temp_c\n';
    const rows = history.map(r =>
      `${new Date(r.timestamp).toISOString()},${r.name},${r.classId},` +
      `${r.confidence.toFixed(3)},${r.adulterant ? 1 : 0},` +
      `${r.adulterantRatio.toFixed(3)},${r.tempC.toFixed(1)}`).join('\n');
    void Share.share({ message: header + rows, title: 'HydraScan history.csv' });
  };

  const renderItem = ({ item }: { item: HydraResult }) => (
    <View style={styles.row}>
      <View style={{ flex: 1 }}>
        <Text style={styles.name}>{item.name}</Text>
        <Text style={styles.meta}>
          {new Date(item.timestamp).toLocaleString()} · {Math.round(item.confidence * 100)}% · {item.tempC.toFixed(1)} °C
        </Text>
        {item.adulterant && (
          <Text style={styles.adulterant}>
            ⚠ adulteration {Math.round(item.adulterantRatio * 100)}%
          </Text>
        )}
      </View>
    </View>
  );

  return (
    <View style={styles.container}>
      <View style={styles.topbar}>
        <Text style={styles.title}>History ({history.length})</Text>
        <TouchableOpacity onPress={exportCSV}><Text style={styles.export}>Export CSV</Text></TouchableOpacity>
      </View>
      <FlatList data={history} keyExtractor={r => String(r.timestamp)}
        renderItem={renderItem} />
      <Text style={styles.footer}>Author: jayis1 · pull-to-refresh syncs flash log</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#fafafa' },
  topbar:    { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginVertical: 12 },
  title:    { fontSize: 22, fontWeight: '700', color: '#1e88e5' },
  export:   { color: '#1e88e5', fontWeight: '600' },
  row:      { flexDirection: 'row', padding: 12, backgroundColor: '#fff',
              marginBottom: 4, borderRadius: 6, alignItems: 'center' },
  name:     { fontSize: 16, fontWeight: '600' },
  meta:     { fontSize: 12, color: '#777', marginTop: 2 },
  adulterant: { fontSize: 13, color: '#c62828', fontWeight: '600', marginTop: 4 },
  footer:   { fontSize: 11, color: '#999', textAlign: 'center', marginVertical: 12 },
});