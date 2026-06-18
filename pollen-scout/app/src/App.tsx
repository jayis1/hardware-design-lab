/**
 * App.tsx - Pollen Scout companion app entry point
 *
 * A React Native (Expo) app that pairs with nearby Pollen Scout
 * stations over BLE, provisions them, and visualizes real-time
 * pollen taxa + 72-hour forecasts. Remote stations are reached
 * via an MQTT broker.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { StationProvider } from './context/StationContext';
import DashboardScreen from './screens/DashboardScreen';
import TaxaDetailScreen from './screens/TaxaDetailScreen';
import DeviceHealthScreen from './screens/DeviceHealthScreen';
import AlertsScreen from './screens/AlertsScreen';
import OnboardingScreen from './screens/OnboardingScreen';

export type RootTabParamList = {
  Dashboard: undefined;
  TaxaDetail: undefined;
  DeviceHealth: undefined;
  Alerts: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <StationProvider>
        <NavigationContainer>
          <Tab.Navigator
            initialRouteName="Dashboard"
            screenOptions={{
              tabBarActiveTintColor: '#2E7D32',
              headerStyle: { backgroundColor: '#E8F5E9' },
              headerTitleStyle: { fontWeight: '700' },
            }}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ title: 'Pollen Scout' }}
            />
            <Tab.Screen
              name="TaxaDetail"
              component={TaxaDetailScreen}
              options={{ title: 'Taxa' }}
            />
            <Tab.Screen
              name="DeviceHealth"
              component={DeviceHealthScreen}
              options={{ title: 'Device' }}
            />
            <Tab.Screen
              name="Alerts"
              component={AlertsScreen}
              options={{ title: 'Alerts' }}
            />
            <Tab.Screen
              name="Onboarding"
              component={OnboardingScreen}
              options={{ title: 'Pair' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </StationProvider>
    </SafeAreaProvider>
  );
}