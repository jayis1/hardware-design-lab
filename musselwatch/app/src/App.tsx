/*
 * App.tsx — MusselWatch companion app entry point
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

import React, { useState } from 'react';
import { NavigationContainer, DarkTheme, DefaultTheme } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { MaterialIcons } from '@expo/vector-icons';

import { NetworkDashboard } from './screens/NetworkDashboard';
import { AlertsScreen } from './screens/Alerts';
import { SettingsScreen } from './screens/Settings';
import { AppConfig } from './types';

const Tab = createBottomTabNavigator();

export default function App() {
  const [config, setConfig] = useState<AppConfig>({
    gatewayUrl: 'mock://musselwatch',
    pollIntervalS: 30,
    alertThreshold: 50,
    temperatureUnitC: true,
    darkMode: true,
  });

  const theme = config.darkMode ? DarkTheme : DefaultTheme;

  return (
    <NavigationContainer theme={theme}>
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ color, size }) => {
            let name: keyof typeof MaterialIcons.glyphMap = 'eco';
            if (route.name === 'Network') name = 'sensors';
            if (route.name === 'Alerts') name = 'notifications';
            if (route.name === 'Settings') name = 'settings';
            return <MaterialIcons name={name} size={size} color={color} />;
          },
          headerStyle: { backgroundColor: config.darkMode ? '#0a1f2c' : '#f5f5f5' },
          headerTintColor: config.darkMode ? '#e0f0f5' : '#0a4d6e',
        })}
      >
        <Tab.Screen name="Network" options={{ title: 'MusselWatch' }}>
          {() => <NetworkDashboard config={config} />}
        </Tab.Screen>
        <Tab.Screen name="Alerts" options={{ title: 'Alerts' }}>
          {() => <AlertsScreen config={config} />}
        </Tab.Screen>
        <Tab.Screen name="Settings" options={{ title: 'Settings' }}>
          {() => <SettingsScreen config={config} setConfig={setConfig} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}