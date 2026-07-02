/**
 * MeshHealthScreen — LoRa mesh link quality and topology view
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Shows per-node RSSI/SNR and hop counts so the installer can place relays.
 */
import React, { useEffect, useState } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { fetchMeshLinks, type MeshLink } from '../api';

export default function MeshHealthScreen() {
  const [links, setLinks] = useState<MeshLink[]>([]);

  useEffect(() => {
    const load = async () => setLinks(await fetchMeshLinks());
    load();
    const interval = setInterval(load, 30000);
    return () => clearInterval(interval);
  }, []);

  const linkQuality = (rssi: number): string => {
    if (rssi > -70) return 'Excellent';
    if (rssi > -90) return 'Good';
    if (rssi > -110) return 'Marginal';
    return 'Poor';
  };
  const linkColour = (rssi: number): string => {
    if (rssi > -70) return '#388e3c';
    if (rssi > -90) return '#4fc3f7';
    if (rssi > -110) return '#f57c00';
    return '#d32f2f';
  };

  const renderItem = ({ item }: { item: MeshLink }) => {
    const c = linkColour(item.rssiDb);
    return (
      <View style={[styles.row, { borderLeftColor: c }]}>
        <View style={styles.rowLeft}>
          <Text style={styles.link}>{item.fromNode} → {item.toNode}</Text>
          <Text style={styles.detail}>RSSI {item.rssiDb} dBm · SNR {item.snrDb} dB · {item.hops} hop(s)</Text>
        </View>
        <Text style={[styles.quality, { color: c }]}>{linkQuality(item.rssiDb)}</Text>
      </View>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Mesh Health</Text>
        <Text style={styles.sub}>{links.length} links active</Text>
      </View>
      <FlatList
        data={links}
        keyExtractor={(l, i) => `${l.fromNode}-${l.toNode}-${i}`}
        renderItem={renderItem}
        ListEmptyComponent={<Text style={styles.empty}>No mesh links. Is the gateway running?</Text>}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0d1a2a' },
  header: { padding: 16, borderBottomWidth: 1, borderBottomColor: '#1a2a3a' },
  title: { color: '#e8f0f8', fontSize: 18, fontWeight: 'bold' },
  sub: { color: '#8a9ab0', fontSize: 13, marginTop: 4 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', backgroundColor: '#142535', marginHorizontal: 12, marginVertical: 4, padding: 12, borderRadius: 8, borderLeftWidth: 4 },
  rowLeft: { flex: 1 },
  link: { color: '#e8f0f8', fontSize: 14, fontWeight: '500' },
  detail: { color: '#8a9ab0', fontSize: 11, marginTop: 4 },
  quality: { fontSize: 12, fontWeight: '600' },
  empty: { color: '#8a9ab0', textAlign: 'center', marginTop: 40, fontSize: 14 },
});