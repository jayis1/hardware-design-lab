/**
 * HistoryScreen.js — Scan history and log viewer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity, Alert, Share,
} from 'react-native';
import { useBle } from '../ble/BleContext';
import Colors from '../constants/Colors';

const HistoryScreen = () => {
  const { scanHistory, clearHistory } = useBle();
  const [selectedEntry, setSelectedEntry] = useState(null);

  const formatTimestamp = (ts) => {
    const d = new Date(ts * 1000);
    const date = d.toLocaleDateString();
    const time = d.toLocaleTimeString();
    return `${date} ${time}`;
  };

  const getConfidenceColor = (conf) => {
    if (conf >= 0.7) return Colors.highConfidence;
    if (conf >= 0.4) return Colors.mediumConfidence;
    return Colors.lowConfidence;
  };

  const exportHistory = async () => {
    if (scanHistory.length === 0) {
      Alert.alert('No Data', 'No scans to export');
      return;
    }

    // Build CSV
    const header = 'Timestamp,Alloy,Confidence,Liftoff_mm,Alternatives\n';
    const rows = scanHistory.map(s => {
      const ts = formatTimestamp(s.timestamp);
      const alts = (s.alternatives || []).join('; ');
      return `"${ts}","${s.alloy || 'Unknown'}",${(s.confidence || 0).toFixed(2)},${(s.liftoff || 0).toFixed(1)},"${alts}"`;
    }).join('\n');

    try {
      await Share.share({
        message: header + rows,
        title: 'AlloyScan History Export',
      });
    } catch (e) {
      Alert.alert('Export Failed', e.message);
    }
  };

  const confirmClear = () => {
    Alert.alert(
      'Clear History',
      `Delete all ${scanHistory.length} scan records?`,
      [
        { text: 'Cancel', style: 'cancel' },
        { text: 'Delete', style: 'destructive', onPress: clearHistory },
      ]
    );
  };

  const renderItem = ({ item, index }) => (
    <TouchableOpacity
      style={styles.historyItem}
      onPress={() => setSelectedEntry(item)}
    >
      <View style={styles.itemHeader}>
        <Text style={styles.itemIndex}>#{index + 1}</Text>
        <Text style={styles.itemTime}>{formatTimestamp(item.timestamp)}</Text>
      </View>
      <View style={styles.itemBody}>
        <Text style={styles.itemAlloy}>{item.alloy || 'Unknown'}</Text>
        <View style={[styles.confidenceDot, { backgroundColor: getConfidenceColor(item.confidence) }]} />
      </View>
      <View style={styles.itemDetails}>
        <Text style={styles.itemConfidence}>
          Confidence: {((item.confidence || 0) * 100).toFixed(0)}%
        </Text>
        {item.liftoff != null && (
          <Text style={styles.itemLiftoff}>Lift-off: {item.liftoff.toFixed(1)}mm</Text>
        )}
      </View>
    </TouchableOpacity>
  );

  if (selectedEntry) {
    return (
      <View style={styles.container}>
        <View style={styles.detailHeader}>
          <TouchableOpacity onPress={() => setSelectedEntry(null)}>
            <Text style={styles.backButton}>← Back</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.detailContent}>
          <Text style={styles.detailAlloy}>{selectedEntry.alloy || 'Unknown'}</Text>
          <Text style={styles.detailTime}>{formatTimestamp(selectedEntry.timestamp)}</Text>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Confidence</Text>
            <Text style={styles.detailValue}>
              {((selectedEntry.confidence || 0) * 100).toFixed(1)}%
            </Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Lift-off</Text>
            <Text style={styles.detailValue}>
              {selectedEntry.liftoff ? `${selectedEntry.liftoff.toFixed(2)} mm` : 'N/A'}
            </Text>
          </View>
          {selectedEntry.alternatives && selectedEntry.alternatives.length > 0 && (
            <View style={styles.detailAlternatives}>
              <Text style={styles.detailSectionTitle}>Closest Alternatives</Text>
              {selectedEntry.alternatives.map((alt, i) => (
                <Text key={i} style={styles.detailAlt}>• {alt}</Text>
              ))}
            </View>
          )}
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.toolbar}>
        <Text style={styles.countText}>{scanHistory.length} scans logged</Text>
        <View style={styles.toolbarButtons}>
          <TouchableOpacity style={styles.toolButton} onPress={exportHistory}>
            <Text style={styles.toolButtonText}>Export</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.toolButton, styles.clearButton]}
            onPress={confirmClear}
          >
            <Text style={styles.toolButtonText}>Clear</Text>
          </TouchableOpacity>
        </View>
      </View>

      {scanHistory.length === 0 ? (
        <View style={styles.emptyState}>
          <Text style={styles.emptyTitle}>No Scans Yet</Text>
          <Text style={styles.emptyText}>
            Scan results from your AlloyScan device will appear here.
          </Text>
        </View>
      ) : (
        <FlatList
          data={[...scanHistory].reverse()}
          keyExtractor={(item, index) => `${item.timestamp}_${index}`}
          renderItem={renderItem}
          contentContainerStyle={styles.list}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.dark },
  toolbar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  countText: { color: Colors.gray, fontSize: 14 },
  toolbarButtons: { flexDirection: 'row' },
  toolButton: {
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 8,
    marginLeft: 8,
    backgroundColor: Colors.primaryLight,
  },
  clearButton: { backgroundColor: Colors.red },
  toolButtonText: { color: Colors.white, fontSize: 13, fontWeight: 'bold' },
  list: { padding: 12 },
  historyItem: {
    backgroundColor: Colors.darkGray,
    borderRadius: 10,
    padding: 12,
    marginBottom: 8,
  },
  itemHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 6,
  },
  itemIndex: { color: Colors.gray, fontSize: 12 },
  itemTime: { color: Colors.gray, fontSize: 12 },
  itemBody: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  itemAlloy: { color: Colors.white, fontSize: 18, fontWeight: 'bold' },
  confidenceDot: { width: 12, height: 12, borderRadius: 6 },
  itemDetails: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 6,
  },
  itemConfidence: { color: Colors.gray, fontSize: 12 },
  itemLiftoff: { color: Colors.gray, fontSize: 12 },
  emptyState: { flex: 1, justifyContent: 'center', alignItems: 'center', padding: 40 },
  emptyTitle: { color: Colors.white, fontSize: 22, fontWeight: 'bold', marginBottom: 12 },
  emptyText: { color: Colors.gray, fontSize: 14, textAlign: 'center' },
  detailHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  backButton: { color: Colors.accent, fontSize: 16 },
  detailContent: { padding: 20 },
  detailAlloy: { color: Colors.white, fontSize: 28, fontWeight: 'bold', marginBottom: 4 },
  detailTime: { color: Colors.gray, fontSize: 14, marginBottom: 20 },
  detailRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  detailLabel: { color: Colors.gray, fontSize: 14 },
  detailValue: { color: Colors.white, fontSize: 14, fontWeight: 'bold' },
  detailAlternatives: { marginTop: 20 },
  detailSectionTitle: { color: Colors.lightGray, fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  detailAlt: { color: Colors.lightGray, fontSize: 14, paddingVertical: 2 },
});

export default HistoryScreen;