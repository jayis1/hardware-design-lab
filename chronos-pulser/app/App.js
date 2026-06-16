// App.js — Chronos Pulser Companion App Root
// React Native app with bottom tab navigation
// Dashboard, Waveform Viewer, Settings, and Device Info screens

import React, { useState, useEffect, useCallback } from 'react';
import { StatusBar, View, Text, StyleSheet, Alert } from 'react-native';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import DashboardScreen from './screens/DashboardScreen';
import WaveformScreen from './screens/WaveformScreen';
import SettingsScreen from './screens/SettingsScreen';
import DeviceInfoScreen from './screens/DeviceInfoScreen';
import DeviceContext, { DeviceProvider } from './components/DeviceContext';
import { ProtocolEngine } from './utils/protocol';

const Tab = createBottomTabNavigator();

// Tab bar icon component (simple text-based icons)
function TabIcon({ label, focused, color }) {
  const icons = {
    Dashboard: focused ? '◉' : '○',
    Waveform: focused ? '▣' : '□',
    Settings: focused ? '⚙' : '⚆',
    Device: focused ? 'ⓘ' : 'i',
  };
  return (
    <View style={styles.tabIconContainer}>
      <Text style={[styles.tabIcon, { color }]}>{icons[label] || '•'}</Text>
    </View>
  );
}

// Main App component
export default function App() {
  const [deviceState, setDeviceState] = useState({
    connected: false,
    deviceInfo: null,
    temperature: 25.0,
    pulseConfig: {
      frequency: 1000,
      amplitude: 250,
      enabled: false,
    },
    vgaGain: 10.0,
    calibrationValid: false,
    lastError: null,
  });

  const [protocolEngine, setProtocolEngine] = useState(null);

  // Initialize protocol engine
  useEffect(() => {
    const engine = new ProtocolEngine();
    engine.onDeviceConnected = (info) => {
      setDeviceState(prev => ({
        ...prev,
        connected: true,
        deviceInfo: info,
        calibrationValid: info.calibrationValid,
      }));
    };
    engine.onDeviceDisconnected = () => {
      setDeviceState(prev => ({
        ...prev,
        connected: false,
        deviceInfo: null,
      }));
    };
    engine.onTemperatureUpdate = (temp) => {
      setDeviceState(prev => ({ ...prev, temperature: temp }));
    };
    engine.onError = (error) => {
      setDeviceState(prev => ({ ...prev, lastError: error }));
    };
    setProtocolEngine(engine);

    return () => {
      if (engine) engine.disconnect();
    };
  }, []);

  // Connect to device
  const connectDevice = useCallback(async () => {
    if (!protocolEngine) return;
    try {
      await protocolEngine.connect();
    } catch (err) {
      Alert.alert('Connection Error', err.message || 'Failed to connect to Chronos Pulser');
    }
  }, [protocolEngine]);

  // Disconnect from device
  const disconnectDevice = useCallback(async () => {
    if (!protocolEngine) return;
    try {
      await protocolEngine.disconnect();
    } catch (err) {
      console.warn('Disconnect error:', err);
    }
  }, [protocolEngine]);

  // Update pulse configuration
  const updatePulseConfig = useCallback(async (config) => {
    if (!protocolEngine || !deviceState.connected) return;
    try {
      await protocolEngine.sendPulseConfig(config);
      setDeviceState(prev => ({ ...prev, pulseConfig: config }));
    } catch (err) {
      Alert.alert('Error', 'Failed to update pulse configuration');
    }
  }, [protocolEngine, deviceState.connected]);

  // Update VGA gain
  const updateVgaGain = useCallback(async (gain) => {
    if (!protocolEngine || !deviceState.connected) return;
    try {
      await protocolEngine.sendVgaGain(gain);
      setDeviceState(prev => ({ ...prev, vgaGain: gain }));
    } catch (err) {
      Alert.alert('Error', 'Failed to update VGA gain');
    }
  }, [protocolEngine, deviceState.connected]);

  // Start acquisition
  const startAcquisition = useCallback(async (params) => {
    if (!protocolEngine || !deviceState.connected) return null;
    try {
      return await protocolEngine.startAcquisition(params);
    } catch (err) {
      Alert.alert('Error', 'Failed to start acquisition');
      return null;
    }
  }, [protocolEngine, deviceState.connected]);

  // Run TDC calibration
  const runTdcCalibration = useCallback(async () => {
    if (!protocolEngine || !deviceState.connected) return null;
    try {
      const result = await protocolEngine.runTdcCalibration();
      setDeviceState(prev => ({ ...prev, calibrationValid: true }));
      return result;
    } catch (err) {
      Alert.alert('Error', 'TDC calibration failed');
      return null;
    }
  }, [protocolEngine, deviceState.connected]);

  // Device context value
  const deviceContextValue = {
    ...deviceState,
    protocolEngine,
    connectDevice,
    disconnectDevice,
    updatePulseConfig,
    updateVgaGain,
    startAcquisition,
    runTdcCalibration,
  };

  return (
    <SafeAreaProvider>
      <DeviceProvider value={deviceContextValue}>
        <NavigationContainer>
          <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />
          <Tab.Navigator
            screenOptions={({ route }) => ({
              headerStyle: {
                backgroundColor: '#1a1a2e',
              },
              headerTintColor: '#e0e0e0',
              headerTitleStyle: {
                fontWeight: 'bold',
                fontSize: 18,
              },
              tabBarStyle: {
                backgroundColor: '#16213e',
                borderTopColor: '#0f3460',
                borderTopWidth: 1,
                height: 60,
                paddingBottom: 8,
                paddingTop: 4,
              },
              tabBarActiveTintColor: '#e94560',
              tabBarInactiveTintColor: '#a0a0a0',
              tabBarIcon: ({ focused, color }) => (
                <TabIcon label={route.name} focused={focused} color={color} />
              ),
            })}
          >
            <Tab.Screen
              name="Dashboard"
              component={DashboardScreen}
              options={{
                title: 'Chronos Pulser',
                headerRight: () => (
                  <View style={styles.headerRight}>
                    <Text style={[
                      styles.connectionIndicator,
                      { color: deviceState.connected ? '#00ff88' : '#ff4444' }
                    ]}>
                      {deviceState.connected ? '● Connected' : '○ Disconnected'}
                    </Text>
                  </View>
                ),
              }}
            />
            <Tab.Screen
              name="Waveform"
              component={WaveformScreen}
              options={{ title: 'Waveform Viewer' }}
            />
            <Tab.Screen
              name="Settings"
              component={SettingsScreen}
              options={{ title: 'Settings' }}
            />
            <Tab.Screen
              name="Device"
              component={DeviceInfoScreen}
              options={{ title: 'Device Info' }}
            />
          </Tab.Navigator>
        </NavigationContainer>
      </DeviceProvider>
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  tabIconContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    width: 28,
    height: 28,
  },
  tabIcon: {
    fontSize: 22,
    fontWeight: 'bold',
  },
  headerRight: {
    marginRight: 16,
  },
  connectionIndicator: {
    fontSize: 12,
    fontWeight: '600',
  },
});
