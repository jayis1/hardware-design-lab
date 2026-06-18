/**
 * App.js — FloraPulse Companion App Entry Point
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Main entry point for the React Native companion app.
 * Provides tab-based navigation between Monitor, Waveform,
 * Sap Flow, Events, and Settings screens.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import MonitorScreen from './screens/MonitorScreen';
import WaveformScreen from './screens/WaveformScreen';
import SapFlowScreen from './screens/SapFlowScreen';
import EventsScreen from './screens/EventsScreen';
import SettingsScreen from './screens/SettingsScreen';
import { BleProvider } from './utils/BleContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <BleProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarActiveTintColor: '#4CAF50',
              tabBarInactiveTintColor: '#666',
              headerStyle: { backgroundColor: '#1a2e1a' },
              headerTintColor: '#fff',
              tabBarStyle: { backgroundColor: '#1a2e1a', borderTopColor: '#333' },
            }}
          >
            <Tab.Screen
              name="Monitor"
              component={MonitorScreen}
              options={{ title: 'Monitor', tabBarIcon: 'leaf' }}
            />
            <Tab.Screen
              name="Waveform"
              component={WaveformScreen}
              options={{ title: 'Waveform', tabBarIcon: 'activity' }}
            />
            <Tab.Screen
              name="SapFlow"
              component={SapFlowScreen}
              options={{ title: 'Sap Flow', tabBarIcon: 'droplet' }}
            />
            <Tab.Screen
              name="Events"
              component={EventsScreen}
              options={{ title: 'Events', tabBarIcon: 'alert-circle' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings', tabBarIcon: 'settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BleProvider>
    </SafeAreaProvider>
  );
}