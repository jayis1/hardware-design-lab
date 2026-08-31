// Threshold Veil companion app
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.

import React, {useMemo, useState} from 'react';
import {SafeAreaView, ScrollView, StyleSheet, Text, TouchableOpacity, View} from 'react-native';

import MetricCard from './components/MetricCard';
import DashboardScreen from './screens/DashboardScreen';
import EventHistoryScreen from './screens/EventHistoryScreen';
import HealthScreen from './screens/HealthScreen';
import NoiseMapScreen from './screens/NoiseMapScreen';
import SealModesScreen from './screens/SealModesScreen';
import SetupScreen from './screens/SetupScreen';
import {buildMockSnapshot, summarizeTimeline} from './utils/protocol';

const TABS = ['Dashboard', 'Seal Modes', 'History', 'Noise Map', 'Health', 'Setup'];

export default function App() {
  const [tab, setTab] = useState('Dashboard');
  const [mode, setMode] = useState('AUTO');
  const snapshot = useMemo(() => buildMockSnapshot(mode), [mode]);
  const timelineSummary = useMemo(() => summarizeTimeline(snapshot.events), [snapshot.events]);

  const renderTab = () => {
    switch (tab) {
      case 'Seal Modes':
        return <SealModesScreen snapshot={snapshot} mode={mode} setMode={setMode} />;
      case 'History':
        return <EventHistoryScreen snapshot={snapshot} />;
      case 'Noise Map':
        return <NoiseMapScreen snapshot={snapshot} />;
      case 'Health':
        return <HealthScreen snapshot={snapshot} />;
      case 'Setup':
        return <SetupScreen snapshot={snapshot} />;
      case 'Dashboard':
      default:
        return <DashboardScreen snapshot={snapshot} />;
    }
  };

  return (
    <SafeAreaView style={styles.root}>
      <View style={styles.header}>
        <Text style={styles.title}>Threshold Veil</Text>
        <Text style={styles.subtitle}>Apartment threshold guardian by jayis1</Text>
      </View>
      <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.tabs}>
        {TABS.map(label => (
          <TouchableOpacity key={label} style={[styles.tab, tab === label && styles.activeTab]} onPress={() => setTab(label)}>
            <Text style={[styles.tabLabel, tab === label && styles.activeTabLabel]}>{label}</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>
      <View style={styles.summaryRow}>
        <MetricCard label="State" value={snapshot.state} accent="#8ce99a" />
        <MetricCard label="Ingress" value={snapshot.ingressScore.toFixed(2)} accent="#74c0fc" />
        <MetricCard label="Battery" value={`${snapshot.batteryPct.toFixed(0)}%`} accent="#ffd43b" />
      </View>
      <Text style={styles.timelineSummary}>{timelineSummary}</Text>
      <View style={styles.body}>{renderTab()}</View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  root: {
    flex: 1,
    backgroundColor: '#08111f',
  },
  header: {
    paddingHorizontal: 20,
    paddingTop: 16,
    paddingBottom: 8,
  },
  title: {
    color: '#f8f9fa',
    fontSize: 28,
    fontWeight: '700',
  },
  subtitle: {
    color: '#adb5bd',
    marginTop: 4,
  },
  tabs: {
    maxHeight: 54,
    paddingHorizontal: 12,
  },
  tab: {
    paddingHorizontal: 14,
    paddingVertical: 10,
    backgroundColor: '#132238',
    borderRadius: 999,
    marginHorizontal: 6,
    marginVertical: 8,
  },
  activeTab: {
    backgroundColor: '#1c7ed6',
  },
  tabLabel: {
    color: '#ced4da',
    fontWeight: '600',
  },
  activeTabLabel: {
    color: '#ffffff',
  },
  summaryRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    marginTop: 8,
  },
  timelineSummary: {
    color: '#dee2e6',
    marginHorizontal: 18,
    marginTop: 12,
    marginBottom: 8,
  },
  body: {
    flex: 1,
    paddingHorizontal: 16,
    paddingBottom: 16,
  },
});
