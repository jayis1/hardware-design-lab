// screens/ConnectScreen.js — device scan & connect
// Author: jayis1  License: MIT
import React, { useState, useEffect } from 'react';
import { View, Text, Button, FlatList, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import { theme } from '../utils/theme';
import ble from '../utils/ble';

export default function ConnectScreen({ onConnected }) {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(false);

  useEffect(() => { return () => { ble.manager.stopDeviceScan(); }; }, []);

  const scan = async () => {
    setScanning(true);
    setDevices([]);
    try {
      const found = await ble.scan(5000);
      setDevices(found);
    } catch (e) {
      Alert.alert('Scan error', String(e.message || e));
    } finally {
      setScanning(false);
    }
  };

  const connect = async (id) => {
    setConnecting(true);
    try {
      await ble.connect(id);
      const info = await ble.readDeviceInfo();
      onConnected(info);
    } catch (e) {
      Alert.alert('Connect error', String(e.message || e));
    } finally {
      setConnecting(false);
    }
  };

  const renderItem = ({ item }) => (
    <TouchableOpacity style={styles.devRow} onPress={() => connect(item.id)}>
      <View>
        <Text style={styles.devName}>{item.name}</Text>
        <Text style={styles.devId}>{item.id}</Text>
      </View>
      <Text style={styles.rssi}>{item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>HydroFluor Connect</Text>
      <Text style={styles.sub}>Find and connect to a nearby HydroFluor sonde over BLE.</Text>
      <Button title={scanning ? 'Scanning…' : 'Scan for devices'} onPress={scan} disabled={scanning || connecting} color={theme.colors.accent} />
      <FlatList
        data={devices}
        keyExtractor={i => i.id}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={<Text style={styles.empty}>{scanning ? 'Looking…' : 'No devices yet — tap Scan.'}</Text>}
      />
      {connecting && <Text style={styles.connecting}>Connecting…</Text>}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: theme.colors.bg, padding: theme.spacing.md },
  title: { color: theme.colors.text, fontSize: 22, fontWeight: '700', fontFamily: theme.fonts.title },
  sub: { color: theme.colors.textDim, fontSize: 13, marginBottom: theme.spacing.md },
  list: { marginTop: theme.spacing.md },
  devRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', backgroundColor: theme.colors.surface, padding: theme.spacing.sm, borderRadius: theme.radius.md, marginBottom: 6 },
  devName: { color: theme.colors.text, fontSize: 15, fontWeight: '600' },
  devId: { color: theme.colors.textDim, fontSize: 11, fontFamily: theme.fonts.mono },
  rssi: { color: theme.colors.accent, fontSize: 12, fontFamily: theme.fonts.mono },
  empty: { color: theme.colors.textDim, textAlign: 'center', marginTop: 30 },
  connecting: { color: theme.colors.warn, marginTop: 10, textAlign: 'center' },
});