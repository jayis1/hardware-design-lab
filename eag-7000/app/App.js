/**
 * EAG-7000 Edge AI Gateway — Companion App
 *
 * Main application entry point. Sets up navigation between:
 * - Dashboard: System overview, CPU/NPU/DRAM status, temperature
 * - CAN Bus: Real-time CAN-FD message monitor and sender
 * - Sensors: I2C sensor data viewer (via TCA9548A mux channels)
 * - Settings: BLE connection, device configuration, firmware update
 *
 * Communication with the EAG-7000 gateway uses a custom binary protocol
 * over BLE, defined in utils/protocol.js. The wire protocol matches
 * the MU mailbox message format used by the M4F firmware:
 *   [TYPE(8)][LEN(8)][DATA(16)]
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

// Screens
import DashboardScreen from './screens/DashboardScreen';
import CANBusScreen from './screens/CANBusScreen';
import SensorsScreen from './screens/SensorsScreen';
import SettingsScreen from './screens/SettingsScreen';

// BLE connection provider
import { BLEProvider } from './components/BLEProvider';

const Tab = createBottomTabNavigator();

function App() {
  return (
    <BLEProvider>
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarIcon: ({ focused, color, size }) => {
              let iconName;

              switch (route.name) {
                case 'Dashboard':
                  iconName = focused ? 'view-dashboard' : 'view-dashboard-outline';
                  break;
                case 'CAN Bus':
                  iconName = focused ? 'bus' : 'bus-outline';
                  break;
                case 'Sensors':
                  iconName = focused ? 'thermometer' : 'thermometer-outline';
                  break;
                case 'Settings':
                  iconName = focused ? 'cog' : 'cog-outline';
                  break;
                default:
                  iconName = 'help-circle';
              }

              return <Icon name={iconName} size={size} color={color} />;
            },
            tabBarActiveTintColor: '#2196F3',
            tabBarInactiveTintColor: '#757575',
            tabBarStyle: {
              backgroundColor: '#1A1A2E',
              borderTopColor: '#16213E',
            },
            headerStyle: {
              backgroundColor: '#1A1A2E',
            },
            headerTintColor: '#E0E0E0',
          })}
        >
          <Tab.Screen
            name="Dashboard"
            component={DashboardScreen}
            options={{ title: 'EAG-7000 Dashboard' }}
          />
          <Tab.Screen
            name="CAN Bus"
            component={CANBusScreen}
            options={{ title: 'CAN-FD Monitor' }}
          />
          <Tab.Screen
            name="Sensors"
            component={SensorsScreen}
            options={{ title: 'Sensor Data' }}
          />
          <Tab.Screen
            name="Settings"
            component={SettingsScreen}
            options={{ title: 'Settings' }}
          />
        </Tab.Navigator>
      </NavigationContainer>
    </BLEProvider>
  );
}

export default App;