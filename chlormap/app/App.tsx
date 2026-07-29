// App.tsx — ChloroMap companion app navigation shell
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { Provider as PaperProvider, DefaultTheme } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

import FieldMapScreen from './src/screens/FieldMapScreen';
import LiveSpectrumScreen from './src/screens/LiveSpectrumScreen';
import MeasurementScreen from './src/screens/MeasurementScreen';
import HistoryScreen from './src/screens/HistoryScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import { BleProvider } from './src/ble/BleManager';

export type RootStackParamList = {
  FieldMap: undefined;
  LiveSpectrum: undefined;
  Measurement: { measurementId: string };
  History: undefined;
  Calibration: undefined;
  Settings: undefined;
};

const Stack = createNativeStackNavigator<RootStackParamList>();

const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#2e7d32',
    accent: '#66bb6a',
    background: '#0a1f0a',
    surface: '#152815',
    text: '#e8f5e9',
  },
};

export default function App() {
  return (
    <BleProvider>
      <PaperProvider theme={theme}>
        <NavigationContainer>
          <StatusBar style="light" />
          <Stack.Navigator
            initialRouteName="FieldMap"
            screenOptions={{
              headerStyle: { backgroundColor: '#0a1f0a' },
              headerTintColor: '#e8f5e9',
              contentStyle: { backgroundColor: '#0a1f0a' },
            }}
          >
            <Stack.Screen
              name="FieldMap"
              component={FieldMapScreen}
              options={{ title: 'ChloroMap — Field Map' }}
            />
            <Stack.Screen
              name="LiveSpectrum"
              component={LiveSpectrumScreen}
              options={{ title: 'Live Spectrum' }}
            />
            <Stack.Screen
              name="Measurement"
              component={MeasurementScreen}
              options={{ title: 'Measurement Detail' }}
            />
            <Stack.Screen
              name="History"
              component={HistoryScreen}
              options={{ title: 'Measurement History' }}
            />
            <Stack.Screen
              name="Calibration"
              component={CalibrationScreen}
              options={{ title: 'Calibration' }}
            />
            <Stack.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Stack.Navigator>
        </NavigationContainer>
      </PaperProvider>
    </BleProvider>
  );
}