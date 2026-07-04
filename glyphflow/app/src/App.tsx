/**
 * App.tsx — GlyphFlow companion app navigation root
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Bottom-tab navigation across Live Capture, Trajectory, Training,
 * Dictionary, Device, and Settings screens.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { LiveCaptureScreen } from './screens/LiveCapture';
import { TrajectoryScreen }   from './screens/Trajectory';
import { TrainingScreen }      from './screens/Training';
import { DictionaryScreen }    from './screens/Dictionary';
import { DeviceScreen }        from './screens/Device';
import { SettingsScreen }      from './screens/Settings';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={{
            tabBarActiveTintColor: '#7cc4ff',
            tabBarInactiveTintColor: '#666',
            headerStyle: { backgroundColor: '#0a0a12' },
            headerTintColor: '#fff',
          }}
        >
          <Tab.Screen name="Capture"     component={LiveCaptureScreen} options={{ title: 'Live' }} />
          <Tab.Screen name="Trajectory"  component={TrajectoryScreen}   options={{ title: 'Trace' }} />
          <Tab.Screen name="Training"    component={TrainingScreen}     options={{ title: 'Train' }} />
          <Tab.Screen name="Dictionary"  component={DictionaryScreen}   options={{ title: 'Set' }} />
          <Tab.Screen name="Device"       component={DeviceScreen}        options={{ title: 'Band' }} />
          <Tab.Screen name="Settings"    component={SettingsScreen}      options={{ title: 'More' }} />
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}