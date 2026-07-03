/**
 * NodeDetailScreen.tsx — Live DSD histogram, Z-R plot, energy & time series
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Card } from '../components/Card';
import { useBLE } from '../components/BLEProvider';
import { useDevice, DSD_BIN_LABELS, DSD_BIN_CENTERS } from '../components/DeviceContext';

export default function NodeDetailScreen() {
  const { bleStatus } = useBLE();
  const { currentDSD, history, selectedNodeId } = useDevice();

  if (!bleStatus.connected || !currentDSD) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyText}>
          {selectedNodeId ? `Connecting to ${selectedNodeId}…` : 'Select a node from the Map tab'}
        </Text>
      </View>
    );
  }

  const maxBin = Math.max(...currentDSD.bins, 1);
  const maxRate = Math.max(...history.map(h => h.rainfallRate), 1);

  return (
    <ScrollView style={styles.container}>
      <Card title={`Node: ${bleStatus.deviceName || 'Unknown'}`} accent="#4fc3f7">
        <View style={styles.row}>
          <Text style={styles.metric}>RSSI: {bleStatus.rssi} dBm</Text>
          <Text style={styles.metric}>State: HARVESTING</Text>
        </View>
      </Card>

      {/* Current rainfall metrics */}
      <Card title="Current Rainfall" accent="#66bb6a">
        <View style={styles.metricGrid}>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.rainfallRate.toFixed(2)}</Text>
            <Text style={styles.metricLabel}>R (mm/hr)</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.reflectivity.toFixed(0)}</Text>
            <Text style={styles.metricLabel}>Z (mm⁶/m³)</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.lwc.toFixed(3)}</Text>
            <Text style={styles.metricLabel}>LWC (g/m³)</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.totalDroplets}</Text>
            <Text style={styles.metricLabel}>Droplets</Text>
          </View>
        </View>
      </Card>

      {/* DSD Histogram */}
      <Card title="Drop-Size Distribution (DSD)" accent="#ff9800">
        <View style={styles.histogram}>
          {currentDSD.bins.map((count, i) => (
            <View key={i} style={styles.barRow}>
              <Text style={styles.barLabel}>{DSD_BIN_LABELS[i]}</Text>
              <View style={styles.barContainer}>
                <View style={[styles.bar, {
                  width: `${(count / maxBin) * 100}%`,
                  backgroundColor: count > 0 ? '#4fc3f7' : '#1a2a4a',
                }]} />
              </View>
              <Text style={styles.barCount}>{count}</Text>
            </View>
          ))}
        </View>
        <View style={styles.row}>
          <Text style={styles.metric}>D_m: {currentDSD.meanDiameter.toFixed(2)} mm</Text>
          <Text style={styles.metric}>D₀: {currentDSD.medianDiameter.toFixed(2)} mm</Text>
        </View>
      </Card>

      {/* Z-R Relationship */}
      <Card title="Z-R Relationship" accent="#ce93d8">
        <Text style={styles.formula}>
          Z = {currentDSD.zrA.toFixed(1)} × R^{currentDSD.zrB.toFixed(1)}
        </Text>
        <Text style={styles.hint}>
          Marshall-Palmer default: Z = 200·R^1.6. Deviations indicate
          convective vs stratiform rain.
        </Text>
      </Card>

      {/* Rainfall Rate Time Series (last 24 intervals) */}
      <Card title="Rain Rate History (24 intervals)" accent="#42a5f5">
        <View style={styles.timeseries}>
          {history.map((h, i) => (
            <View key={i} style={[styles.tsBar, {
              height: `${(h.rainfallRate / maxRate) * 100}%`,
              backgroundColor: h.rainfallRate > 2.5 ? '#f44336' :
                               h.rainfallRate > 0.5 ? '#ff9800' : '#4caf50',
            }]} />
          ))}
        </View>
      </Card>

      {/* Energy Harvest Status */}
      <Card title="Energy Harvest" accent="#26c6da">
        <View style={styles.metricGrid}>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.scapVoltage.toFixed(2)}</Text>
            <Text style={styles.metricLabel}>Supercap (V)</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.temperature.toFixed(1)}</Text>
            <Text style={styles.metricLabel}>Temp (°C)</Text>
          </View>
        </View>
        <Text style={styles.hint}>
          TX threshold: 3.30 V · Cold-start: 3.30 V · Low-voltage cutoff: 2.50 V
        </Text>
      </Card>

      {/* Charge Analysis */}
      <Card title="Droplet Charge Analysis" accent="#ab47bc">
        <View style={styles.metricGrid}>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.posCount}</Text>
            <Text style={styles.metricLabel}>Positive</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.negCount}</Text>
            <Text style={styles.metricLabel}>Negative</Text>
          </View>
          <View style={styles.metricCell}>
            <Text style={styles.metricValue}>{currentDSD.avgCharge.toFixed(1)}</Text>
            <Text style={styles.metricLabel}>Avg q (pC)</Text>
          </View>
        </View>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1628', padding: 12 },
  empty: { flex: 1, backgroundColor: '#0a1628', alignItems: 'center', justifyContent: 'center' },
  emptyText: { color: '#5a6a8a', fontSize: 16 },
  row: { flexDirection: 'row', justifyContent: 'space-between' },
  metric: { color: '#a0b0d0', fontSize: 13 },
  metricGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  metricCell: { width: '48%', alignItems: 'center', marginVertical: 8 },
  metricValue: { color: '#e0e0f0', fontSize: 24, fontWeight: '700' },
  metricLabel: { color: '#5a6a8a', fontSize: 11, marginTop: 2 },
  histogram: { marginVertical: 8 },
  barRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 2 },
  barLabel: { width: 70, color: '#8a9ab0', fontSize: 10 },
  barContainer: { flex: 1, height: 16, backgroundColor: '#0a1525', borderRadius: 3, marginHorizontal: 8 },
  bar: { height: '100%', borderRadius: 3 },
  barCount: { width: 30, color: '#a0b0d0', fontSize: 11, textAlign: 'right' },
  formula: { color: '#e0e0f0', fontSize: 18, textAlign: 'center', marginVertical: 8 },
  hint: { color: '#5a6a8a', fontSize: 11, fontStyle: 'italic', marginTop: 4 },
  timeseries: { flexDirection: 'row', alignItems: 'flex-end', height: 80, gap: 2 },
  tsBar: { flex: 1, minWidth: 6, borderRadius: 2 },
});