// App.js — StrataScan GPR Companion App Entry Point
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Main application component that sets up navigation and the BLE context
// provider.  The app uses a bottom tab navigator with five screens:
//   1. Survey — survey control + status display
//   2. Radargram — live B-scan rendering (the main GPR visualization)
//   3. A-Scan — single-trace waveform inspector
//   4. Calibration — guided calibration wizard
//   5. Settings — app + device settings
//

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { BleProvider } from './utils/BleContext';
import SurveyScreen from './screens/SurveyScreen';
import RadargramScreen from './screens/RadargramScreen';
import AscanScreen from './screens/AscanScreen';
import CalibrationScreen from './screens/CalibrationScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <BleProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarActiveTintColor: '#0066CC',
              tabBarInactiveTintColor: '#999',
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#fff',
            }}
          >
            <Tab.Screen
              name="Survey"
              component={SurveyScreen}
              options={{ tabBarLabel: 'Survey', title: 'StrataScan Survey' }}
            />
            <Tab.Screen
              name="Radargram"
              component={RadargramScreen}
              options={{ tabBarLabel: 'Radargram', title: 'Live B-Scan' }}
            />
            <Tab.Screen
              name="AScan"
              component={AscanScreen}
              options={{ tabBarLabel: 'A-Scan', title: 'A-Scan Inspector' }}
            />
            <Tab.Screen
              name="Calibration"
              component={CalibrationScreen}
              options={{ tabBarLabel: 'Calibration', title: 'Calibration' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ tabBarLabel: 'Settings', title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BleProvider>
    </SafeAreaProvider>
  );
}