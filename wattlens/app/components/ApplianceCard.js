/**
 * ApplianceCard.js — NILM appliance status card
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

export default function ApplianceCard({ name, isOn, probability, power }) {
  return (
    <View style={[styles.card, isOn ? styles.cardOn : styles.cardOff]}>
      <View style={styles.statusDot(isOn)} />
      <View style={styles.info}>
        <Text style={styles.name}>{name}</Text>
        <Text style={styles.detail}>
          {isOn ? 'RUNNING' : 'STANDBY'} · {Math.round(probability * 100)}% confidence
        </Text>
      </View>
      <View style={styles.power}>
        <Text style={styles.powerValue}>{isOn ? Math.round(power) : 0}</Text>
        <Text style={styles.powerUnit}>W</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    marginHorizontal: 12,
    marginVertical: 4,
    borderRadius: 8,
    borderWidth: 1,
  },
  cardOn: {
    backgroundColor: '#0a2a0a',
    borderColor: '#00AA00',
  },
  cardOff: {
    backgroundColor: '#1a1a1a',
    borderColor: '#333',
  },
  statusDot: (isOn) => ({
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 12,
    backgroundColor: isOn ? '#00FF00' : '#444',
  }),
  info: { flex: 1 },
  name: { fontSize: 15, fontWeight: '600', color: '#FFF' },
  detail: { fontSize: 11, color: '#888', marginTop: 2 },
  power: { alignItems: 'flex-end' },
  powerValue: { fontSize: 20, fontWeight: '700', color: '#00CC44' },
  powerUnit: { fontSize: 12, color: '#888' },
});