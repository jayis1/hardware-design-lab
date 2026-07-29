// src/screens/LiveSpectrumScreen.tsx — Real-time 16-band reflectance spectrum
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Card, Title, Paragraph } from 'react-native-paper';
import { useBle } from '../ble/BleManager';
import { BAND_WAVELENGTHS } from '../ble/protocol';
import SpectrumChart from '../components/SpectrumChart';
import IndexGauge from '../components/IndexGauge';

export default function LiveSpectrumScreen() {
  const { latestMeasurement, connectionState } = useBle();

  if (!latestMeasurement) {
    return (
      <View style={styles.container}>
        <Card style={styles.card}>
          <Card.Content>
            <Title style={styles.noData}>No live data</Title>
            <Paragraph style={styles.noDataSub}>
              {connectionState === 'connected'
                ? 'Press the trigger on the device to take a measurement'
                : 'Connect to the ChloroMap device first'}
            </Paragraph>
          </Card.Content>
        </Card>
      </View>
    );
  }

  const m = latestMeasurement;

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Indices gauges */}
      <View style={styles.gaugeRow}>
        <IndexGauge label="SPAD" value={m.spad} min={0} max={100} unit="" color="#66bb6a" />
        <IndexGauge label="NDVI" value={m.ndvi} min={-0.2} max={1.0} unit="" color="#42a5f5" />
        <IndexGauge label="NSI" value={m.nsi} min={-0.5} max={0.5} unit="" color="#ffca28" />
      </View>
      <View style={styles.gaugeRow}>
        <IndexGauge label="LWBI" value={m.lwbi} min={0.8} max={1.2} unit="" color="#26c6da" />
        <IndexGauge label="Red Edge" value={m.rededge} min={0} max={20} unit="/nm" color="#ab47bc" />
        <IndexGauge label="Temp" value={m.tempC} min={0} max={50} unit="°C" color="#ff7043" />
      </View>

      {/* Spectrum chart */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Reflectance Spectrum (8 bands)</Title>
          <SpectrumChart bands={m.bands} wavelengths={BAND_WAVELENGTHS} />
        </Card.Content>
      </Card>

      {/* Band data table */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Band Details</Title>
          {m.bands.map((val, i) => (
            <View key={i} style={styles.bandRow}>
              <Text style={styles.bandNm}>{BAND_WAVELENGTHS[i]} nm</Text>
              <View style={styles.barContainer}>
                <View
                  style={[
                    styles.barFill,
                    { width: `${Math.min(100, val * 100)}%`, backgroundColor: '#66bb6a' },
                  ]}
                />
              </View>
              <Text style={styles.bandVal}>{val.toFixed(3)}</Text>
            </View>
          ))}
        </Card.Content>
      </Card>

      {/* GPS info */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>GPS</Title>
          <View style={styles.gpsRow}>
            <Text style={styles.gpsLabel}>Lat:</Text>
            <Text style={styles.gpsVal}>{m.lat.toFixed(7)}°</Text>
          </View>
          <View style={styles.gpsRow}>
            <Text style={styles.gpsLabel}>Lon:</Text>
            <Text style={styles.gpsVal}>{m.lon.toFixed(7)}°</Text>
          </View>
          <View style={styles.gpsRow}>
            <Text style={styles.gpsLabel}>Satellites:</Text>
            <Text style={styles.gpsVal}>{m.sats}</Text>
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
  noData: { color: '#e8f5e9', textAlign: 'center', fontSize: 20 },
  noDataSub: { color: '#81c784', textAlign: 'center', marginTop: 8 },
  gaugeRow: { flexDirection: 'row', marginBottom: 8 },
  sectionTitle: { color: '#e8f5e9', fontSize: 16, marginBottom: 8 },
  bandRow: { flexDirection: 'row', alignItems: 'center', marginVertical: 3 },
  bandNm: { color: '#b0bec5', fontSize: 12, width: 70 },
  barContainer: { flex: 1, height: 12, backgroundColor: '#0a1f0a', borderRadius: 6, marginHorizontal: 8, overflow: 'hidden' },
  barFill: { height: '100%', borderRadius: 6 },
  bandVal: { color: '#66bb6a', fontSize: 12, width: 50, textAlign: 'right' },
  gpsRow: { flexDirection: 'row', marginVertical: 4 },
  gpsLabel: { color: '#b0bec5', fontSize: 14, width: 100 },
  gpsVal: { color: '#e8f5e9', fontSize: 14 },
});