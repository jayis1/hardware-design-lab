// Threshold Veil setup screen
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

const STEPS = [
  'Mount the jamb module 8-12 mm from the latch edge.',
  'Close the door and run baseline pressure calibration.',
  'Open the door once so the corridor sample path learns the hallway baseline.',
  'Check threshold strip compression and trim the end caps if needed.',
  'Pair over BLE, then add Wi-Fi only if multi-user sync is desired.',
];

export default function SetupScreen({snapshot}) {
  return (
    <View style={styles.panel}>
      <Text style={styles.heading}>Setup</Text>
      {STEPS.map((step, index) => (
        <View key={step} style={styles.step}>
          <Text style={styles.stepIndex}>{index + 1}</Text>
          <Text style={styles.stepText}>{step}</Text>
        </View>
      ))}
      <Text style={styles.footer}>Current recommendation: {snapshot.recommendation}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  panel: {backgroundColor: '#122033', borderRadius: 16, padding: 16},
  heading: {color: '#f1f3f5', fontSize: 22, fontWeight: '700'},
  step: {flexDirection: 'row', marginTop: 12, alignItems: 'flex-start'},
  stepIndex: {color: '#4dabf7', fontSize: 18, fontWeight: '700', width: 24},
  stepText: {color: '#dee2e6', flex: 1, lineHeight: 20},
  footer: {color: '#adb5bd', marginTop: 16, lineHeight: 20},
});
