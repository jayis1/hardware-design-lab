/*
 * App.js — Lichenwatch companion app (React Native).
 *
 * Author: jayis1
 * License: MIT
 *
 * The app provides a bottom-tab navigator with five screens:
 *   - Colony Dashboard (live vital signs + state)
 *   - Walk-by Sync (BLE download)
 *   - Alerts (push notifications history)
 *   - Site Map (deployed nodes)
 *   - Config (measurement / uplink settings)
 *
 * BLE communication is handled by utils/ble.js. The lichen index
 * interpretation lives in utils/braille.js-free lichen.js helper.
 */

import React, { useState, useEffect } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import ColonyDashboardScreen from './screens/ColonyDashboardScreen';
import WalkBySyncScreen from './screens/WalkBySyncScreen';
import AlertsScreen from './screens/AlertsScreen';
import SiteMapScreen from './screens/SiteMapScreen';
import ConfigScreen from './screens/ConfigScreen';

import { BleContext } from './utils/ble';

const Tab = createBottomTabNavigator();

export default function App() {
  const [bleState, setBleState] = useState({
    connectedNodeId: null,
    status: 'Disconnected',
    latest: null,
    history: [],
  });

  return (
    <BleContext.Provider value={{ bleState, setBleState }}>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ color, size }) => {
              let iconName;
              switch (route.name) {
                case 'Dashboard':
                  iconName = 'chart-line';
                  break;
                case 'Sync':
                  iconName = 'bluetooth';
                  break;
                case 'Alerts':
                  iconName = 'bell-alert';
                  break;
                case 'Map':
                  iconName = 'map-marker';
                  break;
                case 'Config':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'circle-outline';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#2e7d32',
            tabBarInactiveTintColor: 'gray',
            headerStyle: { backgroundColor: '#1b3a1b' },
            headerTintColor: '#fff',
          })}
        >
          <Tab.Screen
            name="Dashboard"
            component={ColonyDashboardScreen}
            options={{ title: 'Lichenwatch — Colony Dashboard' }}
          />
          <Tab.Screen
            name="Sync"
            component={WalkBySyncScreen}
            options={{ title: 'Walk-by Sync' }}
          />
          <Tab.Screen
            name="Alerts"
            component={AlertsScreen}
            options={{ title: 'Alerts' }}
          />
          <Tab.Screen
            name="Map"
            component={SiteMapScreen}
            options={{ title: 'Site Map' }}
          />
          <Tab.Screen
            name="Config"
            component={ConfigScreen}
            options={{ title: 'Configuration' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </BleContext.Provider>
  );
}