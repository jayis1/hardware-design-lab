// SettingsScreen.tsx — config, Wi-Fi, firmware OTA
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect } from 'react';
import { View, Text, TextInput, Button, Switch, StyleSheet, Alert } from 'react-native';
import DeviceService from '../services/DeviceService';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 12 },
  section: { color: '#7c9eff', fontSize: 16, fontWeight: '600', marginTop: 16 },
  label: { color: '#7c9eff', fontSize: 12, marginTop: 8 },
  value: { color: '#e0e6ff', fontSize: 14 },
  input: { backgroundColor: '#1a1e3a', color: '#e0e6ff', borderWidth: 1,
           borderColor: '#3a4060', borderRadius: 4, padding: 8, marginVertical: 4 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
         marginVertical: 4 },
  footer: { color: '#3a4060', fontSize: 10, marginTop: 20, textAlign: 'center' },
});

export default function SettingsScreen({ navigation }: any) {
  const [sipmMv, setSipmMv] = useState('15700');
  const [coincNs, setCoincNs] = useState('5');
  const [ssid, setSsid] = useState('');
  const [pass, setPass] = useState('');
  const [solarMode, setSolarMode] = useState(false);
  const [status, setStatus] = useState<any>(null);

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => setStatus(s));
    return () => unsub();
  }, []);

  const handleApplyConfig = async () => {
    try {
      await DeviceService.setConfig(parseInt(sipmMv, 10), parseInt(coincNs, 10));
      Alert.alert('Config Applied', `SiPM ${sipmMv} mV, Coincidence ${coincNs} ns`);
    } catch (e: any) { Alert.alert('Error', e.message); }
  };

  const handleSetWifi = async () => {
    try {
      await DeviceService.setWifi(ssid, pass);
      Alert.alert('Wi-Fi Set', `SSID: ${ssid}`);
    } catch (e: any) { Alert.alert('Error', e.message); }
  };

  const handleSleep = async () => {
    Alert.alert('Sleep', 'Sending device to sleep mode. Wake via BLE or schedule.',
      [{ text: 'Sleep', onPress: () => DeviceService.sleep() },
       { text: 'Cancel', style: 'cancel' }]);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>

      <Text style={styles.section}>Detector</Text>
      <Text style={styles.label}>SiPM Bias Voltage (mV, 14000-16500)</Text>
      <TextInput style={styles.input} value={sipmMv} onChangeText={setSipmMv} keyboardType="numeric" />
      <Text style={styles.label}>Coincidence Window (ns, 1-50)</Text>
      <TextInput style={styles.input} value={coincNs} onChangeText={setCoincNs} keyboardType="numeric" />
      <Button title="Apply Detector Config" onPress={handleApplyConfig} />

      <Text style={styles.section}>Wi-Fi</Text>
      <Text style={styles.label}>Network SSID</Text>
      <TextInput style={styles.input} value={ssid} onChangeText={setSsid} />
      <Text style={styles.label}>Password</Text>
      <TextInput style={styles.input} value={pass} onChangeText={setPass} secureTextEntry />
      <Button title="Set Wi-Fi Credentials" onPress={handleSetWifi} />

      <Text style={styles.section}>Power</Text>
      <View style={styles.row}>
        <Text style={styles.value}>Solar MPPT Mode</Text>
        <Switch value={solarMode} onValueChange={setSolarMode} />
      </View>
      {status && (
        <View>
          <Text style={styles.label}>Battery: {status.soc}% ({status.pack_mv} mV)</Text>
          <Text style={styles.label}>Current: {status.current_ma} mA</Text>
          <Text style={styles.label}>Temperature: {status.temp_c.toFixed(1)} °C</Text>
        </View>
      )}
      <Button title="Sleep Device" onPress={handleSleep} color="#ff6b6b" />

      <Text style={styles.footer}>MûonScape by jayis1 · firmware {status?.fw ?? '?'}</Text>
    </View>
  );
}