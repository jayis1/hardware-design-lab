/**
 * Soilchord Field — main App component
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Sets up React Navigation with six screens and wires the global store.
 */
import React from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import DashboardScreen from './src/screens/DashboardScreen';
import ProbeDetailScreen from './src/screens/ProbeDetailScreen';
import RiskScreen from './src/screens/RiskScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import AlertsScreen from './src/screens/AlertsScreen';
import ExportScreen from './src/screens/ExportScreen';

export type RootStackParamList = {
  Dashboard: undefined;
  ProbeDetail: { probeId: string };
  Risk: undefined;
  Calibration: undefined;
  Alerts: undefined;
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
            headerStyle: { backgroundColor: '#1a3a2a' },
            headerTintColor: '#e8f5e9',
            headerTitleStyle: { fontWeight: 'bold' },
          }}
        >
          <Stack.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'Soilchord Field' }} />
          <Stack.Screen name="ProbeDetail" component={ProbeDetailScreen} options={{ title: 'Probe Detail' }} />
          <Stack.Screen name="Risk" component={RiskScreen} options={{ title: 'Site Risk' }} />
          <Stack.Screen name="Calibration" component={CalibrationScreen} options={{ title: 'Calibration' }} />
          <Stack.Screen name="Alerts" component={AlertsScreen} options={{ title: 'Alerts' }} />
          <Stack.Screen name="Export" component={ExportScreen} options={{ title: 'Export Data' }} />
        </Stack.Navigator>
      </NavigationContainer>
      <StatusBar style="light" />
    </SafeAreaProvider>
  );
}