/**
 * NeuroLink Channel Card Component
 * Displays individual channel signal quality, impedance, and waveform preview
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function ChannelCard({
  channelNumber,
  label = '',
  value = 0,
  impedance = 0,
  quality = 100,
  gain = 6,
  isActive = true,
  unit = 'µV',
}) {
  const qualityColor = quality >= 80 ? '#4CAF50' : quality >= 50 ? '#FFC107' : '#F44336';
  const impColor = impedance < 5 ? '#4CAF50' : impedance < 10 ? '#FFC107' : '#F44336';

  return (
    <View style={[styles.card, !isActive && styles.inactive]}>
      <View style={styles.header}>
        <Text style={styles.channelNum}>CH {channelNumber}</Text>
        <Text style={styles.label}>{label}</Text>
        <View style={[styles.qualityDot, { backgroundColor: qualityColor }]} />
      </View>

      <View style={styles.dataRow}>
        <View style={styles.dataItem}>
          <Text style={styles.dataLabel}>Signal</Text>
          <Text style={styles.dataValue}>
            {value.toFixed(1)} <Text style={styles.dataUnit}>{unit}</Text>
          </Text>
        </View>
        <View style={styles.dataItem}>
          <Text style={styles.dataLabel}>Impedance</Text>
          <Text style={[styles.dataValue, { color: impColor }]}>
            {impedance.toFixed(1)} kΩ
          </Text>
        </View>
        <View style={styles.dataItem}>
          <Text style={styles.dataLabel}>Gain</Text>
          <Text style={styles.dataValue}>×{gain}</Text>
        </View>
      </View>

      {/* Signal quality bar */}
      <View style={styles.qualityBar}>
        <View
          style={[
            styles.qualityFill,
            { width: `${quality}%`, backgroundColor: qualityColor },
          ]}
        />
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#1E2A45',
    borderRadius: 12,
    padding: 14,
    marginVertical: 6,
    marginHorizontal: 4,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  inactive: {
    opacity: 0.4,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  channelNum: {
    color: '#4FC3F7',
    fontSize: 14,
    fontWeight: '700',
    fontFamily: 'monospace',
    marginRight: 8,
  },
  label: {
    color: '#B0BEC5',
    fontSize: 12,
    flex: 1,
  },
  qualityDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
  },
  dataRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  dataItem: {
    flex: 1,
  },
  dataLabel: {
    color: '#607D8B',
    fontSize: 10,
    textTransform: 'uppercase',
  },
  dataValue: {
    color: '#E8E8E8',
    fontSize: 15,
    fontWeight: '600',
    fontFamily: 'monospace',
  },
  dataUnit: {
    color: '#78909C',
    fontSize: 11,
  },
  qualityBar: {
    height: 3,
    backgroundColor: '#2A2A4A',
    borderRadius: 1.5,
    marginTop: 10,
    overflow: 'hidden',
  },
  qualityFill: {
    height: '100%',
    borderRadius: 1.5,
  },
});