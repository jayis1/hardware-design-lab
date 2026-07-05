// SetupScreen.tsx — device discovery and self-test
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect } from 'react';
import { View, Text, Button, StyleSheet, Alert } from 'react-native';
import DeviceService from '../services/DeviceService';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 16 },
  label: { color: '#7c9eff', fontSize: 14, marginTop: 12 },
  value: { color: '#e0e6ff', fontSize: 16, marginBottom: 8 },
  status: { color: '#5a6390', fontSize: 12, marginTop: 8 },
  footer: { color: '#3a4060', fontSize: 10, marginTop: 20, textAlign: 'center' },
});

export default function SetupScreen({ navigation }: any) {
  const [scanning, setScanning] = useState(false);
  const [connected, setConnected] = useState(DeviceService.isBleConnected());
  const [status, setStatus] = useState<any>(null);

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => setStatus(s));
    return () => unsub();
  }, []);

  const handleScan = async () => {
    setScanning(true);
    const ok = await DeviceService.scanAndConnect(15000);
    setScanning(false);
    setConnected(ok);
    if (!ok) Alert.alert('Not Found', 'No MûonScape device in range.');
  };

  const handleSelfTest = async () => {
    try {
      const s = await DeviceService.getStatus();
      setStatus(s);
      Alert.alert('Self-Test', `State: ${s.state}\nFirmware: ${s.fw}\nSiPM: ${s.sipm_mv} mV\nGPS sats: ${s.sats}`);
    } catch (e: any) {
      Alert.alert('Self-Test Failed', e.message);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>MûonScape Setup</Text>
      <Text style={styles.label}>Connection</Text>
      <Text style={styles.value}>{connected ? 'BLE Connected' : 'Disconnected'}</Text>
      <Button title={scanning ? 'Scanning...' : 'Scan & Connect'} onPress={handleScan} disabled={scanning} />
      <Text style={styles.label}>Self-Test</Text>
      <Button title="Run Self-Test" onPress={handleSelfTest} disabled={!connected} />
      {status && (
        <View>
          <Text style={styles.label}>Device Status</Text>
          <Text style={styles.value}>Firmware: {status.fw}</Text>
          <Text style={styles.value}>State: {status.state}</Text>
          <Text style={styles.value}>SiPM Bias: {status.sipm_mv} mV</Text>
          <Text style={styles.value}>Battery: {status.soc}% ({status.pack_mv} mV)</Text>
          <Text style={styles.value}>Temperature: {status.temp_c.toFixed(1)} °C</Text>
          <Text style={styles.value}>GPS: {status.gps_fix ? `FIX (${status.sats} sats)` : 'No fix'}</Text>
          <Text style={styles.value}>Tilt: roll={status.roll.toFixed(1)}° pitch={status.pitch.toFixed(1)}°</Text>
        </View>
      )}
      <View style={{ flexDirection: 'row', marginTop: 16 }}>
        <Button title="Calibration" onPress={() => navigation.navigate('Calibration')} disabled={!connected} />
        <View style={{ width: 8 }} />
        <Button title="Acquisition" onPress={() => navigation.navigate('Acquisition')} disabled={!connected} />
      </View>
      <Text style={styles.footer}>MûonScape by jayis1 — MIT License</Text>
    </View>
  );
}