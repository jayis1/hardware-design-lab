/**
 * App.js - CyberGuard Companion App
 * Multi-protocol hardware security token management
 *
 * Features:
 * - BLE connection and management
 * - FIDO2 credential management
 * - Fingerprint enrollment
 * - Device status monitoring
 * - Settings configuration
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import MainScreen from './screens/MainScreen';
import SettingsScreen from './screens/SettingsScreen';
import CredentialScreen from './screens/CredentialScreen';

const Stack = createStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator
        initialRouteName="Main"
        screenOptions={{
          headerStyle: {
            backgroundColor: '#1a1a2e',
          },
          headerTintColor: '#e94560',
          headerTitleStyle: {
            fontWeight: 'bold',
          },
        }}
      >
        <Stack.Screen
          name="Main"
          component={MainScreen}
          options={{ title: 'CyberGuard' }}
        />
        <Stack.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ title: 'Settings' }}
        />
        <Stack.Screen
          name="Credentials"
          component={CredentialScreen}
          options={{ title: 'Credentials' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}