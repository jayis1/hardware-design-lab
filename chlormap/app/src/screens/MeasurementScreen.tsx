// src/screens/MeasurementScreen.tsx — Detailed measurement view
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Card, Title, Paragraph, Divider } from 'react-native-paper';
import { useRoute } from '@react-navigation/native';
import { useBle } from '../ble/BleManager';
import { interpretSpad, interpretNdvi, interpretLwbi, BAND_WAVELENGTHS } from '../ble/protocol';
import IndexGauge from '../components/IndexGauge';

export default function MeasurementScreen() {
  const route = useRoute<any>();
  const { measurements } = useBle();
  const measId = route.params?.measurementId;

  // Find the measurement by timestamp
  const measurement = measurements.find((m) => String(m.timestampMs) === measId);

  if (!measurement) {
    return (
      <View style={styles.container}>
        <Text style={styles.notFound}>Measurement not found</Text>
      </View>
    );
  }

  const m = measurement;
  const spadInterp = interpretSpad(m.spad);
  const ndviInterp = interpretNdvi(m.ndvi);
  const lwbiInterp = interpretLwbi(m.lwbi);

  // N recommendation based on SPAD + NSI
  const nRecommendation = (() => {
    if (m.spad < 20) return 'Urgent: Apply 80–100 kg N/ha. Severe nitrogen deficiency detected.';
    if (m.spad < 35) return 'Apply 40–60 kg N/ha. Moderate nitrogen deficiency.';
    if (m.spad < 50) return 'Monitor closely. Consider 20–30 kg N/ha supplement.';
    if (m.spad < 65) return 'Nitrogen sufficient. No action needed.';
    return 'High nitrogen. Reduce future N application to avoid lodging.';
  })();

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Timestamp */}
      <Card style={styles.card}>
        <Card.Content>
          <Text style={styles.timestamp}>
            {new Date(m.timestampMs).toLocaleString()}
          </Text>
        </Card.Content>
      </Card>

      {/* Primary indices */}
      <View style={styles.gaugeRow}>
        <IndexGauge label="SPAD" value={m.spad} min={0} max={100} unit="" color="#66bb6a" />
        <IndexGauge label="NDVI" value={m.ndvi} min={-0.2} max={1.0} unit="" color="#42a5f5" />
      </View>
      <View style={styles.gaugeRow}>
        <IndexGauge label="LWBI" value={m.lwbi} min={0.8} max={1.2} unit="" color="#26c6da" />
        <IndexGauge label="NSI" value={m.nsi} min={-0.5} max={0.5} unit="" color="#ffca28" />
      </View>

      {/* Interpretations */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Interpretation</Title>
          <View style={styles.interpRow}>
            <Text style={styles.interpLabel}>Chlorophyll (SPAD):</Text>
            <Text style={[styles.interpVal, { color: spadInterp.color }]}>{spadInterp.label}</Text>
          </View>
          <View style={styles.interpRow}>
            <Text style={styles.interpLabel}>Vegetation (NDVI):</Text>
            <Text style={[styles.interpVal, { color: ndviInterp.color }]}>{ndviInterp.label}</Text>
          </View>
          <View style={styles.interpRow}>
            <Text style={styles.interpLabel}>Hydration (LWBI):</Text>
            <Text style={[styles.interpVal, { color: lwbiInterp.color }]}>{lwbiInterp.label}</Text>
          </View>
          <Divider style={styles.divider} />
          <Text style={styles.recommendationTitle}>Nitrogen Recommendation</Text>
          <Text style={styles.recommendation}>{nRecommendation}</Text>
        </Card.Content>
      </Card>

      {/* GPS */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Location</Title>
          <View style={styles.dataRow}>
            <Text style={styles.dataLabel}>Latitude:</Text>
            <Text style={styles.dataVal}>{m.lat.toFixed(7)}°</Text>
          </View>
          <View style={styles.dataRow}>
            <Text style={styles.dataLabel}>Longitude:</Text>
            <Text style={styles.dataVal}>{m.lon.toFixed(7)}°</Text>
          </View>
          <View style={styles.dataRow}>
            <Text style={styles.dataLabel}>Satellites:</Text>
            <Text style={styles.dataVal}>{m.sats}</Text>
          </View>
        </Card.Content>
      </Card>

      {/* Band details */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Reflectance Bands</Title>
          {m.bands.map((val, i) => (
            <View key={i} style={styles.bandRow}>
              <Text style={styles.bandNm}>{BAND_WAVELENGTHS[i]} nm</Text>
              <View style={styles.barBg}>
                <View style={[styles.barFg, { width: `${Math.min(100, val * 100)}%` }]} />
              </View>
              <Text style={styles.bandVal}>{val.toFixed(4)}</Text>
            </View>
          ))}
        </Card.Content>
      </Card>

      {/* Device info */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Device</Title>
          <View style={styles.dataRow}>
            <Text style={styles.dataLabel}>Battery:</Text>
            <Text style={styles.dataVal}>{m.battMv} mV</Text>
          </View>
          <View style={styles.dataRow}>
            <Text style={styles.dataLabel}>Temperature:</Text>
            <Text style={styles.dataVal}>{m.tempC.toFixed(1)} °C</Text>
          </View>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f0a' },
  content: { padding: 8, paddingBottom: 20 },
  card: { backgroundColor: '#152815', marginBottom: 8 },
  notFound: { color: '#e8f5e9', textAlign: 'center', marginTop: 40, fontSize: 18 },
  timestamp: { color: '#81c784', fontSize: 14, textAlign: 'center' },
  gaugeRow: { flexDirection: 'row', marginBottom: 8 },
  sectionTitle: { color: '#e8f5e9', fontSize: 16, marginBottom: 8 },
  interpRow: { flexDirection: 'row', justifyContent: 'space-between', marginVertical: 4 },
  interpLabel: { color: '#b0bec5', fontSize: 14 },
  interpVal: { fontSize: 14, fontWeight: '600' },
  divider: { marginVertical: 8, backgroundColor: '#2e4d2e' },
  recommendationTitle: { color: '#ffca28', fontSize: 14, fontWeight: 'bold', marginBottom: 4 },
  recommendation: { color: '#e8f5e9', fontSize: 13 },
  dataRow: { flexDirection: 'row', marginVertical: 4 },
  dataLabel: { color: '#b0bec5', fontSize: 14, width: 100 },
  dataVal: { color: '#e8f5e9', fontSize: 14 },
  bandRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 3 },
  bandNm: { color: '#b0bec5', fontSize: 12, width: 70 },
  barBg: { flex: 1, height: 10, backgroundColor: '#0a1f0a', borderRadius: 5, marginHorizontal: 8, overflow: 'hidden' },
  barFg: { height: '100%', backgroundColor: '#66bb6a', borderRadius: 5 },
  bandVal: { color: '#66bb6a', fontSize: 12, width: 55, textAlign: 'right' },
});