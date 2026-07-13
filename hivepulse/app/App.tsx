/**
 * HivePulse — Companion App for Acoustic Beehive Health Monitor
 *
 * React Native + TypeScript app for iOS and Android.
 * Connects to HivePulse devices via BLE (yard visits) and REST API (cloud).
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Provider } from 'react-redux';
import { store } from './src/store/hiveStore';

// Screens
import DashboardScreen from './src/screens/DashboardScreen';
import HiveDetailScreen from './src/screens/HiveDetailScreen';
import SwarmAlertScreen from './src/screens/SwarmAlertScreen';
import AudioInspectorScreen from './src/screens/AudioInspectorScreen';
import FleetMapScreen from './src/screens/FleetMapScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import HistoryScreen from './src/screens/HistoryScreen';

export type RootTabParamList = {
  Dashboard: undefined;
  HiveDetail: { hiveId: string };
  SwarmAlert: { hiveId: string };
  AudioInspector: { hiveId: string };
  FleetMap: undefined;
  Settings: undefined;
  History: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App() {
  return (
    <Provider store={store}>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarActiveTintColor: '#E8A317',
            tabBarInactiveTintColor: '#666',
            headerStyle: { backgroundColor: '#1A1A2E' },
            headerTintColor: '#E8A317',
            tabBarStyle: { backgroundColor: '#1A1A2E' },
          }}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'Hives' }}
          />
          <Tab.Screen
            name="FleetMap"
            component={FleetMapScreen}
            options={{ title: 'Map' }}
          />
          <Tab.Screen
            name="AudioInspector"
            component={AudioInspectorScreen}
            options={{ title: 'Audio' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
          <Tab.Screen
            name="History"
            component={HistoryScreen}
            options={{ title: 'History' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </Provider>
  );
}