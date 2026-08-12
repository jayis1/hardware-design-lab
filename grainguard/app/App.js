/**
 * @file    App.js
 * @brief   GrainGuard Companion App — Root component with tab navigation.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 *
 * The GrainGuard companion app connects to a site gateway (via Wi-Fi or
 * cellular) that aggregates data from GrainGuard in-silo probes over a
 * LoRa mesh. It provides four primary screens:
 *   1. Dashboard  — Overview map of all silos with SRI color codes
 *   2. Silo Detail — Per-silo sensor data (CO2, temp profile, moisture, insects)
 *   3. Alerts     — Active warnings and recommended actions
 *   4. Settings   — Probe configuration, grain type, thresholds, OTA
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DashboardScreen from './src/screens/DashboardScreen';
import SiloDetailScreen from './src/screens/SiloDetailScreen';
import AlertsScreen from './src/screens/AlertsScreen';
import SettingsScreen from './src/screens/SettingsScreen';

import { GrainGuardProvider } from './src/services/GrainGuardContext';

const Tab = createBottomTabNavigator();

export default function App() {
  return (
    <SafeAreaProvider>
      <GrainGuardProvider>
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={({ route }) => ({
              tabBarIcon: ({ color, size }) => {
                let iconName;
                switch (route.name) {
                  case 'Dashboard':  iconName = 'view-dashboard'; break;
                  case 'SiloDetail': iconName = 'silo'; break;
                  case 'Alerts':     iconName = 'alert-circle'; break;
                  case 'Settings':   iconName = 'cog'; break;
                  default:           iconName = 'help';
                }
                return <Icon name={iconName} size={size} color={color} />;
              },
              tabBarActiveTintColor: '#2E7D32',
              tabBarInactiveTintColor: 'gray',
              headerStyle: { backgroundColor: '#1B5E20' },
              headerTintColor: '#fff',
              headerTitleStyle: { fontWeight: 'bold' },
            })}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{ title: 'GrainGuard' }}
            />
            <Tab.Screen
              name="SiloDetail"
              component={SiloDetailScreen}
              options={{ title: 'Silo Detail' }}
            />
            <Tab.Screen
              name="Alerts"
              component={AlertsScreen}
              options={{ title: 'Alerts' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </GrainGuardProvider>
    </SafeAreaProvider>
  );
}