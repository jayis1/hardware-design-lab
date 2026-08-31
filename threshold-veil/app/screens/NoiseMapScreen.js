// Threshold Veil noise map screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

export default function NoiseMapScreen({snapshot}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Noise Map</Text>
      <Text style={styles.copy}>Leak index by recent time slice. Higher bars indicate stronger speech or cart-band coupling through the door perimeter.</Text>
      <View style={styles.graph}>
        {snapshot.leakByHour.map((value, index) => (
          <View key={`bar-${index}`} style={styles.barColumn}>
            <View style={[styles.bar, {height: 24 + value * 36}]} />
            <Text style={styles.barLabel}>{index + 1}</Text>
          </View>
        ))}
      </View>
      <Text style={styles.copy}>Current acoustic bands: low {snapshot.acousticBands[0]} dB, mid {snapshot.acousticBands[1]} dB, high {snapshot.acousticBands[2]} dB.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700'},
  copy: {color: '#ced4da', marginTop: 8, lineHeight: 20},
  graph: {flexDirection: 'row', alignItems: 'flex-end', justifyContent: 'space-between', marginTop: 18},
  barColumn: {alignItems: 'center', flex: 1},
  bar: {width: 18, backgroundColor: '#4dabf7', borderRadius: 9},
  barLabel: {color: '#adb5bd', marginTop: 6, fontSize: 11},
});
