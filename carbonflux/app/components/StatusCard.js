/**
 * StatusCard.js — Device status summary card component.
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import {View, Text, StyleSheet} from 'react-native';

const STATUS_LABELS = {
  0: 'Power Up',
  1: 'Init',
  2: 'Equilibrating',
  3: 'Closing Lid',
  4: 'Accumulating',
  5: 'Computing Flux',
  6: 'Logging Data',
  7: 'Transmitting',
  8: 'Opening Lid',
  9: 'Sleeping',
  10: 'Cal (Zero)',
  11: 'Cal (Span)',
  12: 'Burst Mode',
  15: 'Error',
};

const StatusCard = ({state, batterySoc, fluxUmol, co2Ppm, measurementCount}) => {
  const getBatteryColor = soc => {
    if (soc > 50) return '#4CAF50';
    if (soc > 20) return '#FFC107';
    return '#F44336';
  };

  const getFluxColor = flux => {
    if (flux > 5) return '#FF9800';
    if (flux > 1) return '#4CAF50';
    return '#2196F3';
  };

  return (
    <View style={styles.card}>
      <View style={styles.header}>
        <Text style={styles.title}>Device Status</Text>
        <View style={[styles.badge, {backgroundColor: getBatteryColor(batterySoc)}]}>
          <Text style={styles.badgeText}>{batterySoc}%</Text>
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.metric}>
          <Text style={styles.label}>State</Text>
          <Text style={styles.value}>
            {STATUS_LABELS[state] || `Unknown (${state})`}
          </Text>
        </View>
        <View style={styles.metric}>
          <Text style={styles.label}>Measurements</Text>
          <Text style={styles.value}>{measurementCount}</Text>
        </View>
      </View>

      <View style={styles.row}>
        <View style={styles.metric}>
          <Text style={styles.label}>CO₂</Text>
          <Text style={styles.value}>{co2Ppm.toFixed(0)} ppm</Text>
        </View>
        <View style={styles.metric}>
          <Text style={styles.label}>Flux</Text>
          <Text style={[styles.value, {color: getFluxColor(fluxUmol)}]}>
            {fluxUmol.toFixed(2)} µmol/m²/s
          </Text>
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    margin: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  title: {
    color: '#e0e0e0',
    fontSize: 18,
    fontWeight: '700',
  },
  badge: {
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 12,
  },
  badgeText: {
    color: '#fff',
    fontSize: 12,
    fontWeight: '700',
  },
  row: {
    flexDirection: 'row',
    marginVertical: 4,
  },
  metric: {
    flex: 1,
  },
  label: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
    letterSpacing: 1,
  },
  value: {
    color: '#e0e0e0',
    fontSize: 16,
    fontWeight: '500',
    fontFamily: 'monospace',
  },
});

export default StatusCard;