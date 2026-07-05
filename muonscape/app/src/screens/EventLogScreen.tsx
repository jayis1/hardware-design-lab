// EventLogScreen.tsx — list of acquisitions, export
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect } from 'react';
import { View, Text, FlatList, TouchableOpacity, StyleSheet, Alert } from 'react-native';
import DeviceService from '../services/DeviceService';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 12 },
  item: { backgroundColor: '#1a1e3a', padding: 12, marginVertical: 4, borderRadius: 6 },
  itemTitle: { color: '#e0e6ff', fontSize: 16, fontWeight: '600' },
  itemDetail: { color: '#7c9eff', fontSize: 12, marginTop: 4 },
  footer: { color: '#3a4060', fontSize: 10, marginTop: 16, textAlign: 'center' },
});

interface Acquisition {
  id: string;
  startTime: number;
  durationSec: number;
  trackCount: number;
  lat: number;
  lon: number;
}

export default function EventLogScreen({ navigation }: any) {
  const [acqs, setAcqs] = useState<Acquisition[]>([]);
  const [status, setStatus] = useState<any>(null);

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => setStatus(s));
    /* In production, query device for the list of saved acquisition files */
    return () => unsub();
  }, []);

  const handleExport = async (acq: Acquisition) => {
    try {
      const data = await DeviceService.downloadFile(`acq_${acq.startTime}.bin`);
      Alert.alert('Exported', `${acq.id}: ${(data.length / 1024).toFixed(1)} KB downloaded.`);
    } catch (e: any) {
      Alert.alert('Export Failed', e.message);
    }
  };

  const renderItem = ({ item }: { item: Acquisition }) => (
    <TouchableOpacity style={styles.item} onPress={() => handleExport(item)}>
      <Text style={styles.itemTitle}>{item.id}</Text>
      <Text style={styles.itemDetail}>
        {(new Date(item.startTime)).toLocaleString()} · {Math.floor(item.durationSec / 60)} min · {item.trackCount} tracks
      </Text>
      <Text style={styles.itemDetail}>
        Location: {item.lat.toFixed(5)}, {item.lon.toFixed(5)}
      </Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Acquisitions</Text>
      {acqs.length === 0 ? (
        <Text style={{ color: '#5a6390' }}>
          No completed acquisitions yet. Events are logged to the device SD card.
          Press "Export" on a completed acquisition to download the track log
          (binary, 40 bytes per muon) for offline reconstruction.
        </Text>
      ) : (
        <FlatList data={acqs} renderItem={renderItem} keyExtractor={(i) => i.id} />
      )}
      <Text style={styles.footer}>MûonScape by jayis1</Text>
    </View>
  );
}