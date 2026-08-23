/*
 * AcousticMap.js — StudGuard wall band visualization
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

const h = React.createElement;

export default function AcousticMap({ nodes }) {
  return h(
    View,
    { style: styles.card },
    h(Text, { style: styles.title }, 'Probable origin band'),
    ...nodes.map((node) => h(View, { key: node.id, style: styles.row },
      h(Text, { style: styles.label }, node.id),
      h(View, { style: styles.track },
        h(View, { style: [styles.fill, { width: `${Math.min(node.originBand / 1.5, 1) * 100}%` }] })
      ),
      h(Text, { style: styles.value }, `${node.originBand.toFixed(2)} m`)
    ))
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 14,
    marginBottom: 14
  },
  title: {
    color: '#f9fafb',
    fontSize: 16,
    fontWeight: '700',
    marginBottom: 10
  },
  row: {
    marginBottom: 10
  },
  label: {
    color: '#cbd5e1',
    marginBottom: 4
  },
  track: {
    height: 12,
    backgroundColor: '#1f2937',
    borderRadius: 6,
    overflow: 'hidden'
  },
  fill: {
    height: 12,
    backgroundColor: '#a855f7'
  },
  value: {
    color: '#94a3b8',
    marginTop: 4,
    fontSize: 12
  }
});
