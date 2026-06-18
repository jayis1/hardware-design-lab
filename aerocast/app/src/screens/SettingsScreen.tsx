/**
 * SettingsScreen.tsx — Wi-Fi, MQTT, sampling, notifications, OTA
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { useState } from 'react';
import { View, Text, TextInput, Button, Switch, StyleSheet } from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';

export default function SettingsScreen() {
  const { connected, sendCommand } = useBLE();
  const [ssid, setSsid] = useState('');
  const [pass, setPass] = useState('');
  const [broker, setBroker] = useState('mqtt.aerocast.local');
  const [flow, setFlow] = useState('15');
  const [notify, setNotify] = useState(true);
  const [threshold, setThreshold] = useState('200');

  const save = async () => {
    await AsyncStorage.multiSet([
      ['ssid', ssid], ['broker', broker], ['flow', flow],
      ['notify', String(notify)], ['threshold', threshold],
    ]);
    if (connected) {
      await sendCommand(`set flow ${flow}`);
    }
  };

  const ota = async () => {
    if (connected) await sendCommand('reboot');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Settings</Text>
      <Text style={styles.author}>AeroCast by jayis1</Text>

      <Card>
        <Text style={styles.section}>Wi-Fi</Text>
        <Text style={styles.label}>SSID</Text>
        <TextInput style={styles.input} value={ssid} onChangeText={setSsid} placeholder="home-wifi" placeholderTextColor="#556" />
        <Text style={styles.label}>Password</Text>
        <TextInput style={styles.input} value={pass} onChangeText={setPass} secureTextEntry placeholder="••••" placeholderTextColor="#556" />
      </Card>

      <Card>
        <Text style={styles.section}>MQTT broker</Text>
        <TextInput style={styles.input} value={broker} onChangeText={setBroker} />
      </Card>

      <Card>
        <Text style={styles.section}>Sampling</Text>
        <Text style={styles.label}>Target flow (L/min)</Text>
        <TextInput style={styles.input} value={flow} onChangeText={setFlow} keyboardType="numeric" />
      </Card>

      <Card>
        <Text style={styles.section}>Notifications</Text>
        <View style={styles.row}>
          <Text style={styles.body}>Alert when total pollen &gt; threshold</Text>
          <Switch value={notify} onValueChange={setNotify} />
        </View>
        <Text style={styles.label}>Threshold (grains/m³)</Text>
        <TextInput style={styles.input} value={threshold} onChangeText={setThreshold} keyboardType="numeric" />
      </Card>

      <Card>
        <Button title="Save & apply" onPress={save} disabled={!connected} />
        <View style={{ height: 8 }} />
        <Button title="Reboot device (OTA)" onPress={ota} disabled={!connected} color="#C62828" />
      </Card>

      <Text style={styles.footer}>AeroCast v1.0.0 · author jayis1 · MIT licensed</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1020', padding: 10 },
  title: { color: '#fff', fontSize: 22, fontWeight: '700', marginVertical: 10 },
  author: { color: '#5CB4FF', fontSize: 12, marginBottom: 8 },
  section: { color: '#fff', fontSize: 15, fontWeight: '600', marginBottom: 8 },
  label: { color: '#8aa', fontSize: 12, marginTop: 6, marginBottom: 2 },
  input: { backgroundColor: '#1a2238', color: '#fff', borderRadius: 6, paddingHorizontal: 10, paddingVertical: 8 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  body: { color: '#abc', fontSize: 13, flex: 1, flexWrap: 'wrap' },
  footer: { color: '#556', fontSize: 11, textAlign: 'center', marginTop: 16 },
});