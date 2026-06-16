/**
 * Terramesh — Companion App
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Main application entry point for the Terramesh geotechnical monitoring
 * mesh companion app. Provides real-time monitoring, alerts, and
 * configuration for all deployed sensor nodes.
 */

import React, { useEffect, useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import { StatusBar, LogBox } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './screens/DashboardScreen';
import MapScreen from './screens/MapScreen';
import NodeDetailScreen from './screens/NodeDetailScreen';
import AlertsScreen from './screens/AlertsScreen';
import SettingsScreen from './screens/SettingsScreen';
import CommissionScreen from './screens/CommissionScreen';
import { DeviceProvider } from './components/DeviceContext';

LogBox.ignoreLogs(['Reanimated 2']);

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

const DashboardStack = () => (
  <Stack.Navigator
    screenOptions={{
      headerStyle: { backgroundColor: '#1a1a2e' },
      headerTintColor: '#e0e0e0',
      headerTitleStyle: { fontWeight: 'bold' },
    }}
  >
    <Stack.Screen
      name="Dashboard"
      component={DashboardScreen}
      options={{ title: 'Terramesh' }}
    />
    <Stack.Screen
      name="NodeDetail"
      component={NodeDetailScreen}
      options={{ title: 'Node Details' }}
    />
  </Stack.Navigator>
);

const App = () => {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ focused, color, size }) => {
              let iconName;
              switch (route.name) {
                case 'Dashboard':
                  iconName = 'view-dashboard';
                  break;
                case 'Map':
                  iconName = 'map';
                  break;
                case 'Alerts':
                  iconName = 'bell-ring';
                  break;
                case 'Commission':
                  iconName = 'cellphone-nfc';
                  break;
                case 'Settings':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'circle';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#00d4aa',
            tabBarInactiveTintColor: '#6c6c80',
            tabBarStyle: {
              backgroundColor: '#1a1a2e',
              borderTopColor: '#2a2a3e',
              paddingBottom: 5,
              height: 60,
            },
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#e0e0e0',
          })}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardStack}
            options={{ headerShown: false, title: 'Dashboard' }}
          />
          <Tab.Screen
            name="Map"
            component={MapScreen}
            options={{ title: 'Site Map' }}
          />
          <Tab.Screen
            name="Alerts"
            component={AlertsScreen}
            options={{ title: 'Alerts' }}
          />
          <Tab.Screen
            name="Commission"
            component={CommissionScreen}
            options={{ title: 'Commission' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
};

export default App;
