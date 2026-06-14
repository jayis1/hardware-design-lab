/**
 * App.js — Chronos-RTK Companion App
 * React Native navigation root with bottom tabs
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screens
import MapScreen from './screens/MapScreen';
import SettingsScreen from './screens/SettingsScreen';
import LogScreen from './screens/LogScreen';

// Provider
import { ChronosProvider } from './components/ChronosContext';

const Tab = createBottomTabNavigator();
const Stack = createNativeStackNavigator();

function MainTabs() {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ color, size }) => {
          let iconName;
          if (route.name === 'Map') {
            iconName = 'map-marker-radius';
          } else if (route.name === 'Log') {
            iconName = 'format-list-bullet';
          } else if (route.name === 'Settings') {
            iconName = 'cog';
          }
          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#2196F3',
        tabBarInactiveTintColor: 'gray',
        headerStyle: { backgroundColor: '#1a1a2e' },
        headerTintColor: '#fff',
      })}
    >
      <Tab.Screen
        name="Map"
        component={MapScreen}
        options={{ title: 'Position', tabBarLabel: 'Position' }}
      />
      <Tab.Screen
        name="Log"
        component={LogScreen}
        options={{ title: 'Observation Log', tabBarLabel: 'Log' }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings', tabBarLabel: 'Settings' }}
      />
    </Tab.Navigator>
  );
}

export default function App() {
  return (
    <ChronosProvider>
      <NavigationContainer>
        <Stack.Navigator screenOptions={{ headerShown: false }}>
          <Stack.Screen name="Main" component={MainTabs} />
        </Stack.Navigator>
      </NavigationContainer>
    </ChronosProvider>
  );
}