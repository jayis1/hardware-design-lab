/**
 * TriggerControls.js — Reusable trigger configuration component
 * Provides trigger level slider, channel selector, and type selector
 */

import React from 'react';
import { View, StyleSheet } from 'react-native';
import { Slider, Text, ToggleButton } from 'react-native-paper';

const TRIGGER_TYPES = [
  { value: 'rising', label: '↑ Rise' },
  { value: 'falling', label: '↓ Fall' },
  { value: 'both', label: '↕ Both' },
  { value: 'pulse', label: '⊢ Pulse' },
  { value: 'timeout', label: '⏱ Timeout' },
  { value: 'runt', label: '≀ Runt' },
];

const TRIGGER_CHANNELS = [
  { value: 0, label: 'CH1', color: '#FFD700' },
  { value: 1, label: 'CH2', color: '#00FF00' },
  { value: 2, label: 'CH3', color: '#FF4444' },
  { value: 3, label: 'CH4', color: '#4488FF' },
  { value: 4, label: 'D1', color: '#FF00FF' },
  { value: 5, label: 'D2', color: '#00FFFF' },
];

export default function TriggerControls({
  triggerLevel,
  triggerChannel,
  triggerType,
  onTriggerLevelChange,
  onTriggerChannelChange,
  onTriggerTypeChange,
}) {
  // Convert 8-bit trigger level to voltage display
  const voltage = ((triggerLevel / 255) * 20).toFixed(2);  // ±20V range → 0-20V display

  return (
    <View style={styles.container}>
      {/* Trigger channel selector */}
      <View style={styles.row}>
        <Text style={styles.label}>Source:</Text>
        <View style={styles.channelRow}>
          {TRIGGER_CHANNELS.map((ch) => (
            <ToggleButton
              key={ch.value}
              icon={() => <Text style={{ color: triggerChannel === ch.value ? ch.color : '#888', fontSize: 10 }}>{ch.label}</Text>}
              value={String(ch.value)}
              status={triggerChannel === ch.value ? 'checked' : 'unchecked'}
              onPress={() => onTriggerChannelChange(ch.value)}
              style={[styles.chip, triggerChannel === ch.value && { borderColor: ch.color }]}
            />
          ))}
        </View>
      </View>

      {/* Trigger type selector */}
      <View style={styles.row}>
        <Text style={styles.label}>Type:</Text>
        <View style={styles.typeRow}>
          {TRIGGER_TYPES.map((type) => (
            <ToggleButton
              key={type.value}
              icon={() => <Text style={{ color: triggerType === type.value ? '#FFFFFF' : '#888', fontSize: 9 }}>{type.label}</Text>}
              value={type.value}
              status={triggerType === type.value ? 'checked' : 'unchecked'}
              onPress={() => onTriggerTypeChange(type.value)}
              style={styles.typeChip}
            />
          ))}
        </View>
      </View>

      {/* Trigger level slider */}
      <View style={styles.row}>
        <Text style={styles.label}>Level:</Text>
        <Slider
          value={triggerLevel}
          onValueChange={onTriggerLevelChange}
          minimumValue={0}
          maximumValue={255}
          step={1}
          style={styles.slider}
          thumbColor="#FF6600"
          theme={{ colors: { primary: '#FF6600' } }}
        />
        <Text style={styles.levelValue}>{voltage} V</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    paddingVertical: 4,
    paddingHorizontal: 8,
    backgroundColor: '#16213e',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  label: {
    color: '#AAAAAA',
    fontSize: 12,
    fontFamily: 'monospace',
    width: 50,
  },
  channelRow: {
    flexDirection: 'row',
    flex: 1,
    flexWrap: 'wrap',
  },
  typeRow: {
    flexDirection: 'row',
    flex: 1,
    flexWrap: 'wrap',
  },
  chip: {
    marginHorizontal: 2,
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#333',
  },
  typeChip: {
    marginHorizontal: 2,
    backgroundColor: '#1a1a2e',
    borderWidth: 1,
    borderColor: '#333',
  },
  slider: {
    flex: 1,
    height: 30,
  },
  levelValue: {
    color: '#FF6600',
    fontSize: 12,
    fontFamily: 'monospace',
    width: 60,
    textAlign: 'right',
  },
});