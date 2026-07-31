/**
 * App.tsx — Root component for Synthand companion app.
 *
 * Sets up navigation between the four primary screens:
 * Calibration, Mapping, LiveMonitor, and Settings.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import CalibrationScreen from './src/screens/CalibrationScreen';
import MappingScreen from './src/screens/MappingScreen';
import LiveMonitorScreen from './src/screens/LiveMonitorScreen';
import SettingsScreen from './src/screens/SettingsScreen';

import { BleProvider } from './src/ble/BleManager';

export type RootTabParamList = {
  Calibrate: undefined;
  Map: undefined;
  Monitor: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

/**
 * SynthandApp — the root application component.
 * Author: jayis1
 */
export default function App() {
  return (
    <SafeAreaProvider>
      <BleProvider>
        <NavigationContainer>
          <Tab.Navigator
            initialRouteName="Monitor"
            screenOptions={{
              tabBarActiveTintColor: '#e94560',
              tabBarInactiveTintColor: '#888',
              tabBarStyle: {
                backgroundColor: '#16213e',
                borderTopColor: '#0f3460',
              },
              headerStyle: {
                backgroundColor: '#16213e',
              },
              headerTintColor: '#e94560',
              headerTitleStyle: {
                fontWeight: 'bold',
              },
            }}
          >
            <Tab.Screen
              name="Monitor"
              component={LiveMonitorScreen}
              options={{ title: 'Live Monitor' }}
            />
            <Tab.Screen
              name="Calibrate"
              component={CalibrationScreen}
              options={{ title: 'Calibration' }}
            />
            <Tab.Screen
              name="Map"
              component={MappingScreen}
              options={{ title: 'Mapping' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BleProvider>
    </SafeAreaProvider>
  );
}