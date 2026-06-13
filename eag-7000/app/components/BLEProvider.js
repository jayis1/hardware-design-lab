/**
 * EAG-7000 — BLE Provider Component
 *
 * React context provider that manages the Bluetooth Low Energy connection
 * to the EAG-7000 Edge AI Gateway. Handles scanning, connecting,
 * reading/writing characteristics, and subscribing to notifications.
 *
 * Uses the Nordic UART Service (NUS) compatible UUID scheme for
 * bidirectional communication with the M4F firmware.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

import React, { createContext, useState, useCallback, useRef } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import BleManager from 'react-native-ble-manager';
import { BLE } from '../utils/protocol';

export const BLEContext = createContext({
  connected: false,
  scanning: false,
  device: null,
  deviceInfo: { model: '', firmware: '', hardware: '' },
  startScan: async () => [],
  stopScan: async () => {},
  connect: async () => {},
  disconnect: async () => {},
  writeCharacteristic: async () => {},
  subscribeToCharacteristic: () => () => {},
});

const EAG7000_NAME_PREFIX = 'EAG-7000';

export function BLEProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [device, setDevice] = useState(null);
  const [deviceInfo, setDeviceInfo] = useState({
    model: '',
    firmware: '',
    hardware: '',
  });

  const notificationListeners = useRef([]);
  const peripheralId = useRef(null);

  // Request BLE permissions (Android)
  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      try {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        ]);
        return Object.values(granted).every(
          v => v === PermissionsAndroid.RESULTS.GRANTED
        );
      } catch (err) {
        console.error('BLE permission error:', err);
        return false;
      }
    }
    return true;
  };

  // Start BLE scan for EAG-7000 devices
  const startScan = useCallback(async () => {
    try {
      await requestPermissions();
      await BleManager.start({ showAlert: false });

      setScanning(true);
      const foundDevices = [];

      return new Promise((resolve) => {
        BleManager.scan([BLE.SERVICE_UUID], 5, true)
          .then(() => {
            // Scan will run for 5 seconds
          })
          .catch(err => console.error('Scan error:', err));

        // Listen for discovered devices
        const handler = BleManager.addListener(
          'BleManagerDiscoverPeripheral',
          (peripheral) => {
            if (peripheral.name && peripheral.name.startsWith(EAG7000_NAME_PREFIX)) {
              foundDevices.push({
                id: peripheral.id,
                name: peripheral.name,
                rssi: peripheral.rssi,
              });
            }
          }
        );

        // Stop scan after 5 seconds and return results
        setTimeout(async () => {
          await BleManager.stopScan();
          setScanning(false);
          handler.remove();
          resolve(foundDevices);
        }, 5000);
      });
    } catch (err) {
      console.error('Scan failed:', err);
      setScanning(false);
      return [];
    }
  }, []);

  // Stop BLE scan
  const stopScan = useCallback(async () => {
    try {
      await BleManager.stopScan();
      setScanning(false);
    } catch (err) {
      console.error('Stop scan error:', err);
    }
  }, []);

  // Connect to a BLE device
  const connect = useCallback(async (deviceItem) => {
    try {
      await BleManager.connect(deviceItem.id);
      peripheralId.current = deviceItem.id;

      // Retrieve services and characteristics
      const peripheralInfo = await BleManager.retrieveServices(deviceItem.id);

      // Start notification listener for TX characteristic
      await BleManager.startNotification(
        deviceItem.id,
        BLE.SERVICE_UUID,
        BLE.TX_CHAR_UUID
      );

      // Read device info from standard Device Information Service
      try {
        const modelData = await BleManager.read(
          deviceItem.id,
          BLE.DEVICE_INFO_SERVICE,
          BLE.MODEL_CHAR
        );
        const fwData = await BleManager.read(
          deviceItem.id,
          BLE.DEVICE_INFO_SERVICE,
          BLE.FIRMWARE_CHAR
        );
        const hwData = await BleManager.read(
          deviceItem.id,
          BLE.DEVICE_INFO_SERVICE,
          BLE.HARDWARE_CHAR
        );

        setDeviceInfo({
          model: String.fromCharCode(...new Uint8Array(modelData)),
          firmware: String.fromCharCode(...new Uint8Array(fwData)),
          hardware: String.fromCharCode(...new Uint8Array(hwData)),
        });
      } catch (readErr) {
        // Device info service may not be available — use defaults
        console.warn('Could not read device info:', readErr);
        setDeviceInfo({
          model: 'EAG-7000',
          firmware: '--',
          hardware: 'Rev A',
        });
      }

      setDevice(deviceItem);
      setConnected(true);
    } catch (err) {
      console.error('Connect error:', err);
      throw err;
    }
  }, []);

  // Disconnect from BLE device
  const disconnect = useCallback(async () => {
    try {
      if (peripheralId.current) {
        await BleManager.stopNotification(
          peripheralId.current,
          BLE.SERVICE_UUID,
          BLE.TX_CHAR_UUID
        );
        await BleManager.disconnect(peripheralId.current);
      }
    } catch (err) {
      console.error('Disconnect error:', err);
    } finally {
      setConnected(false);
      setDevice(null);
      peripheralId.current = null;
    }
  }, []);

  // Write data to RX characteristic (App → M4)
  const writeCharacteristic = useCallback(async (data) => {
    if (!peripheralId.current || !connected) {
      throw new Error('Not connected to gateway');
    }

    // Convert Uint8Array to base64 or use writeWithType
    const bytes = data instanceof Uint8Array ? data : new Uint8Array(data);

    await BleManager.write(
      peripheralId.current,
      BLE.SERVICE_UUID,
      BLE.RX_CHAR_UUID,
      Array.from(bytes),
      20  // MTU-based chunk size (20 bytes for safe BLE transmission)
    );
  }, [connected]);

  // Subscribe to notifications from TX characteristic (M4 → App)
  const subscribeToCharacteristic = useCallback((callback) => {
    // Add listener to our list
    const listener = { callback };
    notificationListeners.current.push(listener);

    // Set up BleManager notification handler if not already done
    if (notificationListeners.current.length === 1) {
      BleManager.addListener(
        'BleManagerDidUpdateValueForCharacteristic',
        ({ value, characteristic, peripheral }) => {
          if (characteristic.toUpperCase() === BLE.TX_CHAR_UUID.toUpperCase()) {
            const data = new Uint8Array(value);
            notificationListeners.current.forEach(l => {
              try {
                l.callback(data.buffer);
              } catch (err) {
                console.error('Notification listener error:', err);
              }
            });
          }
        }
      );
    }

    // Return unsubscribe function
    return () => {
      notificationListeners.current = notificationListeners.current.filter(
        l => l !== listener
      );
    };
  }, []);

  const value = {
    connected,
    scanning,
    device,
    deviceInfo,
    startScan,
    stopScan,
    connect,
    disconnect,
    writeCharacteristic,
    subscribeToCharacteristic,
  };

  return (
    <BLEContext.Provider value={value}>
      {children}
    </BLEContext.Provider>
  );
}