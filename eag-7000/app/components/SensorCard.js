/**
 * EAG-7000 — SensorCard Component
 *
 * Displays sensor data for a single I2C mux channel with
 * type, value, and last-updated timestamp.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Text, Card } from 'react-native-paper';

function SensorCard({ channel, label, type, value, unit, timestamp, style }) {
  const isStale = timestamp && (Date.now() - timestamp) > 10000;

  return (
    <Card style={[styles.card, style]}>
      <Card.Content>
        <View style={styles.header}>
          <Text style={styles.channel}>CH{channel}</Text>
          <Text style={styles.label}>{label}</Text>
        </View>
        <View style={styles.body}>
          <Text style={styles.type}>{type}</Text>
          <Text style={[styles.value, { color: isStale ? '#757575' : '#E0E0E0' }]}>
            {value !== null && value !== undefined ? `${value} ${unit}` : '--'}
          </Text>
        </View>
        {timestamp && (
          <Text style={styles.timestamp}>
            Updated: {new Date(timestamp).toLocaleTimeString()}
          </Text>
        )}
      </Card.Content>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1A1A2E',
    marginVertical: 4,
    marginHorizontal: 8,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  channel: {
    color: '#2196F3',
    fontSize: 12,
    fontFamily: 'monospace',
    fontWeight: 'bold',
    width: 32,
  },
  label: {
    color: '#B0B0B0',
    fontSize: 12,
    flex: 1,
  },
  body: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginTop: 4,
  },
  type: {
    color: '#757575',
    fontSize: 12,
  },
  value: {
    fontSize: 16,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  timestamp: {
    color: '#555',
    fontSize: 10,
    marginTop: 4,
  },
});

export default SensorCard;