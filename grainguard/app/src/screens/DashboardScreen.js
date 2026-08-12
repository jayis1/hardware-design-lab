/**
 * @file    DashboardScreen.js
 * @brief   Overview of all silos with SRI color-coded status.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useMemo } from 'react';
import { View, Text, FlatList, StyleSheet, TouchableOpacity } from 'react-native';
import { useGrainGuard } from '../services/GrainGuardContext';
import SRIgauge from '../components/SRIgauge';

export default function DashboardScreen() {
  const { probeList, connected, selectSilo } = useGrainGuard();

  const sorted = useMemo(() => {
    return [...probeList].sort((a, b) => (b.sri || 0) - (a.sri || 0));
  }, [probeList]);

  const summary = useMemo(() => {
    let ok = 0, caution = 0, critical = 0;
    probeList.forEach(p => {
      if (p.sri >= 70) critical++;
      else if (p.sri >= 40) caution++;
      else ok++;
    });
    return { ok, caution, critical, total: probeList.length };
  }, [probeList]);

  const renderItem = ({ item }) => {
    const sri = item.sri || 0;
    const statusColor = sri >= 70 ? '#D32F2F' : sri >= 40 ? '#F9A825' : '#388E3C';
    const grainName = item.grainName || 'Unknown';
    const tempMax = item.tmaxOffset ? (item.tmaxOffset - 128) : '--';
    const tempMin = item.tminOffset ? (item.tminOffset - 128) : '--';

    return (
      <TouchableOpacity
        style={[styles.card, { borderLeftColor: statusColor }]}
        onPress={() => selectSilo(item.serial)}
      >
        <View style={styles.cardHeader}>
          <Text style={styles.siloName}>{item.siloName || `Silo ${item.serial}`}</Text>
          <SRIgauge value={sri} size={48} />
        </View>
        <View style={styles.cardBody}>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Grain</Text>
            <Text style={styles.metricValue}>{grainName}</Text>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>CO₂</Text>
            <Text style={styles.metricValue}>{(item.co2PpmX10 || 0) * 10} ppm</Text>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Temp</Text>
            <Text style={styles.metricValue}>{tempMin}–{tempMax}°C</Text>
          </View>
          <View style={styles.metric}>
            <Text style={styles.metricLabel}>Moisture</Text>
            <Text style={styles.metricValue}>{(item.emcX10 || 0) / 10}%</Text>
          </View>
        </View>
        {item.insectId > 0 && (
          <View style={styles.insectBanner}>
            <Text style={styles.insectText}>
              ⚠ Insect activity detected: {insectName(item.insectId)}
            </Text>
          </View>
        )}
        <Text style={styles.lastUpdate}>
          Updated: {timeAgo(item.lastUpdate)}
        </Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.headerBar}>
        <Text style={styles.headerTitle}>GrainGuard Dashboard</Text>
        <View style={[styles.connDot, { backgroundColor: connected ? '#4CAF50' : '#F44336' }]} />
      </View>

      <View style={styles.summaryBar}>
        <View style={styles.summaryItem}>
          <Text style={[styles.summaryNum, { color: '#388E3C' }]}>{summary.ok}</Text>
          <Text style={styles.summaryLabel}>OK</Text>
        </View>
        <View style={styles.summaryItem}>
          <Text style={[styles.summaryNum, { color: '#F9A825' }]}>{summary.caution}</Text>
          <Text style={styles.summaryLabel}>Caution</Text>
        </View>
        <View style={styles.summaryItem}>
          <Text style={[styles.summaryNum, { color: '#D32F2F' }]}>{summary.critical}</Text>
          <Text style={styles.summaryLabel}>Critical</Text>
        </View>
        <View style={styles.summaryItem}>
          <Text style={styles.summaryNum}>{summary.total}</Text>
          <Text style={styles.summaryLabel}>Total</Text>
        </View>
      </View>

      <FlatList
        data={sorted}
        keyExtractor={(item) => String(item.serial)}
        renderItem={renderItem}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No probes registered yet.</Text>
            <Text style={styles.emptyHint}>Tap an NFC tag on a probe to commission it.</Text>
          </View>
        }
      />
    </View>
  );
}

function insectName(id) {
  const names = {
    1: 'Granary Weevil (S. granarius)',
    2: 'Red Flour Beetle (T. castaneum)',
    3: 'Lesser Grain Borer (R. dominica)',
    254: 'Unknown species',
  };
  return names[id] || 'Unknown';
}

function timeAgo(timestamp) {
  if (!timestamp) return 'never';
  const sec = Math.floor((Date.now() - timestamp) / 1000);
  if (sec < 60) return `${sec}s ago`;
  if (sec < 3600) return `${Math.floor(sec / 60)}m ago`;
  if (sec < 86400) return `${Math.floor(sec / 3600)}h ago`;
  return `${Math.floor(sec / 86400)}d ago`;
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  headerBar: {
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
    paddingHorizontal: 16, paddingVertical: 12, backgroundColor: '#1B5E20',
  },
  headerTitle: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  connDot: { width: 12, height: 12, borderRadius: 6 },
  summaryBar: {
    flexDirection: 'row', justifyContent: 'space-around',
    paddingVertical: 12, backgroundColor: '#fff',
    borderBottomWidth: 1, borderBottomColor: '#E0E0E0',
  },
  summaryItem: { alignItems: 'center' },
  summaryNum: { fontSize: 28, fontWeight: 'bold', color: '#333' },
  summaryLabel: { fontSize: 12, color: '#757575', marginTop: 2 },
  list: { padding: 12 },
  card: {
    backgroundColor: '#fff', borderRadius: 8, marginBottom: 12,
    padding: 14, borderLeftWidth: 4,
    shadowColor: '#000', shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.1, shadowRadius: 2, elevation: 2,
  },
  cardHeader: {
    flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center',
    marginBottom: 10,
  },
  siloName: { fontSize: 18, fontWeight: 'bold', color: '#333' },
  cardBody: { flexDirection: 'row', justifyContent: 'space-between' },
  metric: { flex: 1 },
  metricLabel: { fontSize: 11, color: '#9E9E9E', marginBottom: 2 },
  metricValue: { fontSize: 14, fontWeight: '600', color: '#424242' },
  insectBanner: {
    marginTop: 8, padding: 8, backgroundColor: '#FFEB3B',
    borderRadius: 4,
  },
  insectText: { fontSize: 12, color: '#E65100', fontWeight: '600' },
  lastUpdate: { fontSize: 10, color: '#BDBDBD', marginTop: 6, textAlign: 'right' },
  empty: { alignItems: 'center', padding: 40 },
  emptyText: { fontSize: 16, color: '#9E9E9E' },
  emptyHint: { fontSize: 12, color: '#BDBDBD', marginTop: 8 },
});