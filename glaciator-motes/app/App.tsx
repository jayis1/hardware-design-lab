// App.tsx — Glaciator-Motes companion app navigation shell
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { Provider as PaperProvider, DefaultTheme } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

import DeployWizard from './src/DeployWizard';
import GatewayDashboard from './src/GatewayDashboard';
import EventDetail from './src/EventDetail';
import ArrayHealth from './src/ArrayHealth';
import ConfigSync from './src/ConfigSync';

export type RootStackParamList = {
  Deploy: undefined;
  Dashboard: undefined;
  EventDetail: { eventId: string };
  Health: undefined;
  Config: undefined;
};

const Stack = createNativeStackNavigator<RootStackParamList>();

const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#0d6efd',
    accent: '#17a2b8',
    background: '#0b1220',
    surface: '#141b2d',
    text: '#e6eaf2',
  },
};

export default function App() {
  return (
    <PaperProvider theme={theme}>
      <NavigationContainer>
        <StatusBar style="light" />
        <Stack.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            headerStyle: { backgroundColor: '#0b1220' },
            headerTintColor: '#e6eaf2',
            contentStyle: { backgroundColor: '#0b1220' },
          }}
        >
          <Stack.Screen
            name="Deploy"
            component={DeployWizard}
            options={{ title: 'Deploy Mote' }}
          />
          <Stack.Screen
            name="Dashboard"
            component={GatewayDashboard}
            options={{ title: 'Glaciator-Motes Gateway' }}
          />
          <Stack.Screen
            name="EventDetail"
            component={EventDetail}
            options={{ title: 'Seismic Event' }}
          />
          <Stack.Screen
            name="Health"
            component={ArrayHealth}
            options={{ title: 'Array Health' }}
          />
          <Stack.Screen
            name="Config"
            component={ConfigSync}
            options={{ title: 'Configuration Sync' }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}