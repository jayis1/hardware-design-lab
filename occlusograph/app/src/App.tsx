/*
 * App.tsx — Main entry point for the Occlusograph companion app.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * A React Native app that connects to the Occlusograph intraoral sensor via
 * BLE 5.3 and provides four screens: Live Bite Map, Bruxism Report,
 * Orthodontic Tracker, and Device Manager.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { View, Text, StyleSheet } from 'react-native';

import BiteMapScreen from './BiteMapScreen';
import BruxismReportScreen from './BruxismReportScreen';
import OrthodonticTrackerScreen from './OrthodonticTrackerScreen';
import DeviceManagerScreen from './DeviceManagerScreen';
import { ble, ConnectionState } from './ble';

export type RootTabParamList = {
  BiteMap: undefined;
  BruxismReport: undefined;
  OrthodonticTracker: undefined;
  DeviceManager: undefined;
};

const Tab = createBottomTabNavigator<RootTabParamList>();

export default function App(): React.ReactElement {
  const [connState, setConnState] = useState<ConnectionState>('disconnected');

  useEffect(() => {
    ble.setConnectionStateCallback((state) => setConnState(state));
  }, []);

  return (
    <NavigationContainer>
      <View style={styles.headerBar}>
        <Text style={styles.headerTitle}>Occlusograph</Text>
        <View style={[styles.statusDot,
          { backgroundColor: connState === 'connected' ? '#4CAF50' : '#F44336' }]} />
        <Text style={styles.statusText}>{connState}</Text>
      </View>
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor: '#1565C0',
          tabBarInactiveTintColor: '#999',
          tabBarStyle: { height: 56 },
        }}
      >
        <Tab.Screen
          name="BiteMap"
          component={BiteMapScreen}
          options={{ tabBarLabel: 'Bite Map' }}
        />
        <Tab.Screen
          name="BruxismReport"
          component={BruxismReportScreen}
          options={{ tabBarLabel: 'Bruxism' }}
        />
        <Tab.Screen
          name="OrthodonticTracker"
          component={OrthodonticTrackerScreen}
          options={{ tabBarLabel: 'Ortho Track' }}
        />
        <Tab.Screen
          name="DeviceManager"
          component={DeviceManagerScreen}
          options={{ tabBarLabel: 'Device' }}
        />
      </Tab.Navigator>
    </NavigationContainer>
  );
}

const styles = StyleSheet.create({
  headerBar: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingTop: 50,
    paddingHorizontal: 16,
    paddingBottom: 10,
    backgroundColor: '#0D47A1',
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
    flex: 1,
  },
  statusDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 6,
  },
  statusText: {
    fontSize: 12,
    color: '#fff',
    textTransform: 'capitalize',
  },
});