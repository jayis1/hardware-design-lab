/**
 * App.js — PhotonWeave Companion App Entry Point
 *
 * React Native application for controlling the PhotonWeave CGH Engine.
 * Provides real-time monitoring, parameter configuration, and
 * holographic display control via USB or TCP connection.
 *
 * Navigation: Bottom tabs with 3 screens
 *   - Control: Main hologram control panel
 *   - Monitor: Real-time performance and thermal monitoring
 *   - Settings: Device configuration and calibration
 */

import React, { useState, useEffect, useRef } from 'react';
import { StatusBar, Alert, AppState } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import ControlScreen from './screens/ControlScreen';
import MonitorScreen from './screens/MonitorScreen';
import SettingsScreen from './screens/SettingsScreen';
import DeviceConnection from './components/DeviceConnection';
import { PhotonWeaveProtocol } from './utils/protocol';

const Tab = createBottomTabNavigator();

// Global protocol instance
const protocol = new PhotonWeaveProtocol();

// Theme colors
const THEME = {
  background: '#0A0E27',
  surface: '#141832',
  primary: '#00E5FF',
  secondary: '#7C4DFF',
  accent: '#FF6D00',
  success: '#00E676',
  warning: '#FFD600',
  error: '#FF1744',
  text: '#E0E0E0',
  textSecondary: '#9E9E9E',
  border: '#2A2F4A',
};

export { THEME, protocol };

export default function App() {
  const [isConnected, setIsConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const appState = useRef(AppState.currentState);

  // Handle app state changes (background/foreground)
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appState.current.match(/inactive|background/) &&
          nextAppState === 'active') {
        // App came to foreground — refresh connection
        if (protocol.isConnected()) {
          protocol.refreshStatus();
        }
      }
      appState.current = nextAppState;
    });

    return () => subscription.remove();
  }, []);

  // Connection handler
  const handleConnect = async (connectionParams) => {
    try {
      await protocol.connect(connectionParams);
      const info = await protocol.getDeviceInfo();
      setDeviceInfo(info);
      setIsConnected(true);
    } catch (error) {
      Alert.alert(
        'Connection Failed',
        `Could not connect to PhotonWeave device: ${error.message}`,
        [{ text: 'OK' }]
      );
    }
  };

  const handleDisconnect = async () => {
    try {
      await protocol.disconnect();
      setIsConnected(false);
      setDeviceInfo(null);
    } catch (error) {
      console.warn('Disconnect error:', error);
    }
  };

  return (
    <NavigationContainer
      theme={{
        dark: true,
        colors: {
          primary: THEME.primary,
          background: THEME.background,
          card: THEME.surface,
          text: THEME.text,
          border: THEME.border,
          notification: THEME.accent,
        },
      }}
    >
      <StatusBar barStyle="light-content" backgroundColor={THEME.background} />
      <Tab.Navigator
        screenOptions={({ route }) => ({
          tabBarIcon: ({ focused, color, size }) => {
            let iconName;
            switch (route.name) {
              case 'Control':
                iconName = focused ? 'hologram' : 'hologram';
                break;
              case 'Monitor':
                iconName = focused ? 'chart-line' : 'chart-line-variant';
                break;
              case 'Settings':
                iconName = focused ? 'cog' : 'cog-outline';
                break;
              default:
                iconName = 'circle';
            }
            return <Icon name={iconName} size={size} color={color} />;
          },
          tabBarActiveTintColor: THEME.primary,
          tabBarInactiveTintColor: THEME.textSecondary,
          tabBarStyle: {
            backgroundColor: THEME.surface,
            borderTopColor: THEME.border,
            borderTopWidth: 1,
            height: 60,
            paddingBottom: 8,
            paddingTop: 4,
          },
          tabBarLabelStyle: {
            fontSize: 12,
            fontWeight: '600',
          },
          headerStyle: {
            backgroundColor: THEME.surface,
            borderBottomColor: THEME.border,
            borderBottomWidth: 1,
          },
          headerTintColor: THEME.text,
          headerTitleStyle: {
            fontWeight: '700',
            fontSize: 18,
          },
        })}
      >
        <Tab.Screen
          name="Control"
          options={{
            title: 'Hologram Control',
            headerRight: () => (
              <DeviceConnection
                isConnected={isConnected}
                deviceInfo={deviceInfo}
                onConnect={handleConnect}
                onDisconnect={handleDisconnect}
              />
            ),
          }}
        >
          {(props) => (
            <ControlScreen
              {...props}
              protocol={protocol}
              isConnected={isConnected}
            />
          )}
        </Tab.Screen>
        <Tab.Screen
          name="Monitor"
          options={{
            title: 'Performance Monitor',
          }}
        >
          {(props) => (
            <MonitorScreen
              {...props}
              protocol={protocol}
              isConnected={isConnected}
            />
          )}
        </Tab.Screen>
        <Tab.Screen
          name="Settings"
          options={{
            title: 'Device Settings',
          }}
        >
          {(props) => (
            <SettingsScreen
              {...props}
              protocol={protocol}
              isConnected={isConnected}
              deviceInfo={deviceInfo}
            />
          )}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}
