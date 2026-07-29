// src/screens/FieldMapScreen.tsx — Live chlorophyll heatmap over field map
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, FlatList, RefreshControl,
} from 'react-native';
import { Card, Title, Paragraph, Chip, IconButton, Button } from 'react-native-paper';
import { useNavigation } from '@react-navigation/native';
import type { NativeStackNavigationProp } from '@react-navigation/native-stack';
import MapView, { Marker, Circle } from 'react-native-maps';
import type { RootStackParamList } from '../../App';
import { useBle } from '../ble/BleManager';
import { Measurement, interpretSpad } from '../ble/protocol';

const DEFAULT_REGION = {
  latitude: 37.7749,
  longitude: -122.4194,
  latitudeDelta: 0.01,
  longitudeDelta: 0.01,
};

function spadToColor(spad: number): string {
  if (spad < 20) return '#d32f2f'; // red — very low N
  if (spad < 35) return '#f57c00'; // orange — low N
  if (spad < 50) return '#fbc02d'; // yellow — moderate N
  if (spad < 65) return '#689f38'; // light green — sufficient
  return '#2e7d32'; // dark green — high N
}

export default function FieldMapScreen() {
  const navigation = useNavigation<NativeStackNavigationProp<RootStackParamList>>();
  const { connectionState, connect, disconnect, deviceName, status, latestMeasurement, measurements } = useBle();
  const [refreshing, setRefreshing] = useState(false);

  const handleConnect = useCallback(async () => {
    if (connectionState === 'connected') {
      await disconnect();
    } else {
      await connect();
    }
  }, [connectionState, connect, disconnect]);

  const refresh = useCallback(() => {
    setRefreshing(true);
    setTimeout(() => setRefreshing(false), 500);
  }, []);

  const avgSpad = measurements.length > 0
    ? Math.round(measurements.reduce((a, m) => a + m.spad, 0) / measurements.length)
    : 0;

  const lowNCount = measurements.filter((m) => m.spad < 35).length;
  const sufficientNCount = measurements.filter((m) => m.spad >= 50).length;

  const renderMeasurementItem = ({ item }: { item: Measurement }) => {
    const interp = interpretSpad(item.spad);
    return (
      <TouchableOpacity
        onPress={() => navigation.navigate('Measurement', { measurementId: String(item.timestampMs) })}
      >
        <Card style={styles.measCard}>
          <Card.Content style={styles.measRow}>
            <View style={styles.measLeft}>
              <Text style={styles.measSpad}>SPAD {item.spad}</Text>
              <Text style={styles.measMeta}>
                NDVI {item.ndvi.toFixed(3)} · {item.lat.toFixed(5)}, {item.lon.toFixed(5)}
              </Text>
              <Text style={styles.measMeta}>
                {new Date(item.timestampMs).toLocaleTimeString()}
              </Text>
            </View>
            <Chip style={[styles.measChip, { backgroundColor: spadToColor(item.spad) }]}>
              {interp.label}
            </Chip>
          </Card.Content>
        </Card>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      {/* Connection bar */}
      <View style={styles.connectBar}>
        <Button
          mode="contained"
          onPress={handleConnect}
          color={connectionState === 'connected' ? '#2e7d32' : '#666'}
          compact
        >
          {connectionState === 'connected' ? 'Connected' : 'Connect'}
        </Button>
        {deviceName && <Text style={styles.deviceName}>{deviceName}</Text>}
        {status && (
          <Text style={styles.statusText}>
            🔋 {status.battPct}% · 🛰 {status.sats}
          </Text>
        )}
      </View>

      {/* Summary cards */}
      <View style={styles.summaryRow}>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{measurements.length}</Title>
            <Paragraph style={styles.summaryLbl}>Measurements</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={styles.summaryVal}>{avgSpad}</Title>
            <Paragraph style={styles.summaryLbl}>Avg SPAD</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={[styles.summaryVal, { color: '#d32f2f' }]}>{lowNCount}</Title>
            <Paragraph style={styles.summaryLbl}>Low N zones</Paragraph>
          </Card.Content>
        </Card>
        <Card style={styles.summaryCard}>
          <Card.Content>
            <Title style={[styles.summaryVal, { color: '#2e7d32' }]}>{sufficientNCount}</Title>
            <Paragraph style={styles.summaryLbl}>Sufficient N</Paragraph>
          </Card.Content>
        </Card>
      </View>

      {/* Map with measurement markers */}
      <View style={styles.mapContainer}>
        <MapView
          style={styles.map}
          initialRegion={DEFAULT_REGION}
          region={latestMeasurement ? {
            latitude: latestMeasurement.lat,
            longitude: latestMeasurement.lon,
            latitudeDelta: 0.005,
            longitudeDelta: 0.005,
          } : undefined}
        >
          {measurements.map((m, idx) => (
            <Circle
              key={idx}
              center={{ latitude: m.lat, longitude: m.lon }}
              radius={5}
              fillColor={spadToColor(m.spad)}
              strokeColor={spadToColor(m.spad)}
              strokeWidth={1}
            />
          ))}
          {latestMeasurement && (
            <Marker
              coordinate={{ latitude: latestMeasurement.lat, longitude: latestMeasurement.lon }}
              title={`SPAD ${latestMeasurement.spad}`}
              description={`NDVI ${latestMeasurement.ndvi.toFixed(3)}`}
            />
          )}
        </MapView>
      </View>

      {/* Action buttons */}
      <View style={styles.actionRow}>
        <IconButton icon="chart-line" color="#66bb6a" onPress={() => navigation.navigate('LiveSpectrum')} />
        <IconButton icon="history" color="#42a5f5" onPress={() => navigation.navigate('History')} />
        <IconButton icon="tune" color="#ffca28" onPress={() => navigation.navigate('Calibration')} />
        <IconButton icon="cog" color="#b0bec5" onPress={() => navigation.navigate('Settings')} />
      </View>

      {/* Recent measurements list */}
      <Text style={styles.sectionHeader}>Recent Measurements</Text>
      <FlatList
        data={measurements.slice(-10).reverse()}
        keyExtractor={(item, idx) => String(item.timestampMs) + '_' + idx}
        renderItem={renderMeasurementItem}
        refreshControl={<RefreshControl refreshing={refreshing} onRefresh={refresh} />}
        contentContainerStyle={styles.list}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a1f0a', paddingTop: 8 },
  connectBar: { flexDirection: 'row', alignItems: 'center', paddingHorizontal: 12, marginBottom: 4, gap: 12 },
  deviceName: { color: '#66bb6a', fontSize: 14 },
  statusText: { color: '#b0bec5', fontSize: 13, marginLeft: 'auto' },
  summaryRow: { flexDirection: 'row', paddingHorizontal: 8, marginBottom: 4 },
  summaryCard: { flex: 1, backgroundColor: '#152815', marginHorizontal: 4 },
  summaryVal: { color: '#66bb6a', fontSize: 22, textAlign: 'center' },
  summaryLbl: { color: '#81c784', fontSize: 10, textAlign: 'center' },
  mapContainer: { height: 200, marginHorizontal: 8, marginVertical: 4, borderRadius: 8, overflow: 'hidden' },
  map: { flex: 1 },
  actionRow: { flexDirection: 'row', justifyContent: 'center', paddingVertical: 4 },
  sectionHeader: { color: '#e8f5e9', fontSize: 16, fontWeight: 'bold', paddingHorizontal: 16, paddingTop: 4 },
  list: { paddingHorizontal: 8, paddingBottom: 16 },
  measCard: { backgroundColor: '#152815', marginBottom: 8 },
  measRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  measLeft: { flex: 1 },
  measSpad: { color: '#e8f5e9', fontSize: 16, fontWeight: '600' },
  measMeta: { color: '#81c784', fontSize: 12, marginTop: 2 },
  measChip: { borderRadius: 12, paddingHorizontal: 4 },
});