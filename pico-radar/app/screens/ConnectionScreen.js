/**
 * screens/ConnectionScreen.js — BLE/USB device connection
 *
 * Scans for PicoRadar BLE peripherals, displays RSSI,
 * and establishes connection for data streaming.
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  FlatList,
  TouchableOpacity,
  ActivityIndicator,
} from 'react-native';
import { BleManager } from 'react-native-ble-plx';
import { protocol } from '../../App';

const PICORADAR_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
const PICORADAR_TX_CHAR_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E';
const PICORADAR_RX_CHAR_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E';

export default function ConnectionScreen({ navigation }) {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState([]);
  const [connecting, setConnecting] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState(null);

  const bleManager = new BleManager();

  useEffect(() => {
    return () => {
      bleManager.destroy();
    };
  }, []);

  const startScan = async () => {
    setDevices([]);
    setScanning(true);

    bleManager.startDeviceScan(
      [PICORADAR_SERVICE_UUID],
      { allowDuplicates: false },
      (error, device) => {
        if (error) {
          setScanning(false);
          return;
        }
        if (device.name && device.name.includes('PicoRadar')) {
          setDevices((prev) => {
            if (prev.find((d) => d.id === device.id)) return prev;
            return [...prev, device];
          });
        }
      }
    );

    // Stop scan after 10 seconds
    setTimeout(() => {
      bleManager.stopDeviceScan();
      setScanning(false);
    }, 10000);
  };

  const connectToDevice = async (device) => {
    setConnecting(true);
    try {
      await device.connect();
      await device.discoverAllServicesAndCharacteristics();

      const txChar = await device.readCharacteristicForService(
        PICORADAR_SERVICE_UUID,
        PICORADAR_TX_CHAR_UUID
      );

      // Subscribe to notifications (point cloud data)
      txChar.monitor((error, characteristic) => {
        if (error) return;
        if (characteristic && characteristic.value) {
          // Decode base64 and feed to protocol parser
          const raw = Buffer.from(characteristic.value, 'base64');
          protocol.feedData(raw);
        }
      });

      setConnectedDevice(device);
      protocol.setBleDevice(device, txChar);
      protocol.setConnected(true);

      navigation.goBack();
    } catch (err) {
      Alert.alert('Connection Failed', err.message);
    } finally {
      setConnecting(false);
    }
  };

  const renderDevice = ({ item }) => (
    <TouchableOpacity
      style={styles.deviceItem}
      onPress={() => connectToDevice(item)}
    >
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item.name || 'Unknown'}</Text>
        <Text style={styles.deviceId}>{item.id}</Text>
      </View>
      <Text style={styles.deviceRssi}>{item.rssi} dBm</Text>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Connect to PicoRadar</Text>
      <Text style={styles.subtitle}>
        Scan for BLE devices advertising the PicoRadar service
      </Text>

      <TouchableOpacity
        style={styles.scanButton}
        onPress={startScan}
        disabled={scanning || connecting}
      >
        <Text style={styles.scanButtonText}>
          {scanning ? 'Scanning...' : 'Scan for Devices'}
        </Text>
      </TouchableOpacity>

      {scanning && <ActivityIndicator color="#007AFF" style={styles.loader} />}

      <FlatList
        data={devices}
        keyExtractor={(item) => item.id}
        renderItem={renderDevice}
        ListEmptyComponent={
          !scanning ? (
            <Text style={styles.emptyText}>No devices found. Tap Scan.</Text>
          ) : null
        }
      />

      {connecting && (
        <View style={styles.connectingOverlay}>
          <ActivityIndicator size="large" color="#007AFF" />
          <Text style={styles.connectingText}>Connecting...</Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#1C1C1E',
    padding: 16,
  },
  title: {
    color: '#FFFFFF',
    fontSize: 24,
    fontWeight: '700',
    marginBottom: 8,
  },
  subtitle: {
    color: '#888888',
    fontSize: 14,
    marginBottom: 24,
  },
  scanButton: {
    backgroundColor: '#007AFF',
    paddingVertical: 14,
    borderRadius: 10,
    alignItems: 'center',
    marginBottom: 16,
  },
  scanButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '600',
  },
  loader: {
    marginVertical: 8,
  },
  deviceItem: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#2C2C2E',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: '500',
  },
  deviceId: {
    color: '#888888',
    fontSize: 11,
    marginTop: 2,
  },
  deviceRssi: {
    color: '#4CD964',
    fontSize: 13,
  },
  emptyText: {
    color: '#888888',
    fontSize: 14,
    textAlign: 'center',
    marginTop: 32,
  },
  connectingOverlay: {
    alignItems: 'center',
    paddingVertical: 16,
  },
  connectingText: {
    color: '#CCCCCC',
    fontSize: 14,
    marginTop: 8,
  },
});