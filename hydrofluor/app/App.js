// App.js — HydroFluor companion app root
// Author: jayis1  License: MIT
//
// React Native (Expo) app that connects to the HydroFluor water-quality
// fluorescence sonde over BLE 5, shows live analyte concentrations and the
// raw 6×4 fluorescence matrix, guides the user through calibration, plots
// GPS-tagged samples on a map, and configures deployment / logging.
import React, { useState } from 'react';
import { StatusBar } from 'expo-status-bar';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Text } from 'react-native';
import { theme } from './utils/theme';
import ble from './utils/ble';
import ConnectScreen from './screens/ConnectScreen';
import LiveMonitorScreen from './screens/LiveMonitorScreen';
import CalibrationScreen from './screens/CalibrationScreen';
import SurveyMapScreen from './screens/SurveyMapScreen';
import DeploymentsScreen from './screens/DeploymentsScreen';

const Tab = createBottomTabNavigator();

export default function App() {
  const [connected, setConnected] = useState(ble.isConnected());

  const handleConnected = (info) => {
    setConnected(true);
    if (info) console.log('HydroFluor connected:', info);
  };

  return (
    <SafeAreaView style={{ flex: 1, backgroundColor: theme.colors.bg }}>
      <StatusBar style="light" />
      <NavigationContainer>
        <Tab.Navigator
          screenOptions={({ route }) => ({
            tabBarActiveTintColor: theme.colors.accent,
            tabBarInactiveTintColor: theme.colors.textDim,
            tabBarStyle: { backgroundColor: theme.colors.surface, borderTopColor: theme.colors.gridLine },
            headerShown: false,
          })}
        >
          {!connected ? (
            <Tab.Screen name="Connect">
              {() => <ConnectScreen onConnected={handleConnected} />}
            </Tab.Screen>
          ) : (
            <>
              <Tab.Screen name="Monitor"    component={LiveMonitorScreen}   options={{ tabBarIcon: () => <Text style={{ fontSize: 14 }}>📊</Text> }} />
              <Tab.Screen name="Calibrate"  component={CalibrationScreen}   options={{ tabBarIcon: () => <Text style={{ fontSize: 14 }}>🔬</Text> }} />
              <Tab.Screen name="Map"        component={SurveyMapScreen}      options={{ tabBarIcon: () => <Text style={{ fontSize: 14 }}>🗺</Text> }} />
              <Tab.Screen name="Deploy"     component={DeploymentsScreen}   options={{ tabBarIcon: () => <Text style={{ fontSize: 14 }}>⚙️</Text> }} />
            </>
          )}
        </Tab.Navigator>
      </NavigationContainer>
    </SafeAreaView>
  );
}