/**
 * App.tsx — AeroCast companion app root
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Bottom-tab navigation across Scan, Dashboard, Forecast, History,
 * Calibration, and Settings screens. The app talks to AeroCast
 * hardware over BLE GATT (service 0xFE5A) for commissioning and live
 * status, and over MQTT (via a broker) for historical/forecast data.
 */
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { PaperProvider, DefaultTheme } from 'react-native-paper';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { DeviceProvider } from './src/components/DeviceContext';
import { BLEProvider } from './src/components/BLEProvider';
import ScanScreen       from './src/screens/ScanScreen';
import DashboardScreen  from './src/screens/DashboardScreen';
import ForecastScreen   from './src/screens/ForecastScreen';
import HistoryScreen    from './src/screens/HistoryScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import SettingsScreen   from './src/screens/SettingsScreen';

const Tab = createBottomTabNavigator();

const theme = {
  ...DefaultTheme,
  colors: { ...DefaultTheme.colors, primary: '#00C7A0', accent: '#5CB4FF' },
};

export default function App() {
  return (
    <PaperProvider theme={theme}>
      <SafeAreaProvider>
        <BLEProvider>
          <DeviceProvider>
            <NavigationContainer theme={theme}>
              <Tab.Navigator initialRouteName="Dashboard"
                screenOptions={{ headerStyle: { backgroundColor: '#0b1020' },
                                 headerTintColor: '#fff',
                                 tabBarActiveTintColor: '#00C7A0',
                                 tabBarStyle: { backgroundColor: '#0b1020' } }}>
                <Tab.Screen name="Scan"        component={ScanScreen}        options={{ title: 'Scan' }} />
                <Tab.Screen name="Dashboard"   component={DashboardScreen}   options={{ title: 'Live' }} />
                <Tab.Screen name="Forecast"    component={ForecastScreen}    options={{ title: 'Forecast' }} />
                <Tab.Screen name="History"     component={HistoryScreen}     options={{ title: 'History' }} />
                <Tab.Screen name="Calibration" component={CalibrationScreen} options={{ title: 'Calib' }} />
                <Tab.Screen name="Settings"    component={SettingsScreen}    options={{ title: 'Settings' }} />
              </Tab.Navigator>
            </NavigationContainer>
          </DeviceProvider>
        </BLEProvider>
      </SafeAreaProvider>
    </PaperProvider>
  );
}