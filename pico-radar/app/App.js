/**
 * App.js — PicoRadar Companion App
 *
 * React Native navigation with bottom tabs:
 *  - Radar: Real-time point cloud visualization
 *  - Settings: Device configuration, WiFi, chirp profiles
 *  - Data: IMU data, session logging, export
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createNativeStackNavigator } from '@react-navigation/native-stack';

import RadarScreen from './screens/RadarScreen';
import SettingsScreen from './screens/SettingsScreen';
import DataScreen from './screens/DataScreen';
import ConnectionScreen from './screens/ConnectionScreen';

import { ProtocolManager } from './utils/protocol';

const Tab = createBottomTabNavigator();
const Stack = createNativeStackNavigator();

// Global protocol manager instance (BLE/USB connection)
export const protocol = new ProtocolManager();

function RadarStack() {
  return (
    <Stack.Navigator>
      <Stack.Screen
        name="RadarMain"
        component={RadarScreen}
        options={{ title: 'PicoRadar' }}
      />
      <Stack.Screen
        name="Connection"
        component={ConnectionScreen}
        options={{ title: 'Connect Device' }}
      />
    </Stack.Navigator>
  );
}

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor: '#007AFF',
          tabBarStyle: { backgroundColor: '#1C1C1E' },
          tabBarLabelStyle: { fontSize: 12 },
          headerStyle: { backgroundColor: '#1C1C1E' },
          headerTintColor: '#FFFFFF',
        }}
      >
        <Tab.Screen
          name="Radar"
          component={RadarStack}
          options={{
            tabBarLabel: 'Radar',
            headerShown: false,
          }}
        />
        <Tab.Screen
          name="Data"
          component={DataScreen}
          options={{ tabBarLabel: 'Data' }}
        />
        <Tab.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ tabBarLabel: 'Settings' }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}