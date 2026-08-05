// App.tsx — Root navigation for the FrostSentinel companion app
//
// Wires up the bottom-tab navigator with five tabs: Mesh Dashboard,
// Node Detail, Frost Watch, Calibration, and Settings. Connects to
// the active node on launch via BLE.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { StatusBar, View, Text, StyleSheet } from 'react-native';

import MeshDashboardScreen   from './src/screens/MeshDashboardScreen';
import NodeDetailScreen      from './src/screens/NodeDetailScreen';
import FrostWatchScreen      from './src/screens/FrostWatchScreen';
import CalibrationScreen     from './src/screens/CalibrationScreen';
import SettingsScreen        from './src/screens/SettingsScreen';
import ProvisioningScreen    from './src/screens/ProvisioningScreen';
import bleManager            from './src/ble/BleManager';

const Tab = createBottomTabNavigator();

export default function App() {
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    bleManager.connect().then(() => {
      setConnected(true);
    }).catch((e: any) => {
      console.warn('[FrostSentinel] auto-connect failed:', e);
    });
    return () => {
      bleManager.disconnect().catch(() => {});
    };
  }, []);

  return (
    <NavigationContainer>
      <StatusBar barStyle="light-content" />
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor:   '#2196F3',
          tabBarInactiveTintColor: '#888',
          headerStyle: { backgroundColor: '#0d1b2a' },
          headerTintColor: '#fff',
          headerTitleStyle: { fontWeight: 'bold' },
        }}
      >
        <Tab.Screen
          name="Mesh"
          component={MeshDashboardScreen}
          options={{ title: 'Mesh Dashboard' }}
        />
        <Tab.Screen
          name="Node"
          component={NodeDetailScreen}
          options={{ title: 'Node Detail' }}
        />
        <Tab.Screen
          name="Watch"
          component={FrostWatchScreen}
          options={{ title: 'Frost Watch' }}
        />
        <Tab.Screen
          name="Calibrate"
          component={CalibrationScreen}
          options={{ title: 'Calibration' }}
        />
        <Tab.Screen
          name="Provision"
          component={ProvisioningScreen}
          options={{ title: 'Provisioning' }}
        />
        <Tab.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ title: 'Settings' }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}