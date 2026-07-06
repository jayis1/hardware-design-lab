/**
 * NavigationScreen.js — GPS navigation with haptic directional cues
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Sends directional haptic cues to the ring for turn-by-turn
 * navigation. Uses the phone's GPS and a mock navigation engine
 * (in production, would integrate with Google Maps / Mapbox SDK).
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  View, Text, TouchableOpacity, StyleSheet, ScrollView, Alert,
} from 'react-native';
import { BLEManager, Commands, NavDirection } from '../utils/ble';

const DIRECTIONS = [
  { label: 'Stop', value: NavDirection.STOP, icon: '✖', color: '#cc0000' },
  { label: 'Forward', value: NavDirection.FORWARD, icon: '↑', color: '#008800' },
  { label: 'Right', value: NavDirection.RIGHT, icon: '→', color: '#0066CC' },
  { label: 'Back', value: NavDirection.BACKWARD, icon: '↓', color: '#cc8800' },
  { label: 'Left', value: NavDirection.LEFT, icon: '←', color: '#0066CC' },
  { label: 'NE', value: NavDirection.NE, icon: '↗', color: '#008800' },
  { label: 'SE', value: NavDirection.SE, icon: '↘', color: '#cc8800' },
  { label: 'SW', value: NavDirection.SW, icon: '↙', color: '#cc8800' },
  { label: 'NW', value: NavDirection.NW, icon: '↖', color: '#008800' },
];

export default function NavigationScreen() {
  const [active, setActive] = useState(false);
  const [currentDirection, setCurrentDirection] = useState(null);
  const [navLog, setNavLog] = useState([]);

  const sendDirection = useCallback((direction) => {
    setCurrentDirection(direction);
    BLEManager.sendCommand(Commands.MODE_NAV);
    BLEManager.sendNavDirection(direction);
    const dirInfo = DIRECTIONS.find(d => d.value === direction);
    setNavLog(prev => [
      `${new Date().toLocaleTimeString()}: ${dirInfo?.label || 'Unknown'}`,
      ...prev.slice(0, 9),
    ]);
  }, []);

  const startNav = useCallback(() => {
    BLEManager.sendCommand(Commands.MODE_NAV);
    setActive(true);
    Alert.alert('Navigation Started', 'Haptic navigation cues will be sent to the ring.');
  }, []);

  const stopNav = useCallback(() => {
    sendDirection(NavDirection.STOP);
    setActive(false);
  }, [sendDirection]);

  // Demo mode: cycle through directions every 3 seconds
  useEffect(() => {
    if (!active) return;
    const demoSequence = [
      NavDirection.FORWARD, NavDirection.FORWARD,
      NavDirection.RIGHT, NavDirection.FORWARD,
      NavDirection.LEFT, NavDirection.FORWARD,
      NavDirection.STOP,
    ];
    let idx = 0;
    sendDirection(demoSequence[0]);
    const interval = setInterval(() => {
      idx = (idx + 1) % demoSequence.length;
      sendDirection(demoSequence[idx]);
    }, 3000);
    return () => clearInterval(interval);
  }, [active, sendDirection]);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Navigation Mode</Text>
      <Text style={styles.subtitle}>Send directional haptic cues to the ring</Text>

      {/* Current direction display */}
      <View style={styles.currentDirBox}>
        <Text style={styles.currentDirLabel}>Current Direction:</Text>
        <Text style={styles.currentDirIcon}>
          {currentDirection !== null
            ? DIRECTIONS.find(d => d.value === currentDirection)?.icon
            : '—'}
        </Text>
        <Text style={styles.currentDirName}>
          {currentDirection !== null
            ? DIRECTIONS.find(d => d.value === currentDirection)?.label
            : 'Not navigating'}
        </Text>
      </View>

      {/* Start/Stop buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.navBtn, active ? styles.navBtnStop : styles.navBtnStart]}
          onPress={active ? stopNav : startNav}
        >
          <Text style={styles.btnText}>
            {active ? 'Stop Demo' : 'Start Demo'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Manual direction buttons */}
      <Text style={styles.sectionLabel}>Manual Direction Test:</Text>
      <View style={styles.directionGrid}>
        {DIRECTIONS.map((dir) => (
          <TouchableOpacity
            key={dir.value}
            style={[styles.dirBtn, { backgroundColor: dir.color }]}
            onPress={() => sendDirection(dir.value)}
          >
            <Text style={styles.dirIcon}>{dir.icon}</Text>
            <Text style={styles.dirLabel}>{dir.label}</Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Navigation log */}
      <Text style={styles.sectionLabel}>Navigation Log:</Text>
      {navLog.length > 0 ? (
        navLog.map((entry, i) => (
          <Text key={i} style={styles.logEntry}>{entry}</Text>
        ))
      ) : (
        <Text style={styles.hint}>No navigation events yet</Text>
      )}

      <Text style={styles.author}>Author: jayis1</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f5f5f5' },
  title: { fontSize: 24, fontWeight: 'bold', color: '#0066CC' },
  subtitle: { fontSize: 14, color: '#666', marginBottom: 16 },
  currentDirBox: {
    backgroundColor: '#fff', borderRadius: 12, padding: 20,
    alignItems: 'center', marginBottom: 16, borderWidth: 1, borderColor: '#ddd',
  },
  currentDirLabel: { fontSize: 14, color: '#666' },
  currentDirIcon: { fontSize: 48, marginVertical: 8 },
  currentDirName: { fontSize: 18, fontWeight: 'bold', color: '#0066CC' },
  actionRow: { flexDirection: 'row', justifyContent: 'center', marginBottom: 16 },
  navBtn: { padding: 14, borderRadius: 10, minWidth: 150, alignItems: 'center' },
  navBtnStart: { backgroundColor: '#008800' },
  navBtnStop: { backgroundColor: '#cc0000' },
  btnText: { color: '#fff', fontWeight: 'bold', fontSize: 16 },
  sectionLabel: { fontSize: 16, fontWeight: 'bold', color: '#333', marginTop: 12, marginBottom: 8 },
  directionGrid: { flexDirection: 'row', flexWrap: 'wrap', justifyContent: 'space-between' },
  dirBtn: {
    width: '32%', padding: 12, borderRadius: 10, marginBottom: 8,
    alignItems: 'center',
  },
  dirIcon: { fontSize: 28, color: '#fff' },
  dirLabel: { fontSize: 12, color: '#fff', marginTop: 4 },
  logEntry: { fontSize: 12, color: '#666', paddingVertical: 2 },
  hint: { fontSize: 14, color: '#999', fontStyle: 'italic' },
  author: { fontSize: 10, color: '#999', marginTop: 20, textAlign: 'center' },
});