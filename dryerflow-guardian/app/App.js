/**
 * App.js
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
import React, { useMemo, useState } from 'react';
import { SafeAreaView, View, Text, Pressable, StyleSheet } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import LiveRunScreen from './screens/LiveRunScreen';
import AlertsScreen from './screens/AlertsScreen';
import SetupScreen from './screens/SetupScreen';
import ServiceScreen from './screens/ServiceScreen';

const tabs = [
  ['Dashboard', DashboardScreen],
  ['Live Run', LiveRunScreen],
  ['Alerts', AlertsScreen],
  ['Setup', SetupScreen],
  ['Service', ServiceScreen]
];

export default function App() {
  const [active, setActive] = useState('Dashboard');
  const ActiveScreen = useMemo(() => tabs.find(([name]) => name === active)?.[1] || DashboardScreen, [active]);

  return (
    <SafeAreaView style={styles.safeArea}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>DryerFlow Guardian</Text>
        <Text style={styles.headerSubtitle}>Companion application by jayis1</Text>
      </View>
      <View style={styles.content}>
        <ActiveScreen />
      </View>
      <View style={styles.tabBar}>
        {tabs.map(([name]) => (
          <Pressable key={name} style={[styles.tab, active === name && styles.tabActive]} onPress={() => setActive(name)}>
            <Text style={[styles.tabText, active === name && styles.tabTextActive]}>{name}</Text>
          </Pressable>
        ))}
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: { flex: 1, backgroundColor: '#020617' },
  header: { paddingHorizontal: 16, paddingTop: 12, paddingBottom: 8, backgroundColor: '#111827' },
  headerTitle: { color: '#f8fafc', fontSize: 22, fontWeight: '700' },
  headerSubtitle: { color: '#94a3b8', marginTop: 4 },
  content: { flex: 1 },
  tabBar: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    padding: 12,
    backgroundColor: '#0f172a'
  },
  tab: {
    width: '19%',
    backgroundColor: '#1f2937',
    borderRadius: 10,
    paddingVertical: 10,
    alignItems: 'center'
  },
  tabActive: { backgroundColor: '#2563eb' },
  tabText: { color: '#cbd5e1', fontSize: 11, fontWeight: '600' },
  tabTextActive: { color: '#ffffff' }
});
