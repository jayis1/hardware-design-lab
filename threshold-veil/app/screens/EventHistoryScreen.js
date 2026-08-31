// Threshold Veil event history screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

export default function EventHistoryScreen({snapshot}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Event History</Text>
      {snapshot.events.map(item => (
        <View key={`${item.time}-${item.state}`} style={styles.card}>
          <View style={styles.row}>
            <Text style={styles.state}>{item.state}</Text>
            <Text style={styles.time}>{item.time}</Text>
          </View>
          <Text style={styles.detail}>{item.detail}</Text>
          <Text style={styles.confidence}>Confidence {Math.round(item.confidence * 100)}%</Text>
        </View>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700', marginBottom: 8},
  card: {backgroundColor: '#18283f', borderRadius: 14, padding: 14, marginTop: 10},
  row: {flexDirection: 'row', justifyContent: 'space-between'},
  state: {color: '#74c0fc', fontWeight: '700'},
  time: {color: '#adb5bd'},
  detail: {color: '#dee2e6', marginTop: 6, lineHeight: 20},
  confidence: {color: '#ffd43b', marginTop: 8},
});
