/**
 * App.tsx — SpectraPest Field companion app entry point
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The SpectraPest Field app connects to SpectraPest gateway nodes via WiFi
 * (REST API) or directly to individual nodes via BLE for commissioning.
 * It provides a real-time pest pressure heatmap, detection history,
 * node management, and alert configuration.
 */

import React from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, DefaultTheme } from 'react-native-paper';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './src/screens/DashboardScreen';
import MapScreen from './src/screens/MapScreen';
import DetectionDetailScreen from './src/screens/DetectionDetailScreen';
import NodesScreen from './src/screens/NodesScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import { AppProvider } from './src/context/AppContext';

export type RootTabParamList = {
  Dashboard: undefined;
  Map: undefined;
  Detections: undefined;
  Nodes: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#2E7D32',
    accent: '#81C784',
    background: '#F1F8E9',
  },
};

export default function App() {
  return (
    <SafeAreaProvider>
      <PaperProvider theme={theme}>
        <AppProvider>
          <NavigationContainer theme={theme}>
            <Tab.Navigator
              screenOptions={({ route }) => ({
                tabBarIcon: ({ focused, color, size }) => {
                  let iconName: string;
                  switch (route.name) {
                    case 'Dashboard':
                      iconName = focused ? 'view-dashboard' : 'view-dashboard-outline';
                      break;
                    case 'Map':
                      iconName = focused ? 'map' : 'map-outline';
                      break;
                    case 'Detections':
                      iconName = focused ? 'bug' : 'bug-outline';
                      break;
                    case 'Nodes':
                      iconName = focused ? 'access-point' : 'access-point-check';
                      break;
                    case 'Settings':
                      iconName = focused ? 'cog' : 'cog-outline';
                      break;
                    default:
                      iconName = 'alert-circle';
                  }
                  return <Icon name={iconName} size={size} color={color} />;
                },
                tabBarActiveTintColor: '#2E7D32',
                tabBarInactiveTintColor: 'gray',
                headerStyle: { backgroundColor: '#2E7D32' },
                headerTintColor: '#fff',
              })}
            >
              <Tab.Screen
                name="Dashboard"
                component={DashboardScreen}
                options={{ title: 'Pest Dashboard' }}
              />
              <Tab.Screen
                name="Map"
                component={MapScreen}
                options={{ title: 'Pest Map' }}
              />
              <Tab.Screen
                name="Detections"
                component={DetectionDetailScreen}
                options={{ title: 'Detections' }}
              />
              <Tab.Screen
                name="Nodes"
                component={NodesScreen}
                options={{ title: 'Nodes' }}
              />
              <Tab.Screen
                name="Settings"
                component={SettingsScreen}
                options={{ title: 'Settings' }}
              />
            </Tab.Navigator>
          </NavigationContainer>
          <StatusBar style="light" />
        </AppProvider>
      </PaperProvider>
    </SafeAreaProvider>
  );
}