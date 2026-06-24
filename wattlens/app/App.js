/**
 * App.js — WattLens Companion App entry point
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * React Native app that connects to the WattLens device over BLE,
 * displays real-time power quality measurements, harmonic spectra,
 * NILM appliance disaggregation, event logs, and provides settings
 * for calibration and configuration.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { BleContext, BleProvider } from './utils/BleContext';

import DashboardScreen from './screens/DashboardScreen';
import WaveformScreen from './screens/WaveformScreen';
import HarmonicsScreen from './screens/HarmonicsScreen';
import LoadsScreen from './screens/LoadsScreen';
import EventsScreen from './screens/EventsScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

const TAB_OPTIONS = {
  Dashboard: { tabBarLabel: 'Dashboard', tabBarIcon: null },
  Waveform: { tabBarLabel: 'Waveform', tabBarIcon: null },
  Harmonics: { tabBarLabel: 'Harmonics', tabBarIcon: null },
  Loads: { tabBarLabel: 'Loads', tabBarIcon: null },
  Events: { tabBarLabel: 'Events', tabBarIcon: null },
  Settings: { tabBarLabel: 'Settings', tabBarIcon: null },
};

function App() {
  return (
    <BleProvider>
      <SafeAreaProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarActiveTintColor: '#0080FF',
              tabBarInactiveTintColor: '#666',
              tabBarStyle: { paddingBottom: 4, height: 56 },
              headerShown: true,
              headerTitle: 'WattLens',
              headerTitleAlign: 'center',
              headerStyle: { backgroundColor: '#001530' },
              headerTintColor: '#FFF',
            }}
          >
            <Tab.Screen name="Dashboard" component={DashboardScreen} options={TAB_OPTIONS.Dashboard} />
            <Tab.Screen name="Waveform" component={WaveformScreen} options={TAB_OPTIONS.Waveform} />
            <Tab.Screen name="Harmonics" component={HarmonicsScreen} options={TAB_OPTIONS.Harmonics} />
            <Tab.Screen name="Loads" component={LoadsScreen} options={TAB_OPTIONS.Loads} />
            <Tab.Screen name="Events" component={EventsScreen} options={TAB_OPTIONS.Events} />
            <Tab.Screen name="Settings" component={SettingsScreen} options={TAB_OPTIONS.Settings} />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaProvider>
    </BleProvider>
  );
}

export default App;