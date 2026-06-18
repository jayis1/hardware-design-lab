/**
 * ScanScreen.tsx — BLE scan-and-pair with nearby AeroCast units
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React from 'react';
import { View, Text, Button, FlatList, TouchableOpacity, StyleSheet } from 'react-native';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';
import { useDevice } from '../components/DeviceContext';

export default function ScanScreen() {
  const { scanning, devices, scan, connect, connected } = useBLE();
  const { setDeviceId } = useDevice();

  const renderItem = ({ item }) => (
    <TouchableOpacity onPress={async () => { await connect(item); setDeviceId(item.id); }}>
      <Card>
        <Text style={styles.name}>{item.name}</Text>
        <Text style={styles.id}>{item.id}</Text>
        <Text style={styles.rssi}>RSSI {item.rssi} dBm</Text>
      </Card>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>AeroCast Devices</Text>
      <Text style={styles.author}>by jayis1</Text>
      <Button title={scanning ? 'Scanning…' : 'Scan'} onPress={scan} disabled={scanning} />
      <FlatList data={devices} keyExtractor={d => d.id} renderItem={renderItem} />
      {connected && (
        <Card>
          <Text style={styles.connected}>Connected: {connected.name}</Text>
        </Card>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1020', padding: 10 },
  title: { color: '#fff', fontSize: 22, fontWeight: '700', marginTop: 10 },
  author: { color: '#5CB4FF', fontSize: 12, marginBottom: 10 },
  name: { color: '#fff', fontSize: 16, fontWeight: '600' },
  id: { color: '#8aa', fontSize: 12 },
  rssi: { color: '#5CB4FF', fontSize: 12 },
  connected: { color: '#00C7A0', fontSize: 14, fontWeight: '600' },
});