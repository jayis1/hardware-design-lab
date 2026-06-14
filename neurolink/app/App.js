/**
 * NeuroLink Companion App — Main Entry Point
 * React Native app for biosignal visualization, configuration, and recording
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import StreamScreen from './screens/StreamScreen';
import ChannelsScreen from './screens/ChannelsScreen';
import SettingsScreen from './screens/SettingsScreen';
import RecordingsScreen from './screens/RecordingsScreen';
import { BleProvider } from './components/BleContext';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

function MainTabs() {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ color, size }) => {
          let iconName;
          switch (route.name) {
            case 'Stream':
              iconName = 'waveform';
              break;
            case 'Channels':
              iconName = 'chart-bar';
              break;
            case 'Recordings':
              iconName = 'folder-multiple';
              break;
            case 'Settings':
              iconName = 'cog';
              break;
            default:
              iconName = 'help-circle';
          }
          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#4FC3F7',
        tabBarInactiveTintColor: '#78909C',
        tabBarStyle: { backgroundColor: '#1A1A2E' },
        headerStyle: { backgroundColor: '#16213E' },
        headerTintColor: '#E8E8E8',
      })}
    >
      <Tab.Screen
        name="Stream"
        component={StreamScreen}
        options={{ title: 'Live Stream' }}
      />
      <Tab.Screen
        name="Channels"
        component={ChannelsScreen}
        options={{ title: 'Channels' }}
      />
      <Tab.Screen
        name="Recordings"
        component={RecordingsScreen}
        options={{ title: 'Recordings' }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings' }}
      />
    </Tab.Navigator>
  );
}

export default function App() {
  return (
    <BleProvider>
      <NavigationContainer
        theme={{
          dark: true,
          colors: {
            primary: '#4FC3F7',
            background: '#0F0F1A',
            card: '#16213E',
            text: '#E8E8E8',
            border: '#2A2A4A',
            notification: '#FF5252',
          },
        }}
      >
        <Stack.Navigator>
          <Stack.Screen
            name="Main"
            component={MainTabs}
            options={{ headerShown: false }}
          />
        </Stack.Navigator>
      </NavigationContainer>
    </BleProvider>
  );
}