/**
 * App.js — FermenTiq Companion App Entry Point
 *
 * React Native navigation setup and global state provider for the
 * FermenTiq fermentation monitor companion app.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, DefaultTheme } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './screens/DashboardScreen';
import BatchSetupScreen from './screens/BatchSetupScreen';
import TrendScreen from './screens/TrendScreen';
import CalibrationScreen from './screens/CalibrationScreen';
import SettingsScreen from './screens/SettingsScreen';
import { BleManager, FermenTiqContext } from './utils/ble';

const Tab = createBottomTabNavigator();

// FermenTiq brand theme
const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#D4A017',      // amber/gold (fermentation warmth)
    accent: '#6B8E23',       // olive green
    background: '#1a1a2e',   // dark background
    surface: '#16213e',      // dark surface
    text: '#e0e0e0',
    error: '#E53935',
    warning: '#FF9800',
    success: '#4CAF50',
  },
};

export default function App() {
  const [bleManager] = useState(() => new BleManager());
  const [connectionState, setConnectionState] = useState({
    connected: false,
    deviceId: null,
    deviceName: null,
    liveData: null,
    config: null,
    alerts: [],
  });

  // Auto-connect on mount
  useEffect(() => {
    bleManager.startScan((device) => {
      console.log('Found FermenTiq device:', device.name);
    });

    return () => {
      bleManager.destroy();
    };
  }, [bleManager]);

  // Subscribe to live data updates
  useEffect(() => {
    const unsubscribe = bleManager.onLiveData((data) => {
      setConnectionState(prev => ({
        ...prev,
        liveData: data,
      }));
    });
    return unsubscribe;
  }, [bleManager]);

  // Subscribe to alerts
  useEffect(() => {
    const unsubscribe = bleManager.onAlert((alert) => {
      setConnectionState(prev => ({
        ...prev,
        alerts: [alert, ...prev.alerts].slice(0, 50),
      }));
    });
    return unsubscribe;
  }, [bleManager]);

  const updateConfig = useCallback(async (newConfig) => {
    await bleManager.writeConfig(newConfig);
    setConnectionState(prev => ({
      ...prev,
      config: { ...prev.config, ...newConfig },
    }));
  }, [bleManager]);

  const sendCommand = useCallback(async (command) => {
    await bleManager.sendCommand(command);
  }, [bleManager]);

  const connectToDevice = useCallback(async (deviceId) => {
    await bleManager.connect(deviceId);
    const config = await bleManager.readConfig();
    setConnectionState(prev => ({
      ...prev,
      connected: true,
      deviceId,
      config,
    }));
  }, [bleManager]);

  const contextValue = {
    bleManager,
    connectionState,
    updateConfig,
    sendCommand,
    connectToDevice,
  };

  return (
    <PaperProvider theme={theme}>
      <FermenTiqContext.Provider value={contextValue}>
        <NavigationContainer theme={theme}>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ focused, color, size }) => {
                let iconName;
                switch (route.name) {
                  case 'Dashboard':
                    iconName = focused ? 'view-dashboard' : 'view-dashboard-outline';
                    break;
                  case 'New Batch':
                    iconName = focused ? 'flask' : 'flask-outline';
                    break;
                  case 'Trends':
                    iconName = focused ? 'chart-line' : 'chart-line-variant';
                    break;
                  case 'Calibrate':
                    iconName = focused ? 'tune' : 'tune-variant';
                    break;
                  case 'Settings':
                    iconName = focused ? 'cog' : 'cog-outline';
                    break;
                  default:
                    iconName = 'help-circle-outline';
                }
                return <Icon name={iconName} size={size} color={color} />;
              },
              tabBarActiveTintColor: theme.colors.primary,
              tabBarInactiveTintColor: '#888',
              tabBarStyle: { backgroundColor: theme.colors.surface },
              headerShown: true,
              headerStyle: { backgroundColor: theme.colors.surface },
              headerTintColor: theme.colors.text,
            })}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ title: 'FermenTiq' }}
            />
            <Tab.Screen
              name="New Batch"
              component={BatchSetupScreen}
              options={{ title: 'New Batch' }}
            />
            <Tab.Screen
              name="Trends"
              component={TrendScreen}
              options={{ title: 'Trends' }}
            />
            <Tab.Screen
              name="Calibrate"
              component={CalibrationScreen}
              options={{ title: 'Calibrate' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </FermenTiqContext.Provider>
    </PaperProvider>
  );
}