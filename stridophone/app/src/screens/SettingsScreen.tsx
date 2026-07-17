/**
 * SettingsScreen.tsx — device pairing, sensitivity, Wi-Fi, OTA, export.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet, TextInput, TouchableOpacity, Alert, Slider } from 'react-native';
import { useDevices } from '../context/DeviceContext';

export default function SettingsScreen() {
  const { devices, addDevice, removeDevice } = useDevices();
  const [ip, setIp] = useState('');
  const [name, setName] = useState('');
  const [sensitivity, setSensitivity] = useState(70);

  const pair = () => {
    if (!ip || !name) { Alert.alert('Enter both name and IP'); return; }
    addDevice({ id: `${name}-${Date.now()}`, ip, name });
    setIp(''); setName('');
  };

  return (
    <View style={styles.container}>
      <Text style={styles.h1}>Pair a device</Text>
      <TextInput style={styles.input} placeholder="Device name" placeholderTextColor="#555"
                 value={name} onChangeText={setName} />
      <TextInput style={styles.input} placeholder="IP address (e.g. 192.168.1.42)" placeholderTextColor="#555"
                 value={ip} onChangeText={setIp} keyboardType="numeric" />
      <TouchableOpacity style={styles.btn} onPress={pair}>
        <Text style={styles.btnText}>+ Add device</Text>
      </TouchableOpacity>

      <Text style={styles.section}>Paired ({devices.length})</Text>
      {devices.map((d) => (
        <View key={d.id} style={styles.row}>
          <Text style={styles.devName}>{d.name}</Text>
          <Text style={styles.devIp}>{d.ip}</Text>
          <TouchableOpacity onPress={() => removeDevice(d.id)}>
            <Text style={styles.remove}>Remove</Text>
          </TouchableOpacity>
        </View>
      ))}

      <Text style={styles.section}>Detection sensitivity</Text>
      <Text style={styles.sliderVal}>{sensitivity}%</Text>
      <Slider style={{ width: '100%', height: 40 }}
              minimumValue={0} maximumValue={100}
              value={sensitivity} onValueChange={setSensitivity}
              minimumTrackTintColor="#39d353" />

      <Text style={styles.section}>Wi-Fi provisioning (BLE)</Text>
      <TouchableOpacity style={styles.btn}>
        <Text style={styles.btnText}>Scan over Bluetooth</Text>
      </TouchableOpacity>

      <Text style={styles.section}>Firmware OTA</Text>
      <TouchableOpacity style={styles.btn}>
        <Text style={styles.btnText}>Select .bin and push</Text>
      </TouchableOpacity>

      <Text style={styles.section}>Data export</Text>
      <TouchableOpacity style={styles.btn}>
        <Text style={styles.btnText}>Export events CSV</Text>
      </TouchableOpacity>

      <Text style={styles.footer}>Stridophone v1.0.0 · © jayis1 · MIT</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 16 },
  h1: { color: '#e0e0e0', fontSize: 20, fontWeight: 'bold' },
  input: { backgroundColor: '#1a1a2e', color: '#e0e0e0', borderRadius: 8,
           padding: 12, marginTop: 10, borderWidth: 1, borderColor: '#2a2a3e' },
  btn: { backgroundColor: '#2a2a4e', borderRadius: 8, padding: 14, marginTop: 10, alignItems: 'center' },
  btnText: { color: '#39d353', fontSize: 15, fontWeight: '600' },
  section: { color: '#aaa', fontSize: 14, marginTop: 22, marginBottom: 6 },
  row: { flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
         backgroundColor: '#1a1a2e', borderRadius: 6, padding: 10, marginTop: 4 },
  devName: { color: '#e0e0e0', fontSize: 14 },
  devIp: { color: '#888', fontSize: 12, flex: 1, marginLeft: 8 },
  remove: { color: '#ff6b6b', fontSize: 12 },
  sliderVal: { color: '#39d353', fontSize: 14 },
  footer: { color: '#555', fontSize: 11, textAlign: 'center', marginTop: 30 },
});