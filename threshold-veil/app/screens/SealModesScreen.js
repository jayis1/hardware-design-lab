// Threshold Veil seal modes screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, TouchableOpacity, View} from 'react-native';

const MODES = [
  {key: 'AUTO', title: 'Auto', detail: 'Balance comfort, hazard response, and battery life.'},
  {key: 'QUIET', title: 'Quiet', detail: 'Bias the seal toward hallway speech and cart-noise reduction.'},
  {key: 'SHELTER', title: 'Shelter', detail: 'Maximum inward leak resistance for smoke and corridor aerosol events.'},
  {key: 'OPEN_FLOW', title: 'Open Flow', detail: 'Low-compression mode when airflow and easy passage matter more than sealing.'},
];

export default function SealModesScreen({snapshot, mode, setMode}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Seal Modes</Text>
      {MODES.map(item => (
        <TouchableOpacity key={item.key} style={[styles.mode, mode === item.key && styles.active]} onPress={() => setMode(item.key)}>
          <Text style={styles.modeTitle}>{item.title}</Text>
          <Text style={styles.modeDetail}>{item.detail}</Text>
        </TouchableOpacity>
      ))}
      <Text style={styles.footer}>Current pressure target: {snapshot.sealPressure.toFixed(1)} kPa</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700', marginBottom: 8},
  mode: {backgroundColor: '#18283f', borderRadius: 14, padding: 14, marginTop: 10},
  active: {borderWidth: 1, borderColor: '#4dabf7'},
  modeTitle: {color: '#ffffff', fontSize: 17, fontWeight: '700'},
  modeDetail: {color: '#ced4da', marginTop: 4, lineHeight: 20},
  footer: {color: '#adb5bd', marginTop: 14},
});
