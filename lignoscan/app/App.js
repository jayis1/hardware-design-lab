// ============================================================
// LignoScan App — Main Application Entry
// Companion app for the portable acoustic tomography scanner
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { SafeAreaView, StatusBar } from 'react-native';

import { BleContext, BleProvider } from './utils/BleContext';
import HomeScreen from './screens/HomeScreen';
import TomogramScreen from './screens/TomogramScreen';
import ScanListScreen from './screens/ScanListScreen';
import TreeInventoryScreen from './screens/TreeInventoryScreen';
import ReportScreen from './screens/ReportScreen';
import SettingsScreen from './screens/SettingsScreen';

const Stack = createStackNavigator();

const screenOptions = {
  headerStyle: {
    backgroundColor: '#1a3a1a',
  },
  headerTintColor: '#e0f0e0',
  headerTitleStyle: {
    fontWeight: 'bold',
  },
};

function AppNavigator() {
  return (
    <Stack.Navigator initialRouteName="Home" screenOptions={screenOptions}>
      <Stack.Screen
        name="Home"
        component={HomeScreen}
        options={{ title: 'LignoScan' }}
      />
      <Stack.Screen
        name="Tomogram"
        component={TomogramScreen}
        options={{ title: 'Tomogram' }}
      />
      <Stack.Screen
        name="ScanList"
        component={ScanListScreen}
        options={{ title: 'Scan History' }}
      />
      <Stack.Screen
        name="TreeInventory"
        component={TreeInventoryScreen}
        options={{ title: 'Tree Inventory' }}
      />
      <Stack.Screen
        name="Report"
        component={ReportScreen}
        options={{ title: 'Inspection Report' }}
      />
      <Stack.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings' }}
      />
    </Stack.Navigator>
  );
}

export default function App() {
  return (
    <BleProvider>
      <NavigationContainer>
        <StatusBar barStyle="light-content" backgroundColor="#1a3a1a" />
        <AppNavigator />
      </NavigationContainer>
    </BleProvider>
  );
}

// EOF — App.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT