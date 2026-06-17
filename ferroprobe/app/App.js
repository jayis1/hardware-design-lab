/**
 * App.js — FerroProbe Companion App Entry Point
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Main entry point for the React Native companion app.
 * Provides tab-based navigation between Monitor, Survey Map,
 * Calibration, and Settings screens.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import MonitorScreen from './screens/MonitorScreen';
import SurveyMapScreen from './screens/SurveyMapScreen';
import CalibrationScreen from './screens/CalibrationScreen';
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
              tabBarActiveTintColor: '#2196F3',
              tabBarInactiveTintColor: '#666',
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#fff',
              tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#333' },
            }}
          >
            <Tab.Screen
              name="Monitor"
              component={MonitorScreen}
              options={{ title: 'Monitor', tabBarIcon: 'activity' }}
            />
            <Tab.Screen
              name="Survey"
              component={SurveyMapScreen}
              options={{ title: 'Survey Map', tabBarIcon: 'map' }}
            />
            <Tab.Screen
              name="Calibration"
              component={CalibrationScreen}
              options={{ title: 'Calibration', tabBarIcon: 'crosshair' }}
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