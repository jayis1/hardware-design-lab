/**
 * ExportScreen — CSV/JSON export of measurement history for inspection reports
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 */
import React, { useState } from 'react';
import { View, Text, TextInput, TouchableOpacity, StyleSheet, Share, Alert } from 'react-native';
import { exportCsv } from '../api';

export default function ExportScreen() {
  const [nodeId, setNodeId] = useState('');
  const [hours, setHours] = useState('24');
  const [csv, setCsv] = useState('');

  const doExport = async () => {
    const h = parseInt(hours, 10) || 24;
    const now = Math.floor(Date.now() / 1000);
    const from = now - h * 3600;
    const text = await exportCsv(nodeId, from, now);
    if (text) {
      setCsv(text);
      Alert.alert('Export Ready', `${text.length} bytes — tap Share to send`);
    } else {
      Alert.alert('Export Failed', 'Could not reach the gateway or node not found.');
    }
  };

  const share = async () => {
    if (!csv) return;
    try {
      await Share.share({ message: csv, title: `TensilGuard export — ${nodeId}` });
    } catch { /* user cancelled */ }
  };

  return (
    <View style={styles.container}>
      <View style={styles.card}>
        <Text style={styles.title}>Export Data</Text>
        <Text style={styles.sub}>Download measurement history as CSV for a bridge inspection report.</Text>

        <Text style={styles.label}>Node ID</Text>
        <TextInput style={styles.input} value={nodeId} onChangeText={setNodeId} placeholder="00:11:22:33:44:55" />

        <Text style={styles.label}>Time range (hours back)</Text>
        <TextInput style={styles.input} value={hours} onChangeText={setHours} keyboardType="numeric" placeholder="24" />

        <TouchableOpacity style={styles.btn} onPress={doExport}>
          <Text style={styles.btnText}>Export CSV</Text>
        </TouchableOpacity>

        {csv ? (
          <View>
            <Text style={styles.previewLabel}>Preview (first 500 chars):</Text>
            <Text style={styles.preview}>{csv.slice(0, 500)}{csv.length > 500 ? '…' : ''}</Text>
            <TouchableOpacity style={styles.btn} onPress={share}>
              <Text style={styles.btnText}>Share</Text>
            </TouchableOpacity>
          </View>
        ) : null}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a', padding: 12 },
  card: { backgroundColor: '#142535', borderRadius: 8, padding: 16 },
  title: { color: '#e8f0f8', fontSize: 18, fontWeight: 'bold', marginBottom: 4 },
  sub: { color: '#8a9ab0', fontSize: 13, marginBottom: 16 },
  label: { color: '#6a8aa0', fontSize: 13, marginTop: 12, marginBottom: 4 },
  input: { backgroundColor: '#0d1a2a', color: '#e8f0f8', borderWidth: 1, borderColor: '#2a4a6a', borderRadius: 6, padding: 10 },
  btn: { backgroundColor: '#2196f3', padding: 14, borderRadius: 8, alignItems: 'center', marginTop: 16 },
  btnText: { color: '#fff', fontWeight: '600', fontSize: 15 },
  previewLabel: { color: '#6a8aa0', fontSize: 12, marginTop: 16, marginBottom: 4 },
  preview: { color: '#4fc3f7', fontSize: 11, fontFamily: 'monospace', backgroundColor: '#0d1a2a', padding: 8, borderRadius: 4 },
});