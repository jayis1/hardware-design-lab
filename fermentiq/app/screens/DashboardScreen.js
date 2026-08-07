/**
 * DashboardScreen.js — Live Fermentation Dashboard
 *
 * Displays real-time sensor values (cell density, CO2, pH, temperature,
 * bubble rate), fermentation phase indicator, ABV estimate, and spoilage
 * risk gauge. Shows the most recent alerts.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, RefreshControl } from 'react-native';
import { Card, Title, Paragraph, IconButton, Badge, Surface, Divider } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import SensorGauge from '../components/SensorGauge';
import { FermenTiqContext } from '../utils/ble';

export default function DashboardScreen() {
  const { connectionState } = useContext(FermenTiqContext);
  const [refreshing, setRefreshing] = useState(false);
  const data = connectionState.liveData;

  const onRefresh = () => {
    setRefreshing(true);
    setTimeout(() => setRefreshing(false), 1000);
  };

  if (!connectionState.connected) {
    return (
      <View style={styles.centerContainer}>
        <Icon name="bluetooth-connect" size={64} color="#666" />
        <Text style={styles.noDeviceText}>No FermenTiq device connected</Text>
        <Text style={styles.hintText}>Go to Settings to scan and connect</Text>
      </View>
    );
  }

  if (!data) {
    return (
      <View style={styles.centerContainer}>
        <Icon name="sync" size={64} color="#666" />
        <Text style={styles.noDeviceText}>Waiting for live data...</Text>
      </View>
    );
  }

  const phase = data.fusion.phaseName;
  const phaseColor = data.fusion.phaseColor;
  const risk = data.fusion.spoilageRisk;
  const health = data.fusion.healthScore;
  const abv = data.fusion.abv;
  const age = data.fusion.batchAgeHours;

  const recentAlerts = connectionState.alerts.slice(0, 3);

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh}
          tintColor="#D4A017" />
      }
    >
      {/* Batch Info Card */}
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.row}>
            <View>
              <Title style={styles.title}>
                {connectionState.config?.batchName || 'Active Batch'}
              </Title>
              <Paragraph style={styles.subtitle}>
                {connectionState.config?.typeName || 'Beer'} • {age}h old
              </Paragraph>
            </View>
            <View style={styles.phaseBadge}>
              <View style={[styles.phaseIndicator, { backgroundColor: phaseColor }]}>
                <Text style={styles.phaseText}>{phase}</Text>
              </View>
            </View>
          </View>
        </Card.Content>
      </Card>

      {/* Health & Risk Card */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Fermentation Health</Title>
          <View style={styles.gaugeRow}>
            <SensorGauge
              label="Health"
              value={health}
              unit="/100"
              color="#4CAF50"
              icon="heart-pulse"
              max={100}
            />
            <SensorGauge
              label="Spoilage Risk"
              value={risk}
              unit="/100"
              color={risk > 60 ? '#E53935' : risk > 30 ? '#FF9800' : '#4CAF50'}
              icon="alert-circle"
              max={100}
            />
            <SensorGauge
              label="ABV"
              value={abv}
              unit="%"
              color="#D4A017"
              icon="glass-wine"
              max={20}
            />
          </View>
        </Card.Content>
      </Card>

      {/* Sensor Readings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.sectionTitle}>Live Sensors</Title>
          <View style={styles.gaugeRow}>
            <SensorGauge
              label="Temperature"
              value={data.temperature.liquidC}
              unit="°C"
              color="#42A5F5"
              icon="thermometer"
              max={40}
            />
            <SensorGauge
              label="pH"
              value={data.ph.value}
              unit=""
              color="#9C27B0"
              icon="water"
              max={14}
            />
            <SensorGauge
              label="CO₂"
              value={data.co2.ppm}
              unit="ppm"
              color="#FF7043"
              icon="molecule-co2"
              max={10000}
            />
          </View>
          <Divider style={styles.divider} />
          <View style={styles.gaugeRow}>
            <SensorGauge
              label="Cell Density"
              value={(data.impedance.cellDensity / 1e6).toFixed(2)}
              unit="M/mL"
              color="#66BB6A"
              icon="bacteria"
              max={500}
            />
            <SensorGauge
              label="CER"
              value={data.co2.cerMmolLh}
              unit="mmol/L/h"
              color="#26A69A"
              icon="trending-up"
              max={10}
            />
            <SensorGauge
              label="Bubbles"
              value={data.acoustic.bubbleRate}
              unit="/min"
              color="#78909C"
              icon="chart-bubble"
              max={120}
            />
          </View>
        </Card.Content>
      </Card>

      {/* Recent Alerts */}
      {recentAlerts.length > 0 && (
        <Card style={styles.card}>
          <Card.Content>
            <Title style={styles.sectionTitle}>Recent Alerts</Title>
            {recentAlerts.map((alert, i) => (
              <View key={i} style={styles.alertRow}>
                <Icon
                  name={alert.severity > 50 ? 'alert' : 'alert-outline'}
                  size={24}
                  color={alert.severity > 50 ? '#E53935' : '#FF9800'}
                />
                <Text style={styles.alertText}>{alert.message}</Text>
              </View>
            ))}
          </Card.Content>
        </Card>
      )}

      <View style={styles.footer}>
        <Text style={styles.footerText}>FermenTiq v1.0 • Author: jayis1</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e' },
  centerContainer: { flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#1a1a2e' },
  card: { marginHorizontal: 12, marginVertical: 6, backgroundColor: '#16213e' },
  title: { color: '#e0e0e0', fontSize: 20 },
  subtitle: { color: '#888', fontSize: 14 },
  sectionTitle: { color: '#e0e0e0', fontSize: 18, marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  phaseBadge: { flexDirection: 'row' },
  phaseIndicator: { paddingHorizontal: 12, paddingVertical: 6, borderRadius: 16 },
  phaseText: { color: '#fff', fontWeight: 'bold', fontSize: 13 },
  gaugeRow: { flexDirection: 'row', justifyContent: 'space-around', paddingVertical: 8 },
  divider: { backgroundColor: '#333', marginVertical: 10 },
  alertRow: { flexDirection: 'row', alignItems: 'center', paddingVertical: 6 },
  alertText: { color: '#e0e0e0', marginLeft: 10, flex: 1 },
  noDeviceText: { color: '#888', fontSize: 18, marginTop: 16 },
  hintText: { color: '#666', fontSize: 14, marginTop: 8 },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 12 },
});