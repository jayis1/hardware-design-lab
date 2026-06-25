/**
 * PodDetailScreen.js — Detailed view of a single pod
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, TouchableOpacity } from 'react-native';
import { useSelector } from 'react-redux';
import { SVIGauge } from '../components/SVIGauge';
import { TrendsChart } from '../components/TrendsChart';
import { DepthProfile } from '../components/DepthProfile';

export default function PodDetailScreen({ route, navigation }) {
  const { podId } = route.params;
  const pods = useSelector(state => state.pods.pods);
  const pod = pods.find(p => p.id === podId);

  const [sviHistory, setSviHistory] = useState([]);
  const [envData, setEnvData] = useState({
    moisture: [28.5, 22.1],
    ec: [125, 98],
    depthTemp: [18.2, 16.8, 14.1, 12.3],
    co2: 680,
    batteryMv: 3650,
  });

  useEffect(() => {
    // Generate demo SVI history
    const history = [];
    for (let i = 0; i < 48; i++) {
      history.push({ ts: Date.now() - (48 - i) * 1800000, svi: 50 + Math.sin(i / 5) * 15 + Math.random() * 8 });
    }
    setSviHistory(history);
  }, [podId]);

  if (!pod) {
    return (
      <View style={styles.container}>
        <Text style={styles.errorText}>Pod not found</Text>
      </View>
    );
  }

  return (
    <ScrollView style={styles.container}>
      {/* Pod header */}
      <View style={styles.header}>
        <Text style={styles.podName}>{pod.name || pod.id}</Text>
        <Text style={styles.podId}>ID: {pod.id}</Text>
      </View>

      {/* SVI Gauge */}
      <View style={styles.sviSection}>
        <Text style={styles.sectionTitle}>Current Soil Vitality Index</Text>
        <SVIGauge value={pod.svi || 0} size={160} />
        <Text style={styles.sviValue}>{pod.svi || 0}/100</Text>
      </View>

      {/* SVI Trend */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>SVI Trend (24h)</Text>
        <TrendsChart pods={[pod]} days={1} data={sviHistory} />
      </View>

      {/* Environmental data */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Environmental Sensors</Text>
        <View style={styles.envGrid}>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>Moisture (10cm)</Text>
            <Text style={styles.envValue}>{envData.moisture[0]}%</Text>
          </View>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>Moisture (40cm)</Text>
            <Text style={styles.envValue}>{envData.moisture[1]}%</Text>
          </View>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>EC (10cm)</Text>
            <Text style={styles.envValue}>{envData.ec[0]} µS/cm</Text>
          </View>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>EC (40cm)</Text>
            <Text style={styles.envValue}>{envData.ec[1]} µS/cm</Text>
          </View>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>Soil CO₂</Text>
            <Text style={styles.envValue}>{envData.co2} ppm</Text>
          </View>
          <View style={styles.envCard}>
            <Text style={styles.envLabel}>Battery</Text>
            <Text style={styles.envValue}>{((envData.batteryMv - 3200) / (3670 - 3200) * 100).toFixed(0)}%</Text>
            <Text style={styles.envSubtext}>{envData.batteryMv} mV</Text>
          </View>
        </View>
      </View>

      {/* Depth temperature profile */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Depth Temperature Profile</Text>
        <DepthProfile temps={envData.depthTemp} depths={[10, 20, 40, 60]} />
      </View>

      {/* Actions */}
      <View style={styles.actionsSection}>
        <TouchableOpacity
          style={styles.actionButton}
          onPress={() => navigation.navigate('Soundscape')}
        >
          <Text style={styles.actionButtonText}>View Soundscape</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, { backgroundColor: '#388E3C' }]}
          onPress={() => navigation.navigate('Calibration')}
        >
          <Text style={styles.actionButtonText}>Recalibrate</Text>
        </TouchableOpacity>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#FAFAFA' },
  header: { padding: 20, backgroundColor: '#2E7D32', alignItems: 'center' },
  podName: { fontSize: 22, fontWeight: 'bold', color: '#FFFFFF' },
  podId: { fontSize: 12, color: '#E8F5E9', marginTop: 4 },
  sviSection: { alignItems: 'center', padding: 20, backgroundColor: '#FFFFFF', margin: 10, borderRadius: 12, elevation: 2 },
  section: { padding: 15, margin: 10, backgroundColor: '#FFFFFF', borderRadius: 12, elevation: 1 },
  sectionTitle: { fontSize: 16, fontWeight: 'bold', color: '#424242', marginBottom: 10 },
  sviValue: { fontSize: 28, fontWeight: 'bold', color: '#2E7D32', marginTop: 8 },
  envGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  envCard: { width: '48%', padding: 12, backgroundColor: '#F5F5F5', borderRadius: 8, marginBottom: 8 },
  envLabel: { fontSize: 11, color: '#757575' },
  envValue: { fontSize: 18, fontWeight: 'bold', color: '#424242', marginTop: 3 },
  envSubtext: { fontSize: 10, color: '#9E9E9E', marginTop: 2 },
  actionsSection: { padding: 15, margin: 10 },
  actionButton: { backgroundColor: '#2E7D32', padding: 14, borderRadius: 8, marginTop: 8, alignItems: 'center' },
  actionButtonText: { color: '#FFFFFF', fontSize: 14, fontWeight: 'bold' },
  errorText: { fontSize: 16, color: '#F44336', textAlign: 'center', marginTop: 50 },
});