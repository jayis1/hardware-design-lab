// ============================================================================
// App.js — RideCore Companion App Main Entry
// ============================================================================
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import DashboardScreen from './screens/DashboardScreen';
import TuningScreen from './screens/TuningScreen';
import DiagnosticsScreen from './screens/DiagnosticsScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Tab.Navigator
        initialRouteName="Dashboard"
        screenOptions={{
          tabBarActiveTintColor: '#FF6B35',
          tabBarInactiveTintColor: '#888',
          tabBarStyle: { backgroundColor: '#1A1A2E' },
          headerStyle: { backgroundColor: '#1A1A2E' },
          headerTintColor: '#FFF',
        }}
      >
        <Tab.Screen
          name="Dashboard"
          component={DashboardScreen}
          options={{ tabBarLabel: 'Dashboard' }}
        />
        <Tab.Screen
          name="Tuning"
          component={TuningScreen}
          options={{ tabBarLabel: 'Tuning' }}
        />
        <Tab.Screen
          name="Diagnostics"
          component={DiagnosticsScreen}
          options={{ tabBarLabel: 'Diagnostics' }}
        />
        <Tab.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ tabBarLabel: 'Settings' }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}