/**
 * TensilGuard Field — main App component
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Sets up React Navigation with six screens and wires the global store.
 * The app is a companion for the TensilGuard structural-health monitoring
 * node — a magnetoelastic cable-tension and acoustic-emission sensor for
 * bridge cables, guyed masts, and pre-stressed tendons.
 */
import React from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import DashboardScreen from './src/screens/DashboardScreen';
import NodeDetailScreen from './src/screens/NodeDetailScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import AlertsScreen from './src/screens/AlertsScreen';
import MeshHealthScreen from './src/screens/MeshHealthScreen';
import ExportScreen from './src/screens/ExportScreen';

export type RootStackParamList = {
  Dashboard: undefined;
  NodeDetail: { nodeId: string };
  Calibration: undefined;
  Alerts: undefined;
  MeshHealth: undefined;
  Export: undefined;
};

const Stack = createNativeStackNavigator<RootStackParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            headerStyle: { backgroundColor: '#1a2a3a' },
            headerTintColor: '#e8f0f8',
            headerTitleStyle: { fontWeight: 'bold' },
          }}
        >
          <Stack.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'TensilGuard Field' }} />
          <Stack.Screen name="NodeDetail" component={NodeDetailScreen} options={{ title: 'Node Detail' }} />
          <Stack.Screen name="Calibration" component={CalibrationScreen} options={{ title: 'Calibration' }} />
          <Stack.Screen name="Alerts" component={AlertsScreen} options={{ title: 'Alerts' }} />
          <Stack.Screen name="MeshHealth" component={MeshHealthScreen} options={{ title: 'Mesh Health' }} />
          <Stack.Screen name="Export" component={ExportScreen} options={{ title: 'Export Data' }} />
        </Stack.Navigator>
      </NavigationContainer>
      <StatusBar style="light" />
    </SafeAreaProvider>
  );
}