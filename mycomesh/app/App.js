/**
 * App.js — MycoMesh Companion App entry point
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * React Native app that connects to MycoMesh sensor nodes via BLE
 * (direct) or LoRaWAN→MQTT (mesh), displays real-time fungal
 * electrophysiology data, visualizes spike activity, and configures
 * node settings.
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView, StatusBar } from 'react-native';

import { BleProvider } from './utils/BleContext';
import NodeMapScreen from './screens/NodeMapScreen';
import LiveTraceScreen from './screens/LiveTraceScreen';
import SpikeViewScreen from './screens/SpikeViewScreen';
import EnviroScreen from './screens/EnviroScreen';
import ConfigScreen from './screens/ConfigScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <BleProvider>
      <SafeAreaView style={{ flex: 1, backgroundColor: '#0a1a0a' }}>
        <StatusBar barStyle="light-content" backgroundColor="#0a1a0a" />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              tabBarStyle: { backgroundColor: '#1a2a1a', borderTopColor: '#2a3a2a' },
              tabBarActiveTintColor: '#4CAF50',
              tabBarInactiveTintColor: '#666',
              headerStyle: { backgroundColor: '#1a2a1a' },
              headerTintColor: '#fff',
            }}
          >
            <Tab.Screen
              name="NodeMap"
              component={NodeMapScreen}
              options={{ title: 'Nodes' }}
            />
            <Tab.Screen
              name="LiveTrace"
              component={LiveTraceScreen}
              options={{ title: 'Live' }}
            />
            <Tab.Screen
              name="SpikeView"
              component={SpikeViewScreen}
              options={{ title: 'Spikes' }}
            />
            <Tab.Screen
              name="Enviro"
              component={EnviroScreen}
              options={{ title: 'Env' }}
            />
            <Tab.Screen
              name="Config"
              component={ConfigScreen}
              options={{ title: 'Config' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </SafeAreaView>
    </BleProvider>
  );
}