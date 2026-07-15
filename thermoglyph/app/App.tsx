/**
 * App.tsx — ThermoGlyph companion app entry point
 *
 * React Native + Expo app with bottom tab navigation.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StyleSheet, Text } from 'react-native';

import { DashboardScreen } from './src/screens/DashboardScreen';
import { NavigationScreen } from './src/screens/NavigationScreen';
import { ComposerScreen } from './src/screens/ComposerScreen';
import { BuddiesScreen } from './src/screens/BuddiesScreen';
import { TelemetryScreen } from './src/screens/TelemetryScreen';
import { SettingsScreen } from './src/screens/SettingsScreen';

export type TabParamList = {
  Dashboard: undefined;
  Navigate: undefined;
  Composer: undefined;
  Buddies: undefined;
  Telemetry: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<TabParamList>();

function TabIcon({ label, focused }: { label: string; focused: boolean }) {
  return (
    <Text style={{ color: focused ? '#fa4' : '#666', fontSize: 11 }}>
      {label}
    </Text>
  );
}

export default function App() {
  return (
    <SafeAreaView style={styles.container}>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarStyle: {
              backgroundColor: '#0d0d1a',
              borderTopColor: '#222',
            },
            tabBarActiveTintColor: '#fa4',
            tabBarInactiveTintColor: '#666',
            headerShown: false,
          }}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{
              tabBarLabel: 'Home',
              tabBarIcon: ({ focused }) => <TabIcon label="⌂" focused={focused} />,
            }}
          />
          <Tab.Screen
            name="Navigate"
            component={NavigationScreen}
            options={{
              tabBarLabel: 'Navigate',
              tabBarIcon: ({ focused }) => <TabIcon label="🧭" focused={focused} />,
            }}
          />
          <Tab.Screen
            name="Composer"
            component={ComposerScreen}
            options={{
              tabBarLabel: 'Composer',
              tabBarIcon: ({ focused }) => <TabIcon label="✎" focused={focused} />,
            }}
          />
          <Tab.Screen
            name="Buddies"
            component={BuddiesScreen}
            options={{
              tabBarLabel: 'Buddies',
              tabBarIcon: ({ focused }) => <TabIcon label="📡" focused={focused} />,
            }}
          />
          <Tab.Screen
            name="Telemetry"
            component={TelemetryScreen}
            options={{
              tabBarLabel: 'Telemetry',
              tabBarIcon: ({ focused }) => <TabIcon label="📊" focused={focused} />,
            }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{
              tabBarLabel: 'Settings',
              tabBarIcon: ({ focused }) => <TabIcon label="⚙" focused={focused} />,
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0d0d1a',
  },
});