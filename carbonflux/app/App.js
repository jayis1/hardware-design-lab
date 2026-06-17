/**
 * CarbonFlux — React Native Companion App
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Root application component. Sets up navigation, BLE context, and
 * the main tab navigator with four screens: Dashboard, Timeseries,
 * Sensor Detail, and Configuration.
 */

import React, {useEffect, useState} from 'react';
import {NavigationContainer} from '@react-navigation/native';
import {createBottomTabNavigator} from '@react-navigation/bottom-tabs';
import {SafeAreaProvider} from 'react-native-safe-area-context';
import {StatusBar} from 'react-native';
import BLEContext from './components/BLEContext';
import DashboardScreen from './screens/DashboardScreen';
import TimeseriesScreen from './screens/TimeseriesScreen';
import SensorDetailScreen from './screens/SensorDetailScreen';
import ConfigurationScreen from './screens/ConfigurationScreen';
import DataExportScreen from './screens/DataExportScreen';

const Tab = createBottomTabNavigator();

const App = () => {
  const [bleConnected, setBleConnected] = useState(false);
  const [deviceData, setDeviceData] = useState({
    state: 0,
    co2_ppm: 0,
    air_temp_c: 0,
    flux_umol: 0,
    battery_soc: 0,
    measurement_count: 0,
    pressure_hpa: 1013.25,
    soil_temp_5cm: -127,
    soil_temp_15cm: -127,
    soil_temp_30cm: -127,
    vwc_pct: 0,
    par_umol: 0,
  });

  return (
    <SafeAreaProvider>
      <BLEContext.Provider
        value={{
          bleConnected,
          setBleConnected,
          deviceData,
          setDeviceData,
        }}>
        <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />
        <NavigationContainer>
          <Tab.Navigator
            screenOptions={{
              headerStyle: {backgroundColor: '#1a1a2e'},
              headerTintColor: '#e0e0e0',
              tabBarStyle: {backgroundColor: '#1a1a2e', borderTopColor: '#333'},
              tabBarActiveTintColor: '#4CAF50',
              tabBarInactiveTintColor: '#888',
              tabBarLabelStyle: {fontSize: 11},
            }}>
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{title: 'CarbonFlux', tabBarLabel: 'Dashboard'}}
            />
            <Tab.Screen
              name="Timeseries"
              component={TimeseriesScreen}
              options={{title: 'Flux History', tabBarLabel: 'Timeseries'}}
            />
            <Tab.Screen
              name="Sensors"
              component={SensorDetailScreen}
              options={{title: 'Sensor Detail', tabBarLabel: 'Sensors'}}
            />
            <Tab.Screen
              name="Config"
              component={ConfigurationScreen}
              options={{title: 'Configuration', tabBarLabel: 'Config'}}
            />
            <Tab.Screen
              name="Data"
              component={DataExportScreen}
              options={{title: 'Data Export', tabBarLabel: 'Export'}}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </BLEContext.Provider>
    </SafeAreaProvider>
  );
};

export default App;