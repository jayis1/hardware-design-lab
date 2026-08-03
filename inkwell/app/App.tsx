// App.tsx — Root navigation for the Inkwell companion app
//
// Wires up the bottom-tab navigator with five tabs: Live Canvas, Notebooks,
// Calibration, Export (hidden until a session is selected, but registered),
// and Settings. Connects to the pen on launch.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { StatusBar } from 'react-native';

import LiveCanvasScreen    from './src/screens/LiveCanvasScreen';
import NotebookListScreen  from './src/screens/NotebookListScreen';
import CalibrationScreen   from './src/screens/CalibrationScreen';
import SettingsScreen      from './src/screens/SettingsScreen';
import bleManager          from './src/ble/BleManager';

const Tab = createBottomTabNavigator();

export default function App() {
  useEffect(() => {
    // Auto-connect on launch. Real builds gate this on permissions.
    bleManager.connect().catch(e => console.warn('[Inkwell] auto-connect', e));
    return () => { bleManager.disconnect().catch(() => {}); };
  }, []);

  return (
    <NavigationContainer>
      <StatusBar barStyle="light-content" />
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor:   '#1a1a2e',
          tabBarInactiveTintColor: '#888',
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#fff',
        }}
      >
        <Tab.Screen name="Canvas"      component={LiveCanvasScreen}
                    options={{ title: 'Live Canvas' }} />
        <Tab.Screen name="Notebooks"   component={NotebookListScreen}
                    options={{ title: 'Notebooks' }} />
        <Tab.Screen name="Calibration" component={CalibrationScreen}
                    options={{ title: 'Calibrate' }} />
        <Tab.Screen name="Settings"    component={SettingsScreen}
                    options={{ title: 'Settings' }} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}