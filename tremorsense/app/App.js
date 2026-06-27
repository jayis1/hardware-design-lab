/*
 * App.js — TremorSense Connect companion app (React Native entry)
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Main entry point for the React Native companion app.
 * Provides navigation between Dashboard, Tremor Timeline, Episode Detail,
 * Medication Log, Trends, Doctor Report, and Settings screens.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StatusBar, View, Text, StyleSheet } from 'react-native';

import DashboardScreen from './src/screens/DashboardScreen';
import TimelineScreen from './src/screens/TimelineScreen';
import MedicationScreen from './src/screens/MedicationScreen';
import TrendsScreen from './src/screens/TrendsScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import EpisodeDetailScreen from './src/screens/EpisodeDetailScreen';
import DoctorReportScreen from './src/screens/DoctorReportScreen';
import BleManager from './src/ble/BleManager';
import { TremorProvider } from './src/models/TremorContext';

const Tab = createBottomTabNavigator();

const THEME = {
  primary: '#0A84FF',
  background: '#1C1C1E',
  surface: '#2C2C2E',
  text: '#FFFFFF',
  textSecondary: '#8E8E93',
  parkinsonian: '#FF3B30',
  essential: '#FF9500',
  cerebellar: '#AF52DE',
  physiological: '#34C759',
  drug: '#FFCC00',
};

function App() {
  const [bleState, setBleState] = useState('disconnected');
  const [deviceData, setDeviceData] = useState(null);

  useEffect(() => {
    // Initialize BLE manager on app start
    BleManager.initialize();

    // Subscribe to connection state changes
    const unsubscribe = BleManager.onConnectionChange((state) => {
      setBleState(state);
    });

    // Subscribe to incoming tremor data
    const dataUnsub = BleManager.onData((data) => {
      setDeviceData(data);
    });

    return () => {
      unsubscribe();
      dataUnsub();
      BleManager.disconnect();
    };
  }, []);

  return (
    <TremorProvider value={{ bleState, deviceData, theme: THEME }}>
      <SafeAreaView style={styles.container}>
        <StatusBar barStyle="light-content" backgroundColor={THEME.background} />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: { backgroundColor: THEME.surface, borderTopWidth: 0 },
              tabBarActiveTintColor: THEME.primary,
              tabBarInactiveTintColor: THEME.textSecondary,
              headerShown: false,
            }}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ tabBarLabel: 'Home' }}
            />
            <Tab.Screen
              name="Timeline"
              component={TimelineScreen}
              options={{ tabBarLabel: 'Timeline' }}
            />
            <Tab.Screen
              name="Medication"
              component={MedicationScreen}
              options={{ tabBarLabel: 'Medication' }}
            />
            <Tab.Screen
              name="Trends"
              component={TrendsScreen}
              options={{ tabBarLabel: 'Trends' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ tabBarLabel: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </TremorProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.background,
  },
});

export default App;