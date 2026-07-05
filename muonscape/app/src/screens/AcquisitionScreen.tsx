// AcquisitionScreen.tsx — start/stop, live track rate, time remaining
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect } from 'react';
import { View, Text, Button, TextInput, StyleSheet, Alert } from 'react-native';
import DeviceService from '../services/DeviceService';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 16 },
  label: { color: '#7c9eff', fontSize: 14, marginTop: 12 },
  value: { color: '#e0e6ff', fontSize: 16, marginBottom: 4 },
  input: { backgroundColor: '#1a1e3a', color: '#e0e6ff', borderWidth: 1,
           borderColor: '#3a4060', borderRadius: 4, padding: 8, marginVertical: 4 },
  gauge: { width: 200, height: 200, borderRadius: 100, borderWidth: 4,
           borderColor: '#7c9eff', alignItems: 'center', justifyContent: 'center',
           marginVertical: 16, alignSelf: 'center' },
  gaugeValue: { color: '#e0e6ff', fontSize: 36, fontWeight: '700' },
  gaugeLabel: { color: '#5a6390', fontSize: 12 },
  row: { flexDirection: 'row', justifyContent: 'space-between', marginVertical: 4 },
});

export default function AcquisitionScreen({ navigation }: any) {
  const [status, setStatus] = useState<any>(null);
  const [duration, setDuration] = useState('3600');
  const [target, setTarget] = useState('wall');
  const [distance, setDistance] = useState('200');

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => setStatus(s));
    return () => unsub();
  }, []);

  const handleStart = async () => {
    try {
      await DeviceService.startAcquisition(
        parseInt(duration, 10), target, parseInt(distance, 10)
      );
      Alert.alert('Acquisition Started', `${duration}s on target "${target}"`);
    } catch (e: any) { Alert.alert('Error', e.message); }
  };

  const handleStop = async () => {
    try {
      const r = await DeviceService.stopAcquisition();
      Alert.alert('Stopped', `Tracks: ${r.tracks}`);
    } catch (e: any) { Alert.alert('Error', e.message); }
  };

  const isAcquiring = status && status.state === 'acquiring';
  const trackRate = status ? status.tracks / Math.max(1, status.elapsed_s) : 0;

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Acquisition</Text>

      <View style={styles.gauge}>
        <Text style={styles.gaugeValue}>{trackRate.toFixed(1)}</Text>
        <Text style={styles.gaugeLabel}>tracks/sec</Text>
      </View>

      <Text style={styles.label}>Duration (seconds)</Text>
      <TextInput style={styles.input} value={duration}
        onChangeText={setDuration} keyboardType="numeric" />
      <Text style={styles.label}>Target type</Text>
      <TextInput style={styles.input} value={target} onChangeText={setTarget} />
      <Text style={styles.label}>Distance (cm)</Text>
      <TextInput style={styles.input} value={distance}
        onChangeText={setDistance} keyboardType="numeric" />

      <View style={{ flexDirection: 'row', marginTop: 16 }}>
        <Button title={isAcquiring ? 'Stop' : 'Start'} onPress={isAcquiring ? handleStop : handleStart} />
        <View style={{ width: 8 }} />
        <Button title="Preview Image" onPress={() => navigation.navigate('Image')} />
      </View>

      {status && (
        <View style={{ marginTop: 16 }}>
          <View style={styles.row}>
            <Text style={styles.value}>State:</Text>
            <Text style={styles.value}>{status.state}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.value}>Elapsed:</Text>
            <Text style={styles.value}>{Math.floor(status.elapsed_s / 60)} min</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.value}>Remaining:</Text>
            <Text style={styles.value}>{Math.floor(status.remain_s / 60)} min</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.value}>Tracks:</Text>
            <Text style={styles.value}>{status.tracks}</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.value}>Battery:</Text>
            <Text style={styles.value}>{status.soc}%</Text>
          </View>
          <View style={styles.row}>
            <Text style={styles.value}>Power:</Text>
            <Text style={styles.value}>{status.current_ma} mA</Text>
          </View>
        </View>
      )}
    </View>
  );
}