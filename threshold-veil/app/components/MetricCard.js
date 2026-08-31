// Threshold Veil metric card
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

export default function MetricCard({label, value, accent}) {
  return (
    <View style={[styles.card, {borderColor: accent}]}> 
      <Text style={styles.label}>{label}</Text>
      <Text style={[styles.value, {color: accent}]}>{value}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    backgroundColor: '#122033',
    borderWidth: 1,
    borderRadius: 14,
    padding: 12,
    marginHorizontal: 4,
  },
  label: {
    color: '#adb5bd',
    fontSize: 12,
    textTransform: 'uppercase',
  },
  value: {
    color: '#ffffff',
    fontSize: 20,
    fontWeight: '700',
    marginTop: 6,
  },
});
