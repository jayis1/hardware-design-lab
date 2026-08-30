// CrisperCue app shell
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
import React, { useMemo, useState } from 'react';
import { SafeAreaView, ScrollView, StatusBar, StyleSheet, Text, TouchableOpacity, View } from 'react-native';
import DashboardScreen from './screens/DashboardScreen';
import InventoryScreen from './screens/InventoryScreen';
import FreshnessScreen from './screens/FreshnessScreen';
import RecipesScreen from './screens/RecipesScreen';
import SettingsScreen from './screens/SettingsScreen';
import { buildCommandPacket, getBinHealth, mockBins } from './utils/protocol';

const tabs = ['Dashboard', 'Inventory', 'Freshness', 'Recipes', 'Settings'];

export default function App() {
  const [activeTab, setActiveTab] = useState('Dashboard');
  const [selectedBin, setSelectedBin] = useState(mockBins[0].id);
  const [fanBoost, setFanBoost] = useState(false);
  const [notifications, setNotifications] = useState(true);

  const selected = useMemo(
    () => mockBins.find((item) => item.id === selectedBin) || mockBins[0],
    [selectedBin]
  );

  const commandPacket = useMemo(
    () => buildCommandPacket(selected, { fanBoost, notifications }),
    [selected, fanBoost, notifications]
  );

  const health = useMemo(() => getBinHealth(selected), [selected]);

  const commonProps = {
    bins: mockBins,
    selected,
    selectedBin,
    setSelectedBin,
    fanBoost,
    setFanBoost,
    notifications,
    setNotifications,
    commandPacket,
    health,
  };

  const renderScreen = () => {
    switch (activeTab) {
      case 'Inventory':
        return <InventoryScreen {...commonProps} />;
      case 'Freshness':
        return <FreshnessScreen {...commonProps} />;
      case 'Recipes':
        return <RecipesScreen {...commonProps} />;
      case 'Settings':
        return <SettingsScreen {...commonProps} />;
      default:
        return <DashboardScreen {...commonProps} />;
    }
  };

  return (
    <SafeAreaView style={styles.safeArea}>
      <StatusBar barStyle="light-content" />
      <View style={styles.header}>
        <Text style={styles.title}>CrisperCue</Text>
        <Text style={styles.subtitle}>Produce intelligence system by jayis1</Text>
      </View>
      <ScrollView contentContainerStyle={styles.content}>{renderScreen()}</ScrollView>
      <View style={styles.tabBar}>
        {tabs.map((tab) => (
          <TouchableOpacity key={tab} style={[styles.tab, activeTab === tab && styles.activeTab]} onPress={() => setActiveTab(tab)}>
            <Text style={[styles.tabLabel, activeTab === tab && styles.activeTabLabel]}>{tab}</Text>
          </TouchableOpacity>
        ))}
      </View>
      <View style={styles.footer}>
        <Text style={styles.footerText}>Command packet preview</Text>
        <Text style={styles.packet}>{commandPacket}</Text>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: { flex: 1, backgroundColor: '#081018' },
  header: { paddingHorizontal: 20, paddingTop: 16, paddingBottom: 10 },
  title: { color: '#F6FBFF', fontSize: 30, fontWeight: '700' },
  subtitle: { color: '#9EC5D1', marginTop: 4 },
  content: { paddingHorizontal: 16, paddingBottom: 18 },
  tabBar: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-around',
    paddingVertical: 10,
    backgroundColor: '#0D1721',
    borderTopWidth: 1,
    borderTopColor: '#163040',
  },
  tab: {
    paddingHorizontal: 10,
    paddingVertical: 8,
    borderRadius: 999,
    marginVertical: 4,
  },
  activeTab: { backgroundColor: '#21485B' },
  tabLabel: { color: '#87A5B2', fontSize: 12 },
  activeTabLabel: { color: '#F6FBFF', fontWeight: '700' },
  footer: { padding: 16, backgroundColor: '#0B141D', borderTopWidth: 1, borderTopColor: '#163040' },
  footerText: { color: '#9EC5D1', marginBottom: 4 },
  packet: { color: '#E0F7FA', fontFamily: 'monospace', fontSize: 12 },
});
