/**
 * App.js — SoundLoom companion app main entry
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom: Bioacoustic Soil Health Monitor
 * React Native application for iOS and Android.
 * Provides soundscape visualisation, SVI tracking, event alerts,
 * field-scale mapping, and device configuration.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import { Provider } from 'react-redux';
import { PersistGate } from 'redux-persist/integration/react';
import { store, persistor } from './src/store';
import BluetoothService from './src/services/BluetoothService';
import LoRaService from './src/services/LoRaService';

// Screens
import DashboardScreen from './src/screens/DashboardScreen';
import SoundscapeScreen from './src/screens/SoundscapeScreen';
import EventLogScreen from './src/screens/EventLogScreen';
import FieldMapScreen from './src/screens/FieldMapScreen';
import AlertsScreen from './src/screens/AlertsScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import PodDetailScreen from './src/screens/PodDetailScreen';
import EventDetailScreen from './src/screens/EventDetailScreen';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

/**
 * Main tab navigator: Dashboard, Soundscape, Events, Field Map, Alerts
 */
function MainTabs() {
  return (
    <Tab.Navigator
      screenOptions={{
        tabBarActiveTintColor: '#2E7D32',
        tabBarInactiveTintColor: '#757575',
        tabBarStyle: { backgroundColor: '#FAFAFA', borderTopWidth: 1, borderTopColor: '#E0E0E0' },
        headerStyle: { backgroundColor: '#2E7D32' },
        headerTintColor: '#FFFFFF',
        headerTitleStyle: { fontWeight: 'bold' },
      }}
    >
      <Tab.Screen
        name="Dashboard"
        component={DashboardScreen}
        options={{ tabBarLabel: 'Dashboard', tabBarIcon: ({ color, size }) => null }}
      />
      <Tab.Screen
        name="Soundscape"
        component={SoundscapeScreen}
        options={{ tabBarLabel: 'Soundscape', tabBarIcon: ({ color, size }) => null }}
      />
      <Tab.Screen
        name="Events"
        component={EventLogScreen}
        options={{ tabBarLabel: 'Events', tabBarIcon: ({ color, size }) => null }}
      />
      <Tab.Screen
        name="FieldMap"
        component={FieldMapScreen}
        options={{ tabBarLabel: 'Field Map', tabBarIcon: ({ color, size }) => null }}
      />
      <Tab.Screen
        name="Alerts"
        component={AlertsScreen}
        options={{ tabBarLabel: 'Alerts', tabBarIcon: ({ color, size }) => null }}
      />
    </Tab.Navigator>
  );
}

/**
 * Root stack navigator with modal screens for calibration and settings
 */
function RootStack() {
  return (
    <Stack.Navigator mode="modal">
      <Stack.Screen
        name="Main"
        component={MainTabs}
        options={{ headerShown: false }}
      />
      <Stack.Screen
        name="Calibration"
        component={CalibrationScreen}
        options={{ title: 'Calibration', headerStyle: { backgroundColor: '#2E7D32' }, headerTintColor: '#FFFFFF' }}
      />
      <Stack.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings', headerStyle: { backgroundColor: '#2E7D32' }, headerTintColor: '#FFFFFF' }}
      />
      <Stack.Screen
        name="PodDetail"
        component={PodDetailScreen}
        options={{ title: 'Pod Details', headerStyle: { backgroundColor: '#2E7D32' }, headerTintColor: '#FFFFFF' }}
      />
      <Stack.Screen
        name="EventDetail"
        component={EventDetailScreen}
        options={{ title: 'Event Detail', headerStyle: { backgroundColor: '#2E7D32' }, headerTintColor: '#FFFFFF' }}
      />
    </Stack.Navigator>
  );
}

/**
 * Main App component
 * Author: jayis1
 */
export default function App() {
  const [isReady, setIsReady] = useState(false);

  useEffect(() => {
    async function initialise() {
      try {
        // Initialise Bluetooth service for pod communication
        await BluetoothService.initialise();
        // Initialise LoRa gateway service if available
        await LoRaService.initialise();
        setIsReady(true);
      } catch (error) {
        console.error('SoundLoom: Failed to initialise services:', error);
        // Continue anyway — app is usable in offline/demo mode
        setIsReady(true);
      }
    }
    initialise();
  }, []);

  if (!isReady) {
    return null; // Splash screen would go here
  }

  return (
    <Provider store={store}>
      <PersistGate loading={null} persistor={persistor}>
        <NavigationContainer>
          <RootStack />
        </NavigationContainer>
      </PersistGate>
    </Provider>
  );
}