/**
 * App.js — SpeckleFlow companion app entry point
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The SpeckleFlow app connects to the handheld LSCI blood-flow imager
 * over BLE 5.2, displays a live perfusion map, records sessions, and
 * provides ROI analysis and export functionality.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { StatusBar } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialIcons';

import ConnectScreen from './screens/ConnectScreen';
import LiveFlowScreen from './screens/LiveFlowScreen';
import RecordScreen from './screens/RecordScreen';
import AnalysisScreen from './screens/AnalysisScreen';
import SettingsScreen from './screens/SettingsScreen';
import { DeviceProvider } from './utils/protocol';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <DeviceProvider>
      <NavigationContainer>
        <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />
        <Tab.Navigator
          screenOptions={{
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#e0e0e0',
            tabBarStyle: { backgroundColor: '#1a1a2e', paddingBottom: 4 },
            tabBarActiveTintColor: '#00d4ff',
            tabBarInactiveTintColor: '#666',
            tabBarLabelStyle: { fontSize: 11 },
          }}
        >
          <Tab.Screen
            name="Connect"
            component={ConnectScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="bluetooth" color={color} size={size} />
              ),
              title: 'Device',
            }}
          />
          <Tab.Screen
            name="Live"
            component={LiveFlowScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="visibility" color={color} size={size} />
              ),
              title: 'Live Flow',
            }}
          />
          <Tab.Screen
            name="Record"
            component={RecordScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="fiber-manual-record" color={color} size={size} />
              ),
              title: 'Record',
            }}
          />
          <Tab.Screen
            name="Analysis"
            component={AnalysisScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="analytics" color={color} size={size} />
              ),
              title: 'Analysis',
            }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{
              tabBarIcon: ({ color, size }) => (
                <Icon name="settings" color={color} size={size} />
              ),
              title: 'Settings',
            }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </DeviceProvider>
  );
}