// App.tsx — MûonScape companion app root
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT
//
// React Native (Expo) app that connects to the MûonScape muon
// tomography imager over BLE (control + status) and Wi-Fi TCP (image
// and event download). Provides setup, calibration, acquisition,
// 3-D volume viewer, event log, and settings screens.

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { SafeAreaView, StyleSheet, View, Text } from 'react-native';

import SetupScreen from './src/screens/SetupScreen';
import CalibrationScreen from './src/screens/CalibrationScreen';
import AcquisitionScreen from './src/screens/AcquisitionScreen';
import ImageScreen from './src/screens/ImageScreen';
import EventLogScreen from './src/screens/EventLogScreen';
import SettingsScreen from './src/screens/SettingsScreen';
import DeviceService from './src/services/DeviceService';

export type RootStackParamList = {
  Setup: undefined;
  Calibration: undefined;
  Acquisition: undefined;
  Image: { acquisitionId: string };
  EventLog: undefined;
  Settings: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0a0e27' },
  header: { backgroundColor: '#1a1e3a', borderBottomWidth: 0 },
  headerTitle: { color: '#e0e6ff', fontSize: 18, fontWeight: '600' },
  headerTint: '#7c9eff',
});

export default function App() {
  const [deviceReady, setDeviceReady] = useState(false);
  const [connecting, setConnecting] = useState(true);

  useEffect(() => {
    // On launch, scan for a MûonScape device and connect over BLE
    DeviceService.scanAndConnect().then((connected) => {
      setDeviceReady(connected);
      setConnecting(false);
    }).catch(() => setConnecting(false));
  }, []);

  if (connecting) {
    return (
      <View style={[styles.container, { justifyContent: 'center', alignItems: 'center' }]}>
        <Text style={{ color: '#7c9eff', fontSize: 16 }}>
          Scanning for MûonScape...
        </Text>
        <Text style={{ color: '#5a6390', fontSize: 12, marginTop: 8 }}>
          by jayis1
        </Text>
      </View>
    );
  }

  return (
    <SafeAreaView style={styles.container}>
      <NavigationContainer>
        <Stack.Navigator
          initialRouteName={deviceReady ? 'Acquisition' : 'Setup'}
          screenOptions={{
            headerStyle: styles.header,
            headerTitleStyle: styles.headerTitle,
            headerTintColor: styles.headerTint,
            cardStyle: { backgroundColor: '#0a0e27' },
          }}
        >
          <Stack.Screen name="Setup" component={SetupScreen}
            options={{ title: 'Device Setup' }} />
          <Stack.Screen name="Calibration" component={CalibrationScreen}
            options={{ title: 'Calibration' }} />
          <Stack.Screen name="Acquisition" component={AcquisitionScreen}
            options={{ title: 'Acquisition' }} />
          <Stack.Screen name="Image" component={ImageScreen}
            options={{ title: 'Density Volume' }} />
          <Stack.Screen name="EventLog" component={EventLogScreen}
            options={{ title: 'Acquisitions' }} />
          <Stack.Screen name="Settings" component={SettingsScreen}
            options={{ title: 'Settings' }} />
        </Stack.Navigator>
      </NavigationContainer>
    </SafeAreaView>
  );
}