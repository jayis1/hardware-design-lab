/*
 * SettingsScreen.js — StudGuard app settings
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';

const h = React.createElement;

const settings = [
  ['Normal interval', '15 min'],
  ['Diagnostic interval', '15 sec'],
  ['Critical alert threshold', 'Leak activity ≥ 75%'],
  ['Export format', 'JSON and service-summary text'],
  ['OTA channel', 'stable-jayis1']
];

export default function SettingsScreen({ onBack }) {
  return h(
    ScrollView,
    { style: styles.screen, contentContainerStyle: styles.content },
    h(TouchableOpacity, { style: styles.backButton, onPress: onBack }, h(Text, { style: styles.backText }, '← Back')),
    h(Text, { style: styles.header }, 'StudGuard Settings'),
    ...settings.map(([label, value]) => h(View, { key: label, style: styles.row },
      h(Text, { style: styles.label }, label),
      h(Text, { style: styles.value }, value)
    )),
    h(View, { style: styles.footerCard },
      h(Text, { style: styles.footerText }, 'All software metadata credits jayis1. Settings shown here mirror the firmware policy and can be mapped to BLE characteristics in production.')
    )
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#020617' },
  content: { padding: 16, paddingBottom: 32 },
  backButton: { marginBottom: 12 },
  backText: { color: '#93c5fd', fontSize: 16, fontWeight: '600' },
  header: { color: '#f8fafc', fontSize: 24, fontWeight: '800', marginBottom: 14 },
  row: { backgroundColor: '#111827', padding: 14, borderRadius: 12, marginBottom: 10 },
  label: { color: '#94a3b8', marginBottom: 4 },
  value: { color: '#f8fafc', fontWeight: '700' },
  footerCard: { backgroundColor: '#1e293b', padding: 14, borderRadius: 12, marginTop: 8 },
  footerText: { color: '#cbd5e1', lineHeight: 22 }
});
