/**
 * DataExportScreen.js — Download CSV/JSON, cloud sync, email.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {useState} from 'react';
import {
  View, ScrollView, Text, StyleSheet, TouchableOpacity, Alert, ActivityIndicator,
} from 'react-native';

const DataExportScreen = () => {
  const [exporting, setExporting] = useState(false);
  const [exportCount] = useState(142);  // Mock count

  const exportAsCSV = () => {
    setExporting(true);
    // Simulate export
    setTimeout(() => {
      setExporting(false);
      Alert.alert('Export Complete', `Exported ${exportCount} records as CSV.`);
    }, 1500);
  };

  const exportAsJSON = () => {
    setExporting(true);
    setTimeout(() => {
      setExporting(false);
      Alert.alert('Export Complete', `Exported ${exportCount} records as JSON.`);
    }, 1500);
  };

  const syncToCloud = () => {
    Alert.alert('Cloud Sync', 'Connect to The Things Network to pull latest data.', [
      {text: 'Cancel', style: 'cancel'},
      {text: 'Connect', onPress: () => Alert.alert('Syncing', 'Fetching data from TTN...')},
    ]);
  };

  const shareData = () => {
    Alert.alert('Share', 'Opening share sheet for data file...');
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.sectionTitle}>Export Options</Text>

      {/* Storage info */}
      <View style={styles.storageCard}>
        <Text style={styles.storageTitle}>Device Storage</Text>
        <View style={styles.storageBarBg}>
          <View style={[styles.storageBarFill, {width: `${(exportCount * 32 * 100) / 15728640}%`}]} />
        </View>
        <Text style={styles.storageText}>
          {exportCount} records • {exportCount * 32} / 15,728,640 bytes used
        </Text>
      </View>

      {/* Export buttons */}
      <TouchableOpacity style={styles.exportButton} onPress={exportAsCSV} disabled={exporting}>
        {exporting ? (
          <ActivityIndicator color="#fff" />
        ) : (
          <>
            <Text style={styles.exportIcon}>📄</Text>
            <View>
              <Text style={styles.exportTitle}>Export as CSV</Text>
              <Text style={styles.exportDesc}>Comma-separated values for Excel, R, Python</Text>
            </View>
          </>
        )}
      </TouchableOpacity>

      <TouchableOpacity style={styles.exportButton} onPress={exportAsJSON} disabled={exporting}>
        <Text style={styles.exportIcon}>📋</Text>
        <View>
          <Text style={styles.exportTitle}>Export as JSON</Text>
          <Text style={styles.exportDesc}>Structured format for web apps and APIs</Text>
        </View>
      </TouchableOpacity>

      <Text style={styles.sectionTitle}>Remote Sync</Text>

      <TouchableOpacity style={styles.exportButton} onPress={syncToCloud}>
        <Text style={styles.exportIcon}>☁️</Text>
        <View>
          <Text style={styles.exportTitle}>Sync to Cloud</Text>
          <Text style={styles.exportDesc}>Pull data from The Things Network (LoRaWAN)</Text>
        </View>
      </TouchableOpacity>

      <TouchableOpacity style={styles.exportButton} onPress={shareData}>
        <Text style={styles.exportIcon}>📤</Text>
        <View>
          <Text style={styles.exportTitle}>Share</Text>
          <Text style={styles.exportDesc}>Send data file via email, messaging, or Airdrop</Text>
        </View>
      </TouchableOpacity>

      {/* Data details */}
      <View style={styles.schemaBox}>
        <Text style={styles.schemaTitle}>Record Format (32 bytes each)</Text>
        <Text style={styles.schemaText}>
          Timestamp, CO₂ (ppm), Flux (µmol/m²/s), Soil T@5/15/30 (°C), VWC (%), Pressure (hPa),
          Air T (°C), PAR (µmol/m²/s), Battery (%), Flags
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {flex: 1, backgroundColor: '#0f0f23', padding: 8},
  sectionTitle: {
    color: '#4CAF50', fontSize: 14, fontWeight: '600',
    textTransform: 'uppercase', letterSpacing: 1.5,
    marginTop: 16, marginBottom: 8, marginLeft: 4,
  },
  storageCard: {
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginVertical: 8,
    borderWidth: 1, borderColor: '#333',
  },
  storageTitle: {color: '#e0e0e0', fontSize: 14, fontWeight: '600', marginBottom: 8},
  storageBarBg: {
    height: 8, backgroundColor: '#333', borderRadius: 4, overflow: 'hidden', marginBottom: 8,
  },
  storageBarFill: {height: 8, backgroundColor: '#4CAF50', borderRadius: 4},
  storageText: {color: '#888', fontSize: 12, fontFamily: 'monospace'},
  exportButton: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', borderRadius: 12, padding: 16, marginVertical: 4,
    borderWidth: 1, borderColor: '#333',
  },
  exportIcon: {fontSize: 28, marginRight: 12},
  exportTitle: {color: '#e0e0e0', fontSize: 16, fontWeight: '600'},
  exportDesc: {color: '#888', fontSize: 12, marginTop: 2},
  schemaBox: {
    backgroundColor: '#1a1a2e', borderRadius: 8, padding: 16, marginVertical: 16,
    borderWidth: 1, borderColor: '#333',
  },
  schemaTitle: {color: '#e0e0e0', fontSize: 13, fontWeight: '600', marginBottom: 8},
  schemaText: {color: '#888', fontSize: 12, lineHeight: 18, fontFamily: 'monospace'},
});

export default DataExportScreen;