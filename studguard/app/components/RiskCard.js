/*
 * RiskCard.js — StudGuard app risk summary card
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { riskBand } from '../utils/protocol.js';

const h = React.createElement;

export default function RiskCard({ title, value, subtitle }) {
  const band = riskBand(value);
  return h(
    View,
    { style: [styles.card, { borderLeftColor: band.color }] },
    h(Text, { style: styles.title }, title),
    h(Text, { style: [styles.value, { color: band.color }] }, `${band.label} · ${(value * 100).toFixed(0)}%`),
    h(Text, { style: styles.subtitle }, subtitle)
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#111827',
    borderLeftWidth: 6,
    padding: 14,
    borderRadius: 12,
    marginBottom: 12
  },
  title: {
    color: '#e5e7eb',
    fontSize: 16,
    fontWeight: '700'
  },
  value: {
    marginTop: 6,
    fontSize: 20,
    fontWeight: '800'
  },
  subtitle: {
    color: '#9ca3af',
    marginTop: 6,
    fontSize: 13
  }
});
