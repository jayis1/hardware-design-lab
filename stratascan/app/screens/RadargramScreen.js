// RadargramScreen.js — Live B-Scan Radargram Rendering
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//

import React, { useState, useMemo } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, ScrollView, Dimensions } from 'react-native';
import { useBle } from '../utils/BleContext';
import RadargramCanvas from '../components/RadargramCanvas';
import StatusBar from '../components/StatusBar';

export default function RadargramScreen() {
  const ble = useBle();
  const [colorMap, setColorMap] = useState('greyscale');
  const [selectedTraceIdx, setSelectedTraceIdx] = useState(null);
  const [zoom, setZoom] = useState(1);

  const traces = useMemo(() => {
    if (!ble.bscanBuffer || !ble.bscanBuffer.traces) return [];
    return ble.bscanBuffer.traces.filter(t => t != null);
  }, [ble.bscanBuffer]);

  const depths = ble.bscanBuffer?.depths || [];

  if (!ble.connected) {
    return (
      <View style={styles.container}>
        <Text style={styles.empty}>Connect to a StrataScan device to view live radargram.</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      <StatusBar status={ble.status} />

      <View style={styles.toolbar}>
        <Text style={styles.toolbarTitle}>Color Map</Text>
        <TouchableOpacity
          style={[styles.cmButton, colorMap === 'greyscale' && styles.cmSelected]}
          onPress={() => setColorMap('greyscale')}
        >
          <Text style={styles.cmText}>Grey</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.cmButton, colorMap === 'jet' && styles.cmSelected]}
          onPress={() => setColorMap('jet')}
        >
          <Text style={styles.cmText}>Jet</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.cmButton, colorMap === 'seismic' && styles.cmSelected]}
          onPress={() => setColorMap('seismic')}
        >
          <Text style={styles.cmText}>Seismic</Text>
        </TouchableOpacity>
      </View>

      <View style={styles.toolbar}>
        <Text style={styles.toolbarTitle}>Zoom</Text>
        <TouchableOpacity style={styles.cmButton} onPress={() => setZoom(z => Math.max(0.5, z - 0.5))}>
          <Text style={styles.cmText}>−</Text>
        </TouchableOpacity>
        <Text style={styles.zoomText}>{zoom.toFixed(1)}×</Text>
        <TouchableOpacity style={styles.cmButton} onPress={() => setZoom(z => Math.min(4, z + 0.5))}>
          <Text style={styles.cmText}>+</Text>
        </TouchableOpacity>
      </View>

      <RadargramCanvas
        traces={traces}
        depths={depths}
        colorMap={colorMap}
        zoom={zoom}
      />

      <View style={styles.infoCard}>
        <Text style={styles.infoTitle}>Radargram Info</Text>
        <Text style={styles.infoRow}>Traces displayed: {traces.length}</Text>
        <Text style={styles.infoRow}>Depth bins: {depths.length}</Text>
        {depths.length > 0 && (
          <Text style={styles.infoRow}>
            Depth range: 0 – {depths[depths.length - 1].toFixed(2)} m
          </Text>
        )}
        <Text style={styles.infoRow}>Color map: {colorMap}</Text>
        {selectedTraceIdx !== null && (
          <Text style={styles.infoRow}>Selected trace: #{selectedTraceIdx}</Text>
        )}
      </View>

      <Text style={styles.hint}>
        Tap a trace column in the A-Scan tab to inspect its waveform.
        {'\n'}The radargram updates in real time as you drag the antenna.
      </Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0a1a' },
  empty: { color: '#888', textAlign: 'center', marginTop: 100, fontSize: 16 },
  toolbar: {
    flexDirection: 'row', alignItems: 'center', paddingHorizontal: 15,
    paddingVertical: 8, gap: 8, borderBottomWidth: 1, borderBottomColor: '#1a1a2e',
  },
  toolbarTitle: { color: '#0ff', fontSize: 13, fontWeight: 'bold', marginRight: 5 },
  cmButton: {
    backgroundColor: '#16213e', paddingHorizontal: 12, paddingVertical: 4,
    borderRadius: 6, borderWidth: 1, borderColor: '#333',
  },
  cmSelected: { borderColor: '#0066CC', backgroundColor: '#1a3050' },
  cmText: { color: '#ccc', fontSize: 12 },
  zoomText: { color: '#fff', fontSize: 14, fontWeight: 'bold', marginHorizontal: 8 },
  infoCard: {
    backgroundColor: '#16213e', margin: 15, padding: 15, borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
  },
  infoTitle: { color: '#0ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  infoRow: { color: '#ccc', fontSize: 12, marginVertical: 2, fontFamily: 'monospace' },
  hint: { color: '#666', fontSize: 12, textAlign: 'center', margin: 15, lineHeight: 18 },
});