/**
 * SkyPilot — React Native Companion App
 * Main entry point with navigation
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createNativeStackNavigator } from '@react-navigation/native-stack';

import FlightScreen from './screens/FlightScreen';
import TelemetryScreen from './screens/TelemetryScreen';
import SettingsScreen from './screens/SettingsScreen';
import MapScreen from './screens/MapScreen';
import { ConnectionProvider } from './components/ConnectionContext';
import { Colors } from './utils/theme';

const Tab = createBottomTabNavigator();
const Stack = createNativeStackNavigator();

function TabNavigator() {
  return (
    <Tab.Navigator
      screenOptions={{
        tabBarActiveTintColor: Colors.primary,
        tabBarInactiveTintColor: Colors.inactive,
        tabBarStyle: { backgroundColor: Colors.background },
        headerStyle: { backgroundColor: Colors.primary },
        headerTintColor: '#fff',
      }}
    >
      <Tab.Screen
        name="Flight"
        component={FlightScreen}
        options={{
          tabBarLabel: 'Flight',
          tabBarIcon: ({ color }) => null, // Icon placeholder
          title: 'SkyPilot — Flight Control',
        }}
      />
      <Tab.Screen
        name="Telemetry"
        component={TelemetryScreen}
        options={{
          tabBarLabel: 'Telemetry',
          tabBarIcon: ({ color }) => null,
          title: 'SkyPilot — Telemetry',
        }}
      />
      <Tab.Screen
        name="Map"
        component={MapScreen}
        options={{
          tabBarLabel: 'Map',
          tabBarIcon: ({ color }) => null,
          title: 'SkyPilot — Map',
        }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{
          tabBarLabel: 'Settings',
          tabBarIcon: ({ color }) => null,
          title: 'SkyPilot — Settings',
        }}
      />
    </Tab.Navigator>
  );
}

function App() {
  return (
    <ConnectionProvider>
      <NavigationContainer>
        <Stack.Navigator>
          <Stack.Screen
            name="Main"
            component={TabNavigator}
            options={{ headerShown: false }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </ConnectionProvider>
  );
}

export default App;