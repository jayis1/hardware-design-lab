/**
 * App.js — Nebula Matrix Companion App Entry Point
 *
 * React Native application for configuring, monitoring, and controlling
 * the Nebula Matrix LED Display Engine.
 *
 * Navigation structure:
 *   - Bottom Tab Navigator with 4 tabs:
 *     1. Control — Main control panel (brightness, input source, test patterns)
 *     2. Monitor — Live status dashboard (frame rate, temperature, network stats)
 *     3. Settings — Matrix configuration, color calibration, network settings
 *     4. Firmware — OTA firmware update for FPGA and MCU
 *
 * Communication: UDP binary protocol to the Nebula Matrix device
 * over Ethernet (primary) or USB RNDIS (fallback).
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  StatusBar,
  Alert,
  AppState,
  Platform,
  LogBox,
} from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import ControlScreen from './screens/ControlScreen';
import MonitorScreen from './screens/MonitorScreen';
import SettingsScreen from './screens/SettingsScreen';
import FirmwareScreen from './screens/FirmwareScreen';
import DeviceConnection from './components/DeviceConnection';
import NebulaProtocol from './utils/protocol';

/* Suppress known harmless warnings */
LogBox.ignoreLogs([
  'Non-serializable values were found in the navigation state',
]);

const Tab = createBottomTabNavigator();

/* =========================================================================
 * App State — Global device connection and protocol instance
 * ========================================================================= */

const App = () => {
  const [deviceConnected, setDeviceConnected] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [connectionError, setConnectionError] = useState(null);
  const [isScanning, setIsScanning] = useState(false);

  const protocolRef = useRef(null);
  const appStateRef = useRef(AppState.currentState);

  /* Initialize protocol on mount */
  useEffect(() => {
    protocolRef.current = new NebulaProtocol();

    return () => {
      if (protocolRef.current) {
        protocolRef.current.disconnect();
      }
    };
  }, []);

  /* Handle app state changes (background/foreground) */
  useEffect(() => {
    const subscription = AppState.addEventListener('change', (nextAppState) => {
      if (appStateRef.current.match(/active/) && nextAppState.match(/inactive|background/)) {
        /* App going to background — keep connection alive for monitoring */
      } else if (appStateRef.current.match(/inactive|background/) && nextAppState === 'active') {
        /* App returning to foreground — refresh device status */
        if (deviceConnected && protocolRef.current) {
          refreshDeviceStatus();
        }
      }
      appStateRef.current = nextAppState;
    });

    return () => subscription.remove();
  }, [deviceConnected]);

  /* =====================================================================
   * Device Connection Management
   * ===================================================================== */

  const connectToDevice = useCallback(async (host, port = 6454) => {
    setIsScanning(true);
    setConnectionError(null);

    try {
      const proto = protocolRef.current;
      if (!proto) {
        throw new Error('Protocol not initialized');
      }

      await proto.connect(host, port);

      /* Ping device to verify communication */
      const info = await proto.pingDevice();
      if (!info || info.fpgaId !== 0x4E454255) {
        throw new Error('Device did not respond with valid ID');
      }

      setDeviceInfo(info);
      setDeviceConnected(true);
    } catch (error) {
      setConnectionError(error.message);
      setDeviceConnected(false);
      Alert.alert(
        'Connection Failed',
        `Could not connect to Nebula Matrix at ${host}:${port}\n\n${error.message}`
      );
    } finally {
      setIsScanning(false);
    }
  }, []);

  const disconnectDevice = useCallback(() => {
    if (protocolRef.current) {
      protocolRef.current.disconnect();
    }
    setDeviceConnected(false);
    setDeviceInfo(null);
  }, []);

  const refreshDeviceStatus = useCallback(async () => {
    if (!protocolRef.current || !deviceConnected) return;
    try {
      const status = await protocolRef.current.getFullStatus();
      setDeviceInfo(prev => ({ ...prev, ...status }));
    } catch (error) {
      console.warn('Status refresh failed:', error.message);
    }
  }, [deviceConnected]);

  /* =====================================================================
   * Navigation Screen Options
   * ===================================================================== */

  const screenOptions = ({ route }) => ({
    tabBarIcon: ({ focused, color, size }) => {
      let iconName;
      switch (route.name) {
        case 'Control':   iconName = focused ? 'gamepad-circle' : 'gamepad-circle-outline'; break;
        case 'Monitor':   iconName = focused ? 'monitor-dashboard' : 'monitor-eye'; break;
        case 'Settings':  iconName = focused ? 'cog' : 'cog-outline'; break;
        case 'Firmware':  iconName = focused ? 'chip' : 'chip-64'; break;
        default:          iconName = 'help-circle';
      }
      return <Icon name={iconName} size={size} color={color} />;
    },
    tabBarActiveTintColor: '#00E5FF',
    tabBarInactiveTintColor: '#666',
    tabBarStyle: {
      backgroundColor: '#1A1A2E',
      borderTopColor: '#333',
      borderTopWidth: 1,
      paddingBottom: Platform.OS === 'ios' ? 20 : 5,
      paddingTop: 5,
      height: Platform.OS === 'ios' ? 85 : 60,
    },
    tabBarLabelStyle: {
      fontSize: 11,
      fontWeight: '600',
    },
    headerStyle: {
      backgroundColor: '#16213E',
      shadowColor: 'transparent',
      elevation: 0,
    },
    headerTintColor: '#E0E0E0',
    headerTitleStyle: {
      fontWeight: '700',
      fontSize: 18,
    },
  });

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <NavigationContainer
      theme={{
        dark: true,
        colors: {
          primary: '#00E5FF',
          background: '#0F3460',
          card: '#16213E',
          text: '#E0E0E0',
          border: '#333',
          notification: '#FF6B6B',
        },
      }}
    >
      <StatusBar barStyle="light-content" backgroundColor="#16213E" />

      <Tab.Navigator screenOptions={screenOptions}>
        <Tab.Screen
          name="Control"
          options={{
            title: 'Control Panel',
            headerRight: () => (
              <DeviceConnection
                connected={deviceConnected}
                deviceInfo={deviceInfo}
                isScanning={isScanning}
                error={connectionError}
                onConnect={connectToDevice}
                onDisconnect={disconnectDevice}
              />
            ),
          }}
        >
          {(props) => (
            <ControlScreen
              {...props}
              protocol={protocolRef.current}
              connected={deviceConnected}
              deviceInfo={deviceInfo}
            />
          )}
        </Tab.Screen>

        <Tab.Screen
          name="Monitor"
          options={{
            title: 'Live Monitor',
            headerRight: () => (
              <DeviceConnection
                connected={deviceConnected}
                deviceInfo={deviceInfo}
                isScanning={isScanning}
                error={connectionError}
                onConnect={connectToDevice}
                onDisconnect={disconnectDevice}
              />
            ),
          }}
        >
          {(props) => (
            <MonitorScreen
              {...props}
              protocol={protocolRef.current}
              connected={deviceConnected}
              deviceInfo={deviceInfo}
              onRefresh={refreshDeviceStatus}
            />
          )}
        </Tab.Screen>

        <Tab.Screen
          name="Settings"
          options={{
            title: 'Settings',
            headerRight: () => (
              <DeviceConnection
                connected={deviceConnected}
                deviceInfo={deviceInfo}
                isScanning={isScanning}
                error={connectionError}
                onConnect={connectToDevice}
                onDisconnect={disconnectDevice}
              />
            ),
          }}
        >
          {(props) => (
            <SettingsScreen
              {...props}
              protocol={protocolRef.current}
              connected={deviceConnected}
              deviceInfo={deviceInfo}
            />
          )}
        </Tab.Screen>

        <Tab.Screen
          name="Firmware"
          options={{
            title: 'Firmware Update',
            headerRight: () => (
              <DeviceConnection
                connected={deviceConnected}
                deviceInfo={deviceInfo}
                isScanning={isScanning}
                error={connectionError}
                onConnect={connectToDevice}
                onDisconnect={disconnectDevice}
              />
            ),
          }}
        >
          {(props) => (
            <FirmwareScreen
              {...props}
              protocol={protocolRef.current}
              connected={deviceConnected}
              deviceInfo={deviceInfo}
            />
          )}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
};

export default App;
