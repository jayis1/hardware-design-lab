/**
 * @file    App.js
 * @brief   TideBand Companion App — Root component with navigation.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 *
 * The TideBand companion app connects to the TideBand wrist-worn
 * hydrographic current profiler via BLE 5.0. It provides four
 * primary screens:
 *   1. Live Dive — real-time current velocity, depth, temperature
 *   2. Dive History — past dives with replay and export
 *   3. Mission Planning — thresholds, sample rate, pre-dive checks
 *   4. Settings — firmware, calibration, units, OTA updates
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import LiveDiveScreen from './src/screens/LiveDiveScreen';
import DiveHistoryScreen from './src/screens/DiveHistoryScreen';
import MissionPlanningScreen from './src/screens/MissionPlanningScreen';
import SettingsScreen from './src/screens/SettingsScreen';

import { TideBandProvider } from './src/services/TideBandContext';

const Tab = createBottomTabNavigator();

const TideBandTabIcon = ({ focused, color, size }) => null;

export default function App() {
  return (
    <SafeAreaProvider>
      <TideBandProvider>
        <NavigationContainer>
          <Tab.Navigator
            initialRouteName="LiveDive"
            screenOptions={{
              tabBarActiveTintColor: '#0080FF',
              tabBarInactiveTintColor: '#808080',
              tabBarStyle: { backgroundColor: '#FFFFFF' },
              headerStyle: { backgroundColor: '#0080FF' },
              headerTintColor: '#FFFFFF',
              headerTitleStyle: { fontWeight: 'bold' },
            }}
          >
            <Tab.Screen
              name="LiveDive"
              component={LiveDiveScreen}
              options={{
                title: 'Live Dive',
                tabBarLabel: 'Live',
              }}
            />
            <Tab.Screen
              name="DiveHistory"
              component={DiveHistoryScreen}
              options={{
                title: 'Dive History',
                tabBarLabel: 'History',
              }}
            />
            <Tab.Screen
              name="MissionPlanning"
              component={MissionPlanningScreen}
              options={{
                title: 'Mission Planning',
                tabBarLabel: 'Mission',
              }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{
                title: 'Settings',
                tabBarLabel: 'Settings',
              }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </TideBandProvider>
    </SafeAreaProvider>
  );
}