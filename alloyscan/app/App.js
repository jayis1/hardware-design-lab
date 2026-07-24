/**
 * App.js — AlloyScan React Native companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * The AlloyScan app connects to the AlloyScan handheld device via BLE,
 * displays real-time scan results, browses the alloy reference database,
 * reviews scan history, and manages calibration.
 */

import React from 'react';
import { StatusBar } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import { BleProvider } from './src/ble/BleContext';
import LiveScanScreen from './src/screens/LiveScanScreen';
import DatabaseScreen from './src/screens/DatabaseScreen';
import HistoryScreen from './src/screens/HistoryScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import Colors from './src/constants/Colors';

const Tab = createBottomTabNavigator();

const App = () => {
  return (
    <SafeAreaProvider>
      <BleProvider>
        <StatusBar barStyle="light-content" backgroundColor={Colors.primary} />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ focused, color, size }) => {
                let iconName;
                switch (route.name) {
                  case 'Scan':
                    iconName = focused ? 'radar' : 'radar';
                    break;
                  case 'Database':
                    iconName = focused ? 'database' : 'database-outline';
                    break;
                  case 'History':
                    iconName = focused ? 'history' : 'history';
                    break;
                  case 'Calibrate':
                    iconName = focused ? 'wrench' : 'wrench-outline';
                    break;
                  case 'Settings':
                    iconName = focused ? 'cog' : 'cog-outline';
                    break;
                  default:
                    iconName = 'circle-outline';
                }
                return <Icon name={iconName} size={size} color={color} />;
              },
              tabBarActiveTintColor: Colors.accent,
              tabBarInactiveTintColor: Colors.gray,
              tabBarStyle: { backgroundColor: Colors.dark, borderTopColor: Colors.primary },
              headerStyle: { backgroundColor: Colors.primary },
              headerTintColor: Colors.white,
              headerTitleStyle: { fontWeight: 'bold' },
            })}
          >
            <Tab.Screen
              name="Scan"
              component={LiveScanScreen}
              options={{ title: 'AlloyScan' }}
            />
            <Tab.Screen
              name="Database"
              component={DatabaseScreen}
              options={{ title: 'Alloy Database' }}
            />
            <Tab.Screen
              name="History"
              component={HistoryScreen}
              options={{ title: 'Scan History' }}
            />
            <Tab.Screen
              name="Calibrate"
              component={CalibrationScreen}
              options={{ title: 'Calibration' }}
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
};

export default App;