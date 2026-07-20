/**
 * ExportView.js — Data export screen
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Alert, Switch,
} from 'react-native';
import Share from 'react-native-share';

const Colors = {
  bg: '#0a1628', card: '#162640', accent: '#3b82f6', text: '#e2e8f0',
  textDim: '#94a3b8', success: '#10b981',
};

export function ExportView({ navigation }) {
  const [format, setFormat] = useState('csv');
  const [includeRaw, setIncludeRaw] = useState(true);
  const [includeRecon, setIncludeRecon] = useState(true);
  const [includeEnv, setIncludeEnv] = useState(true);
  const [includeMeta, setIncludeMeta] = useState(true);
  const [exporting, setExporting] = useState(false);

  const formats = [
    { key: 'csv', label: 'CSV (raw frames)', desc: 'Spreadsheet-friendly raw data' },
    { key: 'png', label: 'PNG (reconstruction images)', desc: 'High-res false-color maps' },
    { key: 'netcdf', label: 'NetCDF (volumetric)', desc: '3D stacked depth scans for research' },
    { key: 'json', label: 'JSON (full metadata)', desc: 'Machine-readable with all metadata' },
  ];

  const handleExport = async () => {
    setExporting(true);
    try {
      // In a production app, this would generate the actual file
      // from the collected scan data and trigger the OS share sheet
      const shareOptions = {
        title: 'RootTrace Data Export',
        message: `RootTrace scan data (${format.toUpperCase()}) — jayis1 © 2026`,
        type: 'text/plain',
        filename: `roottrace_export_${Date.now()}.${format === 'netcdf' ? 'nc' : format}`,
      };
      await Share.open(shareOptions);
      Alert.alert('Export', `Data exported as ${format.toUpperCase()}`);
    } catch (e) {
      Alert.alert('Export Error', e.message || 'Failed to export data');
    } finally {
      setExporting(false);
    }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Data Export</Text>
      <Text style={styles.subtitle}>Export scan data in various formats</Text>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Format</Text>
        {formats.map(f => (
          <TouchableOpacity
            key={f.key}
            style={[styles.formatRow, format === f.key && styles.formatRowActive]}
            onPress={() => setFormat(f.key)}>
            <View style={styles.radioOuter}>
              {format === f.key && <View style={styles.radioInner} />}
            </View>
            <View style={styles.formatInfo}>
              <Text style={styles.formatLabel}>{f.label}</Text>
              <Text style={styles.formatDesc}>{f.desc}</Text>
            </View>
          </TouchableOpacity>
        ))}
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Include</Text>
        <ExportToggle label="Raw measurements (208 × complex impedance)" value={includeRaw} onChange={setIncludeRaw} />
        <ExportToggle label="Reconstruction images" value={includeRecon} onChange={setIncludeRecon} />
        <ExportToggle label="Environmental data (T, RH, moisture)" value={includeEnv} onChange={setIncludeEnv} />
        <ExportToggle label="Metadata (GPS, timestamp, plant ID)" value={includeMeta} onChange={setIncludeMeta} />
      </View>

      <View style={styles.card}>
        <Text style={styles.cardTitle}>Preview</Text>
        <Text style={styles.previewText}>
          Format: {format.toUpperCase()}{'\n'}
          Raw data: {includeRaw ? '✓' : '✗'}{'\n'}
          Reconstruction: {includeRecon ? '✓' : '✗'}{'\n'}
          Environmental: {includeEnv ? '✓' : '✗'}{'\n'}
          Metadata: {includeMeta ? '✓' : '✗'}
        </Text>
        <Text style={styles.estimateText}>
          Estimated size: ~{includeRaw ? '4.2 KB' : '0 KB'}
          {includeRecon ? ' + 230 KB' : ''} per frame
        </Text>
      </View>

      <TouchableOpacity
        style={[styles.exportButton, exporting && styles.exportButtonDisabled]}
        onPress={handleExport}
        disabled={exporting}>
        <Text style={styles.exportButtonText}>
          {exporting ? 'Exporting...' : 'Export & Share'}
        </Text>
      </TouchableOpacity>

      <Text style={styles.footer}>© 2026 jayis1 · MIT</Text>
    </View>
  );
}

function ExportToggle({ label, value, onChange }) {
  return (
    <View style={styles.toggleRow}>
      <Text style={styles.toggleLabel}>{label}</Text>
      <Switch
        value={value}
        onValueChange={onChange}
        trackColor={{ false: Colors.card, true: Colors.accent }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.bg, padding: 16 },
  title: { fontSize: 24, fontWeight: 'bold', color: Colors.text },
  subtitle: { fontSize: 14, color: Colors.textDim, marginBottom: 16 },
  card: { backgroundColor: Colors.card, borderRadius: 12, padding: 16, marginBottom: 12 },
  cardTitle: { fontSize: 16, color: Colors.text, fontWeight: 'bold', marginBottom: 12 },
  formatRow: { flexDirection: 'row', alignItems: 'center', padding: 10, borderRadius: 8, marginBottom: 4 },
  formatRowActive: { backgroundColor: 'rgba(59,130,246,0.15)' },
  radioOuter: { width: 18, height: 18, borderRadius: 9, borderWidth: 2, borderColor: Colors.accent, marginRight: 12, alignItems: 'center', justifyContent: 'center' },
  radioInner: { width: 10, height: 10, borderRadius: 5, backgroundColor: Colors.accent },
  formatInfo: { flex: 1 },
  formatLabel: { fontSize: 14, color: Colors.text, fontWeight: '500' },
  formatDesc: { fontSize: 11, color: Colors.textDim, marginTop: 2 },
  toggleRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingVertical: 8 },
  toggleLabel: { fontSize: 13, color: Colors.text, flex: 1 },
  previewText: { fontSize: 13, color: Colors.text, fontFamily: 'monospace', lineHeight: 20 },
  estimateText: { fontSize: 11, color: Colors.textDim, marginTop: 8 },
  exportButton: { backgroundColor: Colors.accent, borderRadius: 10, padding: 16, alignItems: 'center', marginTop: 8 },
  exportButtonDisabled: { opacity: 0.5 },
  exportButtonText: { color: Colors.text, fontSize: 16, fontWeight: 'bold' },
  footer: { fontSize: 11, color: Colors.textDim, textAlign: 'center', paddingVertical: 16 },
});