/**
 * App.js — HexaScope Companion App
 * Main navigation between Waveform View, Settings, and Protocol Decode screens
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/native-stack';
import { Provider as PaperProvider } from 'react-native-paper';

import WaveformScreen from './screens/WaveformScreen';
import SettingsScreen from './screens/SettingsScreen';
import ProtocolScreen from './screens/ProtocolScreen';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

function HomeTabs() {
  return (
    <Tab.Navigator
      screenOptions={{
        tabBarActiveTintColor: '#2196F3',
        tabBarInactiveTintColor: '#757575',
        tabBarStyle: { backgroundColor: '#1a1a2e' },
        headerStyle: { backgroundColor: '#16213e' },
        headerTintColor: '#ffffff',
      }}
    >
      <Tab.Screen
        name="Waveform"
        component={WaveformScreen}
        options={{
          tabBarLabel: 'Waveform',
          tabBarIcon: ({ color }) => null,
        }}
      />
      <Tab.Screen
        name="Protocol"
        component={ProtocolScreen}
        options={{
          tabBarLabel: 'Decode',
          tabBarIcon: ({ color }) => null,
        }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{
          tabBarLabel: 'Settings',
          tabBarIcon: ({ color }) => null,
        }}
      />
    </Tab.Navigator>
  );
}

export default function App() {
  return (
    <PaperProvider>
      <NavigationContainer>
        <Stack.Navigator>
          <Stack.Screen
            name="Home"
            component={HomeTabs}
            options={{ headerShown: false }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}