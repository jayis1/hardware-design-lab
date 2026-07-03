/**
 * MapScreen.tsx — Geographic map of deployed RainForge nodes
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Shows a map with all RainForge nodes, color-coded by rainfall rate.
 * Tapping a node navigates to the Node Detail screen.
 */
import React, { useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';
import { useDevice } from '../components/DeviceContext';

// Simulated node locations (would come from backend in production)
const NODE_LOCATIONS = [
  { id: 'rf-node-001', name: 'RainForge-001', lat: 48.137, lon: 11.576, rainRate: 2.3 },
  { id: 'rf-node-002', name: 'RainForge-002', lat: 48.140, lon: 11.580, rainRate: 0.0 },
  { id: 'rf-node-003', name: 'RainForge-003', lat: 48.135, lon: 11.570, rainRate: 5.1 },
];

function rainColor(rate: number): string {
  if (rate === 0) return '#37474f';
  if (rate < 1) return '#4caf50';
  if (rate < 2.5) return '#ffeb3b';
  if (rate < 5) return '#ff9800';
  return '#f44336';
}

export default function MapScreen() {
  const { startScan, devices, scanning } = useBLE();
  const { selectNode } = useDevice();

  useEffect(() => {
    startScan();
  }, []);

  const handleNodePress = (id: string) => {
    selectNode(id);
  };

  return (
    <ScrollView style={styles.container}>
      <Card title="Network Overview" accent="#4fc3f7">
        <Text style={styles.statusText}>
          {scanning ? 'Scanning for RainForge nodes…' : `${NODE_LOCATIONS.length} nodes detected`}
        </Text>
        <Text style={styles.hint}>
          Tap a node to view live DSD, energy harvest, and droplet data.
        </Text>
      </Card>

      <Card title="Node Map" accent="#66bb6a">
        <View style={styles.mapPlaceholder}>
          {NODE_LOCATIONS.map((node) => (
            <TouchableOpacity
              key={node.id}
              style={[styles.nodeDot, { backgroundColor: rainColor(node.rainRate) }]}
              onPress={() => handleNodePress(node.id)}
            >
              <Text style={styles.nodeDotText}>{node.name.slice(-3)}</Text>
            </TouchableOpacity>
          ))}
          <Text style={styles.mapHint}>Map renders with react-native-maps in production</Text>
        </View>
      </Card>

      <Card title="Legend" accent="#ff9800">
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: rainColor(0) }]} />
          <Text style={styles.legendText}>No rain (0 mm/hr)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: rainColor(0.5) }]} />
          <Text style={styles.legendText}>Light (&lt; 1 mm/hr)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: rainColor(2) }]} />
          <Text style={styles.legendText}>Moderate (1-2.5 mm/hr)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: rainColor(4) }]} />
          <Text style={styles.legendText}>Heavy (2.5-5 mm/hr)</Text>
        </View>
        <View style={styles.legendRow}>
          <View style={[styles.legendDot, { backgroundColor: rainColor(6) }]} />
          <Text style={styles.legendText}>Very heavy (&gt; 5 mm/hr)</Text>
        </View>
      </Card>

      <Card title="BLE Devices Nearby" accent="#ce93d8">
        {devices.length === 0 ? (
          <Text style={styles.statusText}>No BLE devices found. Pull to rescan.</Text>
        ) : (
          devices.map(dev => (
            <TouchableOpacity
              key={dev.id}
              style={styles.deviceRow}
              onPress={() => handleNodePress(dev.id)}
            >
              <Text style={styles.deviceName}>{dev.name}</Text>
              <Text style={styles.deviceRssi}>{dev.rssi} dBm</Text>
            </TouchableOpacity>
          ))
        )}
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1628', padding: 12 },
  statusText: { color: '#a0b0d0', fontSize: 14, marginBottom: 4 },
  hint: { color: '#5a6a8a', fontSize: 12, fontStyle: 'italic' },
  mapPlaceholder: {
    height: 200,
    backgroundColor: '#0a1525',
    borderRadius: 6,
    flexDirection: 'row',
    flexWrap: 'wrap',
    alignItems: 'center',
    justifyContent: 'space-around',
    padding: 10,
  },
  nodeDot: {
    width: 44,
    height: 44,
    borderRadius: 22,
    alignItems: 'center',
    justifyContent: 'center',
    elevation: 3,
  },
  nodeDotText: { color: '#fff', fontSize: 10, fontWeight: '700' },
  mapHint: { width: '100%', textAlign: 'center', color: '#3a4a6a', fontSize: 10, marginTop: 8 },
  legendRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 4 },
  legendDot: { width: 16, height: 16, borderRadius: 8, marginRight: 10 },
  legendText: { color: '#a0b0d0', fontSize: 13 },
  deviceRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#1a2a4a',
  },
  deviceName: { color: '#e0e0f0', fontSize: 14 },
  deviceRssi: { color: '#5a6a8a', fontSize: 13 },
});