/**
 * WaveForge — Polyphonic Synthesizer Companion App
 * Main entry point with React Navigation
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';

import MainScreen from './screens/MainScreen';
import SettingsScreen from './screens/SettingsScreen';
import DataViewScreen from './screens/DataViewScreen';

const Stack = createNativeStackNavigator();

function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator
        initialRouteName="Main"
        screenOptions={{
          headerStyle: {
            backgroundColor: '#1a1a2e',
          },
          headerTintColor: '#e0e0e0',
          headerTitleStyle: {
            fontWeight: 'bold',
            fontSize: 18,
          },
          cardStyle: {
            backgroundColor: '#16213e',
          },
        }}
      >
        <Stack.Screen
          name="Main"
          component={MainScreen}
          options={{ title: 'WaveForge Synth' }}
        />
        <Stack.Screen
          name="Settings"
          component={SettingsScreen}
          options={{ title: 'Patch Settings' }}
        />
        <Stack.Screen
          name="DataView"
          component={DataViewScreen}
          options={{ title: 'CV Monitor' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}

export default App;