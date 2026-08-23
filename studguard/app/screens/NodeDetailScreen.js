/*
 * NodeDetailScreen.js — StudGuard node detail view
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';
import RiskCard from '../components/RiskCard.js';
import MoistureSparkline from '../components/MoistureSparkline.js';

const h = React.createElement;

export default function NodeDetailScreen({ node, onBack }) {
  return h(
    ScrollView,
    { style: styles.screen, contentContainerStyle: styles.content },
    h(TouchableOpacity, { style: styles.backButton, onPress: onBack }, h(Text, { style: styles.backText }, '← Back')),
    h(Text, { style: styles.header }, `${node.id} · ${node.zone}`),
    h(Text, { style: styles.subheader }, `Event: ${node.event} · Author: jayis1`),
    h(RiskCard, { title: 'Leak activity', value: node.leakActivity, subtitle: 'Fusion of acoustic attenuation and moisture delta' }),
    h(RiskCard, { title: 'Wetness spread', value: node.wetnessSpread, subtitle: 'Estimated vertical / lateral migration within wall band' }),
    h(RiskCard, { title: 'Confidence', value: node.confidence, subtitle: `Battery ${node.battery}% · Probable origin band ${node.originBand.toFixed(2)} m` }),
    h(Text, { style: styles.sectionTitle }, 'Leak timeline'),
    h(MoistureSparkline, { values: node.spark }),
    h(Text, { style: styles.sectionTitle }, 'Capacitive ring segments'),
    h(View, { style: styles.segmentRow },
      ...node.segments.map((segment, index) => h(View, { key: `${node.id}-seg-${index}`, style: styles.segmentCard },
        h(Text, { style: styles.segmentLabel }, `S${index + 1}`),
        h(Text, { style: styles.segmentValue }, `${(segment * 100).toFixed(0)}%`)
      ))
    )
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#020617' },
  content: { padding: 16, paddingBottom: 32 },
  backButton: { marginBottom: 12 },
  backText: { color: '#93c5fd', fontSize: 16, fontWeight: '600' },
  header: { color: '#f8fafc', fontSize: 24, fontWeight: '800' },
  subheader: { color: '#94a3b8', marginVertical: 8 },
  sectionTitle: { color: '#e5e7eb', marginTop: 16, marginBottom: 10, fontWeight: '700', fontSize: 16 },
  segmentRow: { flexDirection: 'row', justifyContent: 'space-between', flexWrap: 'wrap' },
  segmentCard: { backgroundColor: '#111827', width: '48%', padding: 14, borderRadius: 12, marginBottom: 10 },
  segmentLabel: { color: '#94a3b8' },
  segmentValue: { color: '#f8fafc', fontSize: 22, fontWeight: '800', marginTop: 6 }
});
