/*
 * SettingsScreen.tsx — pairing, thresholds, firmware info
 * Author: jayis1
 */
import React, { useState } from 'react';
import {
  View, Text, TextInput, Switch, TouchableOpacity, StyleSheet, Alert,
} from 'react-native';
import { hydra } from '../ble';

export default function SettingsScreen() {
  const [milkThresh, setMilkThresh]   = useState('0.10');
  const [whiskyThresh, setWhiskyThresh] = useState('0.05');
  const [otaBeta, setOtaBeta]         = useState(false);
  const [connected, setConnected]    = useState(hydra.isConnected());

  const pair = async () => {
    try {
      await hydra.scanAndConnect();
      setConnected(true);
    } catch (e: any) {
      Alert.alert('Pair failed', e?.message ?? '');
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.row}>
          <Text>Status</Text>
          <Text style={{ color: connected ? '#2e7d32' : '#c62828', fontWeight: '600' }}>
            {connected ? 'Connected' : 'Disconnected'}
          </Text>
        </View>
        {!connected && (
          <TouchableOpacity style={styles.button} onPress={pair}>
            <Text style={styles.buttonText}>Pair HydraScan</Text>
          </TouchableOpacity>
        )}
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Adulteration thresholds</Text>
        <Text style={styles.hint}>Milk ↔ water</Text>
        <TextInput style={styles.input} value={milkThresh} onChangeText={setMilkThresh} keyboardType="decimal-pad" />
        <Text style={styles.hint}>Whisky ↔ water</Text>
        <TextInput style={styles.input} value={whiskyThresh} onChangeText={setWhiskyThresh} keyboardType="decimal-pad" />
      </View>

      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <View style={styles.row}>
          <Text>OTA beta channel</Text>
          <Switch value={otaBeta} onValueChange={setOtaBeta} />
        </View>
        <Text style={styles.hint}>HydraScan firmware 1.0.0 · author jayis1</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#fafafa' },
  title:    { fontSize: 22, fontWeight: '700', marginVertical: 12, color: '#1e88e5' },
  section:  { backgroundColor: '#fff', padding: 16, borderRadius: 10, marginBottom: 16 },
  sectionTitle: { fontSize: 14, fontWeight: '700', color: '#444', marginBottom: 8 },
  row:      { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 8 },
  hint:     { fontSize: 12, color: '#777', marginTop: 8 },
  input:    { borderWidth: 1, borderColor: '#ddd', borderRadius: 6, padding: 10, marginTop: 4 },
  button:   { backgroundColor: '#1e88e5', padding: 12, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  buttonText: { color: '#fff', fontWeight: '600' },
});