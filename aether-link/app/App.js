// App.js — Aether-Link Companion App Entry Point
// React Native app for monitoring and managing the Aether-Link
// NVMe-oF/TCP Hardware Accelerator.
//
// Navigation: Bottom tabs with 3 screens
//   - Dashboard: Real-time telemetry and status overview
//   - Connections: NVMe-oF connection management
//   - Settings: Device configuration and firmware updates

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  NavigationContainer,
  DefaultTheme,
} from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { StatusBar, AppState, Alert } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './screens/DashboardScreen';
import ConnectionsScreen from './screens/ConnectionsScreen';
import SettingsScreen from './screens/SettingsScreen';
import { DeviceProvider, useDevice } from './components/DeviceContext';
import { FrameAccumulator, parseFrame, parseEvent, CMD, FLAGS } from './utils/protocol';

const Tab = createBottomTabNavigator();

// Custom theme matching Aether-Link branding
const AetherTheme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#1A4B8C',       // Deep blue — Nous Research brand
    background: '#0A0E1A',    // Dark background
    card: '#111827',          // Card surface
    text: '#E5E7EB',          // Light text
    border: '#1F2937',        // Subtle borders
    notification: '#EF4444',  // Red for alerts
  },
};

// ============================================================================
// Main App Component
// ============================================================================

function AppContent() {
  const { connected, connect, disconnect, deviceHost, setDeviceHost } = useDevice();
  const [appState, setAppState] = useState(AppState.currentState);

  // Handle app state changes (background/foreground)
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appState.match(/active/) && nextAppState.match(/inactive|background/)) {
        // App going to background — keep connection alive for telemetry
      } else if (appState.match(/inactive|background/) && nextAppState === 'active') {
        // App returning to foreground — refresh data
      }
      setAppState(nextAppState);
    });

    return () => subscription.remove();
  }, [appState]);

  // Auto-connect on mount if host is configured
  useEffect(() => {
    if (deviceHost && !connected) {
      connect().catch(() => {
        // Connection will be retried by the user
      });
    }
  }, []);

  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName;
          switch (route.name) {
            case 'Dashboard':
              iconName = focused ? 'view-dashboard' : 'view-dashboard-outline';
              break;
            case 'Connections':
              iconName = focused ? 'lan-connect' : 'lan-disconnect';
              break;
            case 'Settings':
              iconName = focused ? 'cog' : 'cog-outline';
              break;
            default:
              iconName = 'help-circle';
          }
          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#3B82F6',
        tabBarInactiveTintColor: '#6B7280',
        tabBarStyle: {
          backgroundColor: '#111827',
          borderTopColor: '#1F2937',
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
          backgroundColor: '#111827',
          shadowColor: 'transparent',
          elevation: 0,
        },
        headerTintColor: '#E5E7EB',
        headerTitleStyle: {
          fontWeight: '700',
          fontSize: 18,
        },
      })}
    >
      <Tab.Screen
        name="Dashboard"
        component={DashboardScreen}
        options={{
          title: 'Aether-Link',
          headerRight: () => (
            <ConnectionIndicator connected={connected} />
          ),
        }}
      />
      <Tab.Screen
        name="Connections"
        component={ConnectionsScreen}
        options={{
          title: 'NVMe-oF Connections',
        }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{
          title: 'Settings',
        }}
      />
    </Tab.Navigator>
  );
}

// ============================================================================
// Connection Status Indicator (header right)
// ============================================================================

function ConnectionIndicator({ connected }) {
  return (
    <Icon
      name={connected ? 'lan-connect' : 'lan-disconnect'}
      size={24}
      color={connected ? '#10B981' : '#EF4444'}
      style={{ marginRight: 16 }}
    />
  );
}

// ============================================================================
// Root App with Providers
// ============================================================================

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer theme={AetherTheme}>
        <StatusBar barStyle="light-content" backgroundColor="#0A0E1A" />
        <AppContent />
      </NavigationContainer>
    </DeviceProvider>
  );
}
