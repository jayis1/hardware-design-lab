/**
 * EAG-7000 — StatusBadge Component
 *
 * Displays a colored status indicator with label text.
 * Used across Dashboard, CAN Bus, and Sensors screens.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text } from 'react-native-paper';

function StatusBadge({ label, active, style }) {
  return (
    <View style={[styles.container, style]}>
      <View style={[styles.dot, { backgroundColor: active ? '#4CAF50' : '#F44336' }]} />
      <Text style={[styles.label, { color: active ? '#81C784' : '#EF9A9A' }]}>
        {label}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 2,
    paddingHorizontal: 4,
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 6,
  },
  label: {
    fontSize: 13,
  },
});

export default StatusBadge;