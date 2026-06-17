/**
 * SonicSight Companion — React Native Application
 *
 * @format
 * @flow strict-local
 * @author    jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license   MIT
 */

import React, { useEffect, useState } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { BleManager } from 'react-native-ble-plx';

import HomeScreen from './src/screens/HomeScreen';
import LiveScanScreen from './src/screens/LiveScanScreen';
import HistogramScreen from './src/screens/HistogramScreen';
import DetailScreen from './src/screens/DetailScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import BleService from './src/services/BleService';

/* -------------------------------------------------------------------------- */
/*  Constants                                                                 */
/* -------------------------------------------------------------------------- */

const Tab = createBottomTabNavigator();
const bleManager = new BleManager();
const bleService = new BleService(bleManager);

/* -------------------------------------------------------------------------- */
/*  App Entry                                                                 */
/* -------------------------------------------------------------------------- */

const App = () => {
  const [isConnected, setIsConnected] = useState(false);
  const [deviceName, setDeviceName] = useState('Disconnected');

  useEffect(() => {
    /* Start BLE scanning on mount */
    bleService.startScan((device) => {
      if (device.name && device.name.includes('SonicSight')) {
        bleService.connect(device.id)
          .then(() => {
            setIsConnected(true);
            setDeviceName(device.name || 'SonicSight');
          })
          .catch((err) => console.warn('BLE connect failed:', err));
      }
    });

    return () => {
      bleService.stopScan();
      bleManager.destroy();
    };
  }, []);

  return (
    <NavigationContainer>
      <Tab.Navigator
        screenOptions={{
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#fff',
          tabBarStyle: { backgroundColor: '#1a1a2e', borderTopColor: '#16213e' },
          tabBarActiveTintColor: '#00d2ff',
          tabBarInactiveTintColor: '#888',
        }}>
        <Tab.Screen
          name="Home"
          options={{ title: 'SonicSight' }}>
          {() => <HomeScreen bleService={bleService} isConnected={isConnected}
                    deviceName={deviceName} />}
        </Tab.Screen>
        <Tab.Screen
          name="LiveScan"
          component={LiveScanScreen}
          options={{ title: 'Live Scan' }}
          initialParams={{ bleService }}
        />
        <Tab.Screen
          name="History"
          component={HistogramScreen}
          options={{ title: 'Scan History' }}
        />
        <Tab.Screen
          name="Detail"
          component={DetailScreen}
          options={{ title: 'Scan Detail' }}
        />
        <Tab.Screen
          name="Settings"
          options={{ title: 'Settings' }}>
          {() => <SettingsScreen bleService={bleService} />}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
};

export default App;