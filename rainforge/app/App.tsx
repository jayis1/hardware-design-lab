/**
 * App.tsx — RainForge companion app entry point
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Sets up the tab navigator and BLE provider wrapping all screens.
 */
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'expo-status-bar';

import { BLEProvider } from './src/components/BLEProvider';
import { DeviceProvider } from './src/components/DeviceContext';
import MapScreen from './src/screens/MapScreen';
import NodeDetailScreen from './src/screens/NodeDetailScreen';
import DropletExplorerScreen from './src/screens/DropletExplorerScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import SettingsScreen from './src/screens/SettingsScreen';

export type RootTabParamList = {
  Map: undefined;
  NodeDetail: undefined;
  Droplets: undefined;
  Calibrate: undefined;
  Settings: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App() {
  return (
    <SafeAreaProvider>
      <BLEProvider>
        <DeviceProvider>
          <NavigationContainer>
            <StatusBar style="light" />
            <Tab.Navigator
              screenOptions={{
                headerStyle: { backgroundColor: '#0a1628' },
                headerTintColor: '#4fc3f7',
                tabBarStyle: { backgroundColor: '#0a1628', borderTopColor: '#1a2a4a' },
                tabBarActiveTintColor: '#4fc3f7',
                tabBarInactiveTintColor: '#5a6a8a',
              }}
            >
              <Tab.Screen name="Map" component={MapScreen}
                options={{ title: 'Network Map' }} />
              <Tab.Screen name="NodeDetail" component={NodeDetailScreen}
                options={{ title: 'Node Detail' }} />
              <Tab.Screen name="Droplets" component={DropletExplorerScreen}
                options={{ title: 'Droplets' }} />
              <Tab.Screen name="Calibrate" component={CalibrationScreen}
                options={{ title: 'Calibrate' }} />
              <Tab.Screen name="Settings" component={SettingsScreen}
                options={{ title: 'Settings' }} />
            </Tab.Navigator>
          </NavigationContainer>
        </DeviceProvider>
      </BLEProvider>
    </SafeAreaProvider>
  );
}