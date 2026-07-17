/**
 * App.tsx — Stridophone companion app entry point.
 *
 * Author : jayis1
 * License: MIT
 *
 * Sets up the React Navigation stack with the six primary screens:
 *   Dashboard, LiveView, SpeciesDetail, DensityHeatmap, Alerts, Settings.
 */
import React, { useState, useEffect } from 'react';
import { StatusBar } from 'expo-status-bar';
import { StyleSheet } from 'react-native';
import { NavigationContainer, DarkTheme } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import AsyncStorage from '@react-native-async-storage/async-storage';

import DashboardScreen from './src/screens/DashboardScreen';
import LiveViewScreen from './src/screens/LiveViewScreen';
import SpeciesDetailScreen from './src/screens/SpeciesDetailScreen';
import DensityHeatmapScreen from './src/screens/DensityHeatmapScreen';
import AlertsScreen from './src/screens/AlertsScreen';
import SettingsScreen from './src/screens/SettingsScreen';

import { DeviceProvider } from './src/context/DeviceContext';

export type RootStackParamList = {
  Dashboard: undefined;
  LiveView: { deviceId: string };
  Species: { deviceId: string; speciesId: number };
  Heatmap: { deviceId: string };
  Alerts: undefined;
  Settings: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

export default function App() {
  const [ready, setReady] = useState(false);

  useEffect(() => {
    /* One-time setup: confirm async storage works. */
    AsyncStorage.getItem('@stridophone/init')
      .then(async (v) => {
        if (!v) await AsyncStorage.setItem('@stridophone/init', '1');
        setReady(true);
      })
      .catch(() => setReady(true));
  }, []);

  if (!ready) return null;

  return (
    <DeviceProvider>
      <NavigationContainer theme={DarkTheme}>
        <StatusBar style="light" />
        <Stack.Navigator
          screenOptions={{
            headerStyle: styles.header,
            headerTintColor: '#e0e0e0',
            headerTitleStyle: styles.title,
          }}
        >
          <Stack.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'Stridophone' }} />
          <Stack.Screen name="LiveView" component={LiveViewScreen} options={{ title: 'Live View' }} />
          <Stack.Screen name="Species" component={SpeciesDetailScreen} options={{ title: 'Species' }} />
          <Stack.Screen name="Heatmap" component={DensityHeatmapScreen} options={{ title: 'Density Heatmap' }} />
          <Stack.Screen name="Alerts" component={AlertsScreen} options={{ title: 'Alerts' }} />
          <Stack.Screen name="Settings" component={SettingsScreen} options={{ title: 'Settings' }} />
        </Stack.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}

const styles = StyleSheet.create({
  header: { backgroundColor: '#1a1a2e' },
  title: { fontWeight: 'bold', fontSize: 18 },
});