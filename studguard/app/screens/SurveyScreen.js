/*
 * SurveyScreen.js — StudGuard deployment workflow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, ScrollView, TouchableOpacity, StyleSheet } from 'react-native';

const h = React.createElement;

const steps = [
  'Photograph the wall and mark suspected plumbing path.',
  'Place one tile above fixture height, one below, and one lateral control tile.',
  'Press “Start baseline” after 10 minutes of undisturbed mounting.',
  'Run guided fixture event if leak is intermittent.',
  'Review probable opening band and export service report.'
];

export default function SurveyScreen({ onBack }) {
  return h(
    ScrollView,
    { style: styles.screen, contentContainerStyle: styles.content },
    h(TouchableOpacity, { style: styles.backButton, onPress: onBack }, h(Text, { style: styles.backText }, '← Back')),
    h(Text, { style: styles.header }, 'New StudGuard Survey'),
    h(Text, { style: styles.subheader }, 'Field workflow authored by jayis1 for fast hidden-leak deployment.'),
    ...steps.map((step, index) => h(View, { key: `step-${index}`, style: styles.stepCard },
      h(Text, { style: styles.stepIndex }, `Step ${index + 1}`),
      h(Text, { style: styles.stepText }, step)
    )),
    h(View, { style: styles.reportCard },
      h(Text, { style: styles.reportTitle }, 'Recommended kit'),
      h(Text, { style: styles.reportText }, '4-tile bathroom wall survey or 3-tile cabinet chase survey. Keep one dry reference tile outside the suspected wet band for confidence normalization.')
    )
  );
}

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#020617' },
  content: { padding: 16, paddingBottom: 32 },
  backButton: { marginBottom: 12 },
  backText: { color: '#93c5fd', fontSize: 16, fontWeight: '600' },
  header: { color: '#f8fafc', fontSize: 24, fontWeight: '800' },
  subheader: { color: '#94a3b8', marginTop: 8, marginBottom: 16 },
  stepCard: { backgroundColor: '#111827', borderRadius: 12, padding: 14, marginBottom: 10 },
  stepIndex: { color: '#38bdf8', fontWeight: '700', marginBottom: 4 },
  stepText: { color: '#e5e7eb', lineHeight: 22 },
  reportCard: { backgroundColor: '#172554', borderRadius: 12, padding: 14, marginTop: 8 },
  reportTitle: { color: '#dbeafe', fontWeight: '800', marginBottom: 6 },
  reportText: { color: '#bfdbfe', lineHeight: 22 }
});
