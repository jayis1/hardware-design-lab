/**
 * App.js — TactiScript Companion App Entry Point
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The TactiScript companion app provides the user interface for the
 * TactiScript haptic display ring. It connects via BLE 5.0 and offers
 * five main modes:
 *   1. Reader — stream text as Braille to the ring
 *   2. Navigation — send directional haptic cues from GPS
 *   3. Tutor — interactive Braille learning
 *   4. Music — translate rhythm to haptic patterns
 *   5. Texture Lab — design custom tactile textures
 *   Plus a Settings screen for device configuration.
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { Text, View, StyleSheet } from 'react-native';

import { BLEManager, ConnectionState } from './utils/ble';
import ConnectionStatus from './components/ConnectionStatus';
import ReaderScreen from './screens/ReaderScreen';
import NavigationScreen from './screens/NavigationScreen';
import TutorScreen from './screens/TutorScreen';
import MusicScreen from './screens/MusicScreen';
import TextureLabScreen from './screens/TextureLabScreen';
import SettingsScreen from './screens/SettingsScreen';

// Author metadata (jayis1)
const AUTHOR = 'jayis1';
const COPYRIGHT = 'Copyright (C) 2026 jayis1';

const Tab = createBottomTabNavigator();

export default function App() {
  const [connectionState, setConnectionState] = useState(ConnectionState.DISCONNECTED);
  const [batteryPct, setBatteryPct] = useState(0);
  const [deviceMode, setDeviceMode] = useState('READER');

  useEffect(() => {
    // Initialize BLE manager on app launch
    BLEManager.init();

    // Subscribe to connection state changes
    const unsubscribe = BLEManager.onConnectionChange((state) => {
      setConnectionState(state);
    });

    // Subscribe to status updates (battery, mode)
    const unsubStatus = BLEManager.onStatusUpdate((status) => {
      setBatteryPct(status.batteryPct);
      setDeviceMode(status.mode);
    });

    return () => {
      unsubscribe();
      unsubStatus();
      BLEManager.disconnect();
    };
  }, []);

  // Global connection header component
  const ConnectionHeader = useCallback(() => {
    return (
      <ConnectionStatus
        state={connectionState}
        batteryPct={batteryPct}
        mode={deviceMode}
      />
    );
  }, [connectionState, batteryPct, deviceMode]);

  return (
    <SafeAreaProvider>
      <NavigationContainer>
        <View style={styles.container}>
          <ConnectionHeader />
          <Tab.Navigator
            screenOptions={{
              tabBarActiveTintColor: '#0066CC',
              tabBarInactiveTintColor: '#999',
              tabBarStyle: { paddingBottom: 5, height: 60 },
              headerShown: false,
            }}
          >
            <Tab.Screen
              name="Reader"
              component={ReaderScreen}
              options={{ tabBarLabel: 'Reader' }}
            />
            <Tab.Screen
              name="Navigation"
              component={NavigationScreen}
              options={{ tabBarLabel: 'Nav' }}
            />
            <Tab.Screen
              name="Tutor"
              component={TutorScreen}
              options={{ tabBarLabel: 'Tutor' }}
            />
            <Tab.Screen
              name="Music"
              component={MusicScreen}
              options={{ tabBarLabel: 'Music' }}
            />
            <Tab.Screen
              name="Texture"
              component={TextureLabScreen}
              options={{ tabBarLabel: 'Texture' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ tabBarLabel: 'Settings' }}
            />
          </Tab.Navigator>
          <View style={styles.footer}>
            <Text style={styles.footerText}>TactiScript — by {AUTHOR}</Text>
            <Text style={styles.footerSubText}>{COPYRIGHT}</Text>
          </View>
        </View>
      </NavigationContainer>
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  footer: {
    padding: 8,
    backgroundColor: '#e0e0e0',
    alignItems: 'center',
  },
  footerText: {
    fontSize: 10,
    color: '#666',
    fontWeight: 'bold',
  },
  footerSubText: {
    fontSize: 8,
    color: '#999',
  },
});