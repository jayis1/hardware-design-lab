/*
 * DashboardScreen.js — StudGuard fleet overview
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import RiskCard from '../components/RiskCard.js';
import AcousticMap from '../components/AcousticMap.js';
import { summaryStats } from '../utils/protocol.js';

const h = React.createElement;

export default function DashboardScreen({ nodes, onSelectNode, onOpenSurvey, onOpenSettings }) {
  const stats = summaryStats(nodes);
  return h(
    ScrollView,
    { style: styles.screen, contentContainerStyle: styles.content },
    h(Text, { style: styles.header }, 'StudGuard Dashboard · jayis1'),
    h(Text, { style: styles.subheader }, `Critical zones: ${stats.critical} · Avg leak: ${stats.averageLeak} · Lowest battery: ${stats.lowestBattery}%`),
    h(RiskCard, { title: 'Portfolio leak risk', value: stats.averageLeak, subtitle: 'Combined mesh view across deployed tiles' }),
    h(RiskCard, { title: 'Escalation count', value: stats.critical / Math.max(nodes.length, 1), subtitle: 'Nodes requiring same-day inspection' }),
    h(AcousticMap, { nodes }),
    ...nodes.map((node) => h(
      TouchableOpacity,
      { key: node.id, style: styles.nodeCard, onPress: () => onSelectNode(node) },
      h(Text, { style: styles.nodeTitle }, `${node.id} · ${node.zone}`),
      h(Text, { style: styles.nodeMeta }, `Leak ${(node.leakActivity * 100).toFixed(0)}% · Spread ${(node.wetnessSpread * 100).toFixed(0)}% · ${node.event}`)
    )),
    h(View, { style: styles.buttonRow },
      h(TouchableOpacity, { style: styles.primaryButton, onPress: onOpenSurvey }, h(Text, { style: styles.primaryText }, 'New Survey')),
      h(TouchableOpacity, { style: styles.secondaryButton, onPress: onOpenSettings }, h(Text, { style: styles.secondaryText }, 'Settings'))
    )
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#020617' },
  content: { padding: 16, paddingBottom: 32 },
  header: { color: '#f8fafc', fontSize: 24, fontWeight: '800', marginBottom: 6 },
  subheader: { color: '#94a3b8', marginBottom: 16 },
  nodeCard: { backgroundColor: '#0f172a', padding: 14, borderRadius: 12, marginBottom: 10 },
  nodeTitle: { color: '#e2e8f0', fontSize: 15, fontWeight: '700' },
  nodeMeta: { color: '#94a3b8', marginTop: 6 },
  buttonRow: { flexDirection: 'row', gap: 12, marginTop: 10 },
  primaryButton: { flex: 1, backgroundColor: '#2563eb', padding: 14, borderRadius: 12, alignItems: 'center' },
  secondaryButton: { flex: 1, backgroundColor: '#1e293b', padding: 14, borderRadius: 12, alignItems: 'center' },
  primaryText: { color: 'white', fontWeight: '700' },
  secondaryText: { color: '#e2e8f0', fontWeight: '700' }
});
