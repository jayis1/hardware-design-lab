/**
 * App.js — Vortex-SDR React Native entry point
 * Portable SDR Spectrum Analyzer companion app via BLE
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StyleSheet, StatusBar } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DeviceScreen from './screens/DeviceScreen';
import SpectrumScreen from './screens/SpectrumScreen';
import SettingsScreen from './screens/SettingsScreen';

import { BleProvider } from './components/BleContext';
import { theme } from './components/Theme';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <BleProvider>
      <SafeAreaView style={styles.container}>
        <StatusBar
          barStyle="light-content"
          backgroundColor={theme.colors.background}
        />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ focused, color, size }) => {
                let iconName;
                switch (route.name) {
                  case 'Device':
                    iconName = 'radio-handheld';
                    break;
                  case 'Spectrum':
                    iconName = 'waveform';
                    break;
                  case 'Settings':
                    iconName = 'cog';
                    break;
                  default:
                    iconName = 'help-circle';
                }
                return <Icon name={iconName} size={size} color={color} />;
              },
              tabBarActiveTintColor: theme.colors.primary,
              tabBarInactiveTintColor: theme.colors.textSecondary,
              tabBarStyle: {
                backgroundColor: theme.colors.surface,
                borderTopColor: theme.colors.border,
              },
              tabBarLabelStyle: {
                fontSize: 12,
                fontWeight: '600',
              },
              headerStyle: {
                backgroundColor: theme.colors.surface,
              },
              headerTintColor: theme.colors.text,
              headerTitleStyle: {
                fontWeight: '700',
              },
            })}
          >
            <Tab.Screen
              name="Device"
              component={DeviceScreen}
              options={{ title: 'Vortex-SDR' }}
            />
            <Tab.Screen
              name="Spectrum"
              component={SpectrumScreen}
              options={{ title: 'Spectrum' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </BleProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: theme.colors.background,
  },
});