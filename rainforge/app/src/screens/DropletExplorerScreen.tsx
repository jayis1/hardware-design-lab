/**
 * DropletExplorerScreen.tsx — Scrollable list of individual droplet events
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Shows the last 500 individual raindrop events with diameter, velocity,
 * charge and timestamp. Tapping a droplet shows its raw piezo waveform
 * (fetched via BLE on-demand in production).
 */
import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity, FlatList } from 'react-native';
import { Card } from '../components/Card';
import { useDevice, DropletEvent } from '../components/DeviceContext';

function diameterColor(d: number): string {
  if (d < 0.6) return '#4caf50';
  if (d < 1.5) return '#ffeb3b';
  if (d < 2.5) return '#ff9800';
  if (d < 4.0) return '#ff5722';
  return '#f44336';
}

function chargeColor(q: number): string {
  if (q > 1) return '#42a5f5';    // positive → blue
  if (q < -1) return '#ef5350';   // negative → red
  return '#757575';               // neutral → grey
}

export default function DropletExplorerScreen() {
  const { droplets } = useDevice();
  const [selected, setSelected] = useState<DropletEvent | null>(null);

  const renderItem = ({ item }: { item: DropletEvent }) => (
    <TouchableOpacity
      style={styles.dropletRow}
      onPress={() => setSelected(item)}
    >
      <View style={[styles.diameterDot, { backgroundColor: diameterColor(item.diameter) }]} />
      <Text style={styles.dropletD}>{item.diameter.toFixed(2)} mm</Text>
      <Text style={styles.dropletV}>{item.velocity.toFixed(2)} m/s</Text>
      <Text style={[styles.dropletQ, { color: chargeColor(item.charge) }]}>
        {item.charge > 0 ? '+' : ''}{item.charge.toFixed(1)} pC
      </Text>
      <Text style={styles.dropletE}>{item.energy.toFixed(2)} µJ</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Card title={`Droplet Events (${droplets.length})`} accent="#4fc3f7">
        <Text style={styles.hint}>
          Each row is one raindrop impact. Tap for waveform detail.
        </Text>
      </Card>

      {selected && (
        <Card title={`Droplet #${selected.id}`} accent="#ff9800">
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Diameter:</Text>
            <Text style={styles.detailValue}>{selected.diameter.toFixed(3)} mm</Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Velocity:</Text>
            <Text style={styles.detailValue}>{selected.velocity.toFixed(3)} m/s</Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Charge:</Text>
            <Text style={[styles.detailValue, { color: chargeColor(selected.charge) }]}>
              {selected.charge > 0 ? '+' : ''}{selected.charge.toFixed(2)} pC
            </Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Kinetic E:</Text>
            <Text style={styles.detailValue}>{selected.energy.toFixed(3)} µJ</Text>
          </View>
          <View style={styles.waveformPlaceholder}>
            <Text style={styles.waveformText}>
              Piezo waveform (128 samples × 4 channels)
            </Text>
            <Text style={styles.waveformHint}>
              Fetched via BLE characteristic RF04 on demand
            </Text>
          </View>
          <TouchableOpacity onPress={() => setSelected(null)}>
            <Text style={styles.closeBtn}>Close</Text>
          </TouchableOpacity>
        </Card>
      )}

      <FlatList
        data={droplets}
        keyExtractor={item => item.id.toString()}
        renderItem={renderItem}
        style={styles.list}
        ListEmptyComponent={
          <Text style={styles.emptyText}>No droplet events yet. Connect to a node.</Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1628', padding: 12 },
  hint: { color: '#5a6a8a', fontSize: 11, fontStyle: 'italic' },
  list: { flex: 1 },
  dropletRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 10,
    paddingHorizontal: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1a2a4a',
  },
  diameterDot: { width: 12, height: 12, borderRadius: 6, marginRight: 8 },
  dropletD: { color: '#e0e0f0', fontSize: 13, width: 70 },
  dropletV: { color: '#a0b0d0', fontSize: 13, width: 70 },
  dropletQ: { fontSize: 13, width: 70 },
  dropletE: { color: '#5a6a8a', fontSize: 12, flex: 1, textAlign: 'right' },
  detailRow: { flexDirection: 'row', justifyContent: 'space-between', marginVertical: 4 },
  detailLabel: { color: '#5a6a8a', fontSize: 13 },
  detailValue: { color: '#e0e0f0', fontSize: 14, fontWeight: '600' },
  waveformPlaceholder: {
    backgroundColor: '#0a1525',
    borderRadius: 6,
    padding: 20,
    alignItems: 'center',
    marginVertical: 12,
  },
  waveformText: { color: '#4fc3f7', fontSize: 13, marginBottom: 4 },
  waveformHint: { color: '#3a4a6a', fontSize: 10 },
  closeBtn: { color: '#ef5350', fontSize: 14, textAlign: 'center', marginTop: 4 },
  emptyText: { color: '#5a6a8a', fontSize: 14, textAlign: 'center', marginTop: 20 },
});