/**
 * App.js — BreathPrint companion app entry point
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Main app component with bottom tab navigation.
 * Screens: Home, Trends, Diet, Insights, Settings
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import { BleProvider } from './utils/BleContext';
import HomeScreen from './screens/HomeScreen';
import TrendScreen from './screens/TrendScreen';
import DietScreen from './screens/DietScreen';
import InsightsScreen from './screens/InsightsScreen';
import SettingsScreen from './screens/SettingsScreen';

const Tab = createBottomTabNavigator();

const App = () => {
  return (
    <BleProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ color, size }) => {
              let iconName;
              switch (route.name) {
                case 'Home':
                  iconName = 'home';
                  break;
                case 'Trends':
                  iconName = 'chart-line';
                  break;
                case 'Diet':
                  iconName = 'food-apple';
                  break;
                case 'Insights':
                  iconName = 'lightbulb-on';
                  break;
                case 'Settings':
                  iconName = 'cog';
                  break;
                default:
                  iconName = 'circle-outline';
              }
              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#2E86AB',
            tabBarInactiveTintColor: 'gray',
            headerStyle: { backgroundColor: '#1a1a2e' },
            headerTintColor: '#fff',
            headerTitleStyle: { fontWeight: 'bold' },
          })}
        >
          <Tab.Screen
            name="Home"
            component={HomeScreen}
            options={{ title: 'BreathPrint' }}
          />
          <Tab.Screen
            name="Trends"
            component={TrendScreen}
            options={{ title: 'Trends' }}
          />
          <Tab.Screen
            name="Diet"
            component={DietScreen}
            options={{ title: 'Diet Log' }}
          />
          <Tab.Screen
            name="Insights"
            component={InsightsScreen}
            options={{ title: 'Insights' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </BleProvider>
  );
};

export default App;