/**
 * LithoCore — Companion App
 * Root navigator and app entry point.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import LiveSweepScreen from './src/screens/LiveSweepScreen';
import CellReportScreen from './src/screens/CellReportScreen';
import HistoryScreen from './src/screens/HistoryScreen';
import PackBuilderScreen from './src/screens/PackBuilderScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import { DatabaseProvider } from './src/db/database';

export type RootStackParamList = {
  LiveSweep: undefined;
  CellReport: { cellId?: string };
  History: undefined;
  PackBuilder: undefined;
  Settings: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <DatabaseProvider>
        <NavigationContainer>
          <Stack.Navigator
            initialRouteName="LiveSweep"
            screenOptions={{
              headerStyle: { backgroundColor: '#1a1a2e' },
              headerTintColor: '#e0e0e0',
              headerTitleStyle: { fontWeight: 'bold' },
              cardStyle: { backgroundColor: '#12122a' },
            }}
          >
            <Stack.Screen
              name="LiveSweep"
              component={LiveSweepScreen}
              options={{ title: 'LithoCore — Live Sweep' }}
            />
            <Stack.Screen
              name="CellReport"
              component={CellReportScreen}
              options={{ title: 'Cell Report' }}
            />
            <Stack.Screen
              name="History"
              component={HistoryScreen}
              options={{ title: 'Test History' }}
            />
            <Stack.Screen
              name="PackBuilder"
              component={PackBuilderScreen}
              options={{ title: 'Pack Builder' }}
            />
            <Stack.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Stack.Navigator>
        </NavigationContainer>
      </DatabaseProvider>
    </SafeAreaProvider>
  );
}