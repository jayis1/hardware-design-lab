// Threshold Veil health screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

export default function HealthScreen({snapshot}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Health Check</Text>
      <Text style={styles.copy}>Seal health {snapshot.sealHealthPct}% • Battery {snapshot.batteryPct}%</Text>
      {snapshot.installationChecks.map(item => (
        <View key={item.label} style={styles.row}>
          <Text style={styles.label}>{item.label}</Text>
          <Text style={styles.status}>{item.status}</Text>
        </View>
      ))}
      <Text style={styles.section}>Maintenance guidance</Text>
      <Text style={styles.copy}>Clean the sample inlet lint filter monthly, wipe the threshold strip with mild soap, and re-run latch calibration after any door hardware adjustment.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700'},
  section: {color: '#ffffff', marginTop: 16, fontWeight: '700'},
  copy: {color: '#ced4da', marginTop: 8, lineHeight: 20},
  row: {flexDirection: 'row', justifyContent: 'space-between', marginTop: 12, paddingBottom: 8, borderBottomWidth: 1, borderBottomColor: '#20324d'},
  label: {color: '#dee2e6'},
  status: {color: '#8ce99a', fontWeight: '700'},
});
