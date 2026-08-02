/*
 * App.tsx — HydraScan companion app entry + navigation
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Five-tab bottom navigation: Scan, Fingerprint, Library, History, Settings.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';

import ScanScreen        from './screens/ScanScreen';
import FingerprintScreen from './screens/FingerprintScreen';
import LibraryScreen     from './screens/LibraryScreen';
import HistoryScreen      from './screens/HistoryScreen';
import SettingsScreen     from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{ tabBarActiveTintColor: '#1e88e5', headerTitleAlign: 'center' }}
      >
        <Tab.Screen name="Scan"        component={ScanScreen}
          options={{ title: 'HydraScan — Scan' }} />
        <Tab.Screen name="Fingerprint" component={FingerprintScreen} />
        <Tab.Screen name="Library"     component={LibraryScreen} />
        <Tab.Screen name="History"     component={HistoryScreen} />
        <Tab.Screen name="Settings"    component={SettingsScreen} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}