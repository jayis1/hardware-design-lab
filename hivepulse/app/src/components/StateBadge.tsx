/**
 * StateBadge — Colony state indicator badge
 * Author: jayis1
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { ColonyState } from '../types';

const stateConfig = {
  [ColonyState.QueenrightHealthy]: {
    label: 'Queenright', icon: '✓', color: '#4CAF50', bg: 'rgba(76,175,80,0.15)',
  },
  [ColonyState.Queenless]: {
    label: 'Queenless', icon: '✗', color: '#DC3545', bg: 'rgba(220,53,69,0.15)',
  },
  [ColonyState.PreparingSwarm]: {
    label: 'Pre-Swarm', icon: '⚠', color: '#FFC107', bg: 'rgba(255,193,7,0.15)',
  },
  [ColonyState.VarroaHigh]: {
    label: 'Varroa High', icon: '⚠', color: '#FF6B35', bg: 'rgba(255,107,53,0.15)',
  },
  [ColonyState.WinterCluster]: {
    label: 'Winter', icon: '❄', color: '#03A9F4', bg: 'rgba(3,169,244,0.15)',
  },
  [ColonyState.DeadInactive]: {
    label: 'Dead', icon: '☠', color: '#666', bg: 'rgba(102,102,102,0.15)',
  },
};

export default function StateBadge({ state, confidence }: {
  state: ColonyState;
  confidence: number;
}) {
  const cfg = stateConfig[state] || stateConfig[ColonyState.DeadInactive];

  return (
    <View style={[styles.badge, { backgroundColor: cfg.bg, borderColor: cfg.color }]}>
      <Text style={[styles.icon, { color: cfg.color }]}>{cfg.icon}</Text>
      <Text style={[styles.label, { color: cfg.color }]}>{cfg.label}</Text>
      <Text style={styles.confidence}>{(confidence * 100).toFixed(0)}%</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 8,
    borderWidth: 1,
  },
  icon: { fontSize: 14, marginRight: 4 },
  label: { fontSize: 12, fontWeight: '600' },
  confidence: { fontSize: 10, color: '#888', marginLeft: 4 },
});