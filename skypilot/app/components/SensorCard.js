/**
 * SkyPilot — Sensor Card Component
 * Reusable card for displaying a sensor value with label and unit
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { Colors, Typography, Spacing } from '../utils/theme';

function SensorCard({ label, value, unit }) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
      <Text style={styles.value}>{value}</Text>
      {unit ? <Text style={styles.unit}>{unit}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: Colors.surface,
    padding: Spacing.sm,
    borderRadius: 8,
    alignItems: 'center',
    minWidth: 80,
    flex: 1,
    marginHorizontal: 2,
  },
  label: {
    ...Typography.caption,
    color: Colors.textSecondary,
    marginBottom: 2,
  },
  value: {
    ...Typography.h4,
    color: Colors.textPrimary,
    fontFamily: 'monospace',
  },
  unit: {
    ...Typography.caption,
    color: Colors.textTertiary,
    fontSize: 10,
  },
});

export { SensorCard };
export default SensorCard;