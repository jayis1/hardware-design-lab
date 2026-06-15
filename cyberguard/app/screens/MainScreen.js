/**
 * MainScreen.js - CyberGuard Main Dashboard
 * Shows device connection status, battery, and quick actions
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  ScrollView,
  Alert,
  Vibration,
} from 'react-native';
import { BleManager } from 'react-native-ble-plx';
import StatusCard from '../components/StatusCard';
import { CyberGuardProtocol, COMMANDS } from '../utils/protocol';

const BLE_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const BLE_TX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const BLE_RX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

export default function MainScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [deviceName, setDeviceName] = useState('');
  const [batteryLevel, setBatteryLevel] = useState(null);
  const [fingerprintCount, setFingerprintCount] = useState(0);
  const [credentialCount, setCredentialCount] = useState(0);
  const [lastAuthTime, setLastAuthTime] = useState(null);
  const [deviceVersion, setDeviceVersion] = useState('');

  const bleManager = React.useRef(new BleManager()).current;
  const deviceRef = React.useRef(null);
  const protocolRef = React.useRef(new CyberGuardProtocol());

  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, []);

  const handleConnect = useCallback(async () => {
    if (connected) {
      // Disconnect
      if (deviceRef.current) {
        await deviceRef.current.cancelConnection();
        deviceRef.current = null;
      }
      setConnected(false);
      setDeviceName('');
      setBatteryLevel(null);
      Vibration.vibrate(50);
      return;
    }

    try {
      setScanning(true);
      const device = await bleManager.connectToDevice(
        null, // Will scan for CyberGuard devices
        { autoConnect: true }
      );

      // In production, we'd scan for devices advertising our service UUID
      // For now, show a simplified flow
      Alert.alert(
        'Scanning',
        'Searching for CyberGuard devices nearby...',
        [{ text: 'OK' }]
      );

      // Simulated connection flow for demonstration
      // In production: use bleManager.startDeviceScan() with service UUID filter

      deviceRef.current = device;
      setConnected(true);
      setDeviceName(device.name || 'CyberGuard Token');
      Vibration.vibrate([0, 50, 50, 50]);

      // Request device info
      await requestDeviceInfo();
    } catch (error) {
      Alert.alert('Connection Error', error.message);
    } finally {
      setScanning(false);
    }
  }, [connected]);

  const requestDeviceInfo = useCallback(async () => {
    if (!deviceRef.current || !connected) return;

    try {
      // Send GET_INFO command
      const frame = protocolRef.current.buildFrame(
        COMMANDS.GET_INFO,
        Buffer.alloc(0)
      );

      // Write to BLE characteristic
      await deviceRef.current.writeCharacteristicWithResponseForService(
        BLE_SERVICE_UUID,
        BLE_TX_CHAR_UUID,
        frame.toString('base64')
      );

      // In production, read response from RX characteristic
      // const response = await device.readCharacteristicForService(...)

      // Parse response (simulated)
      setDeviceVersion('1.0.0');
      setFingerprintCount(0);
      setCredentialCount(0);
      setBatteryLevel(85);
    } catch (error) {
      console.error('Failed to get device info:', error);
    }
  }, [connected]);

  const handleAuthenticate = useCallback(() => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to your CyberGuard token first.');
      return;
    }

    Vibration.vibrate(100);
    Alert.alert(
      'Authentication',
      'Touch your CyberGuard token to authenticate.',
      [
        {
          text: 'Cancel',
          style: 'cancel',
        },
        {
          text: 'Authenticate',
          onPress: async () => {
            // Send AUTHENTICATE command
            const challenge = crypto.getRandomValues(new Uint8Array(32));
            const frame = protocolRef.current.buildFrame(
              COMMANDS.AUTHENTICATE,
              Buffer.from(challenge)
            );

            // In production: send via BLE and wait for signed response
            setLastAuthTime(new Date().toLocaleTimeString());
            Vibration.vibrate([0, 100, 50, 100]);
          },
        },
      ]
    );
  }, [connected]);

  const handleEnrollFingerprint = useCallback(() => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to your CyberGuard token first.');
      return;
    }

    navigation.navigate('Settings');
  }, [connected, navigation]);

  const handleManageCredentials = useCallback(() => {
    if (!connected) {
      Alert.alert('Not Connected', 'Please connect to your CyberGuard token first.');
      return;
    }

    navigation.navigate('Credentials');
  }, [connected, navigation]);

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      <Text style={styles.title}>🛡️ CyberGuard</Text>
      <Text style={styles.subtitle}>Multi-Protocol Security Token</Text>

      {/* Connection Status */}
      <StatusCard
        title="Connection"
        value={connected ? deviceName : 'Not Connected'}
        status={connected ? 'connected' : 'disconnected'}
        onPress={handleConnect}
        buttonText={connected ? 'Disconnect' : scanning ? 'Scanning...' : 'Connect'}
        loading={scanning}
      />

      {/* Battery Level */}
      {connected && batteryLevel !== null && (
        <StatusCard
          title="Battery"
          value={`${batteryLevel}%`}
          status={batteryLevel > 20 ? 'good' : 'low'}
        />
      )}

      {/* Device Info */}
      {connected && (
        <View style={styles.infoGrid}>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Fingerprints</Text>
            <Text style={styles.infoValue}>{fingerprintCount}/10</Text>
          </View>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Credentials</Text>
            <Text style={styles.infoValue}>{credentialCount}/50</Text>
          </View>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Version</Text>
            <Text style={styles.infoValue}>{deviceVersion}</Text>
          </View>
          <View style={styles.infoItem}>
            <Text style={styles.infoLabel}>Last Auth</Text>
            <Text style={styles.infoValue}>
              {lastAuthTime || 'Never'}
            </Text>
          </View>
        </View>
      )}

      {/* Quick Actions */}
      {connected && (
        <View style={styles.actions}>
          <TouchableOpacity
            style={[styles.actionButton, styles.authButton]}
            onPress={handleAuthenticate}
          >
            <Text style={styles.actionButtonText}>🔑 Authenticate</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[styles.actionButton, styles.enrollButton]}
            onPress={handleEnrollFingerprint}
          >
            <Text style={styles.actionButtonText}>👆 Enroll Fingerprint</Text>
          </TouchableOpacity>

          <TouchableOpacity
            style={[styles.actionButton, styles.credButton]}
            onPress={handleManageCredentials}
          >
            <Text style={styles.actionButtonText}>📋 Manage Credentials</Text>
          </TouchableOpacity>
        </View>
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#16213e',
  },
  content: {
    padding: 16,
    alignItems: 'center',
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#e94560',
    marginTop: 20,
  },
  subtitle: {
    fontSize: 14,
    color: '#a0a0b0',
    marginBottom: 24,
  },
  infoGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    width: '100%',
    marginTop: 12,
  },
  infoItem: {
    width: '48%',
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 12,
    marginBottom: 8,
  },
  infoLabel: {
    fontSize: 12,
    color: '#a0a0b0',
  },
  infoValue: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#ffffff',
    marginTop: 4,
  },
  actions: {
    width: '100%',
    marginTop: 16,
  },
  actionButton: {
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    marginBottom: 12,
  },
  authButton: {
    backgroundColor: '#e94560',
  },
  enrollButton: {
    backgroundColor: '#0f3460',
  },
  credButton: {
    backgroundColor: '#533483',
  },
  actionButtonText: {
    color: '#ffffff',
    fontSize: 16,
    fontWeight: '600',
  },
});