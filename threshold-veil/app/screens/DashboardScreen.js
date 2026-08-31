// Threshold Veil dashboard screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

import MetricCard from '../components/MetricCard';

export default function DashboardScreen({snapshot}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Boundary Dashboard</Text>
      <Text style={styles.copy}>{snapshot.recommendation}</Text>
      <View style={styles.row}>
        <MetricCard label="PM2.5" value={`${snapshot.pm25.toFixed(1)} ug/m3`} accent="#ff8787" />
        <MetricCard label="VOC Delta" value={snapshot.vocDelta.toFixed(1)} accent="#69db7c" />
      </View>
      <View style={styles.row}>
        <MetricCard label="Seal Pressure" value={`${snapshot.sealPressure.toFixed(1)} kPa`} accent="#74c0fc" />
        <MetricCard label="Pressure" value={`${snapshot.pressurePa.toFixed(1)} Pa`} accent="#fcc419" />
      </View>
      <Text style={styles.section}>What the model sees</Text>
      <Text style={styles.bullet}>• Corridor-to-apartment gradient is the main signal.</Text>
      <Text style={styles.bullet}>• Acoustic bands estimate speech and cart noise leakage.</Text>
      <Text style={styles.bullet}>• Seal pressure only rises when the boundary needs help.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700'},
  copy: {color: '#ced4da', marginTop: 8, lineHeight: 21},
  row: {flexDirection: 'row', marginTop: 12},
  section: {color: '#ffffff', fontWeight: '700', marginTop: 16, marginBottom: 6},
  bullet: {color: '#dee2e6', lineHeight: 20, marginTop: 2},
});
