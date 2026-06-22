/**
 * App.js — LumiCast Companion App entry point
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * React Native app that connects to the LumiCast device over BLE,
 * displays real-time circadian photometry measurements, visualizes the
 * spectral power distribution, and exports logged data.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StatusBar } from 'react-native';

import { BleProvider } from './utils/BleContext';
import MonitorScreen from './screens/MonitorScreen';
import SpectrumScreen from './screens/SpectrumScreen';
import CircadianScreen from './screens/CircadianScreen';
import FlickerScreen from './screens/FlickerScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <BleProvider>
      <SafeAreaView style={{ flex: 1, backgroundColor: '#0a0a1a' }}>
        <StatusBar barStyle="light-content" backgroundColor="#0a0a1a" />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#333' },
              tabBarActiveTintColor: '#7C4DFF',
              tabBarInactiveTintColor: '#666',
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#fff',
            }}
          >
            <Tab.Screen
              name="Monitor"
              component={MonitorScreen}
              options={{ title: 'Live' }}
            />
            <Tab.Screen
              name="Spectrum"
              component={SpectrumScreen}
              options={{ title: 'SPD' }}
            />
            <Tab.Screen
              name="Circadian"
              component={CircadianScreen}
              options={{ title: 'Circadian' }}
            />
            <Tab.Screen
              name="Flicker"
              component={FlickerScreen}
              options={{ title: 'Flicker' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </BleProvider>
  );
}