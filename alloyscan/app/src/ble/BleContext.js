/**
 * BleContext.js — BLE connection manager for AlloyScan device
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Manages the BLE connection to the AlloyScan hardware using the
 * Nordic UART Service (NUS). Provides a React Context for all
 * screens to access connection state, scan results, and commands.
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { BleManager, State } from 'react-native-ble-plx';
import { PermissionsAndroid, Platform } from 'react-native';

// Nordic UART Service UUIDs
const NUS_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_TX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';  // Device -> Phone
const NUS_RX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';  // Phone -> Device

const DEVICE_NAME_PREFIX = 'AlloyScan-';

const BleContext = createContext(null);

export const BleProvider = ({ children }) => {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [scanResult, setScanResult] = useState(null);
  const [scanHistory, setScanHistory] = useState([]);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [error, setError] = useState(null);

  // Request Android permissions
  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      try {
        const grants = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        ]);
        return Object.values(grants).every(
          g => g === PermissionsAndroid.RESULTS.GRANTED
        );
      } catch (e) {
        console.error('Permission error:', e);
        return false;
      }
    }
    return true;
  };

  // Scan for AlloyScan devices
  const startScan = useCallback(async () => {
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setError('BLE permissions denied');
      return;
    }

    setScanning(true);
    setError(null);

    manager.startDeviceScan(null, null, (scanError, scannedDevice) => {
      if (scanError) {
        console.error('Scan error:', scanError);
        setError(scanError.message);
        setScanning(false);
        return;
      }

      if (scannedDevice.name && scannedDevice.name.startsWith(DEVICE_NAME_PREFIX)) {
        manager.stopDeviceScan();
        setScanning(false);
        connectToDevice(scannedDevice);
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      if (scanning) {
        manager.stopDeviceScan();
        setScanning(false);
      }
    }, 10000);
  }, [manager]);

  // Connect to device
  const connectToDevice = useCallback(async (targetDevice) => {
    try {
      const connectedDevice = await targetDevice.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();

      // Subscribe to TX characteristic (device -> phone)
      connectedDevice.monitorCharacteristicForService(
        NUS_SERVICE_UUID,
        NUS_TX_CHAR_UUID,
        (monitorError, characteristic) => {
          if (monitorError) {
            console.error('Monitor error:', monitorError);
            return;
          }
          if (characteristic && characteristic.value) {
            handleDeviceMessage(characteristic.value);
          }
        }
      );

      setDevice(connectedDevice);
      setConnected(true);
      setError(null);
    } catch (e) {
      console.error('Connection error:', e);
      setError(e.message);
    }
  }, []);

  // Handle incoming BLE message from device
  const handleDeviceMessage = useCallback((base64Value) => {
    try {
      // Decode base64 to JSON string
      const decoded = atob(base64Value);
      const msg = JSON.parse(decoded);

      switch (msg.type) {
        case 'scan':
          const result = {
            alloy: msg.alloy,
            confidence: msg.confidence,
            alternatives: msg.alts || [],
            liftoff: msg.liftoff,
            timestamp: msg.ts || Date.now(),
            iq: msg.iq || null,
          };
          setScanResult(result);
          setScanHistory(prev => [...prev, result].slice(-1000));
          break;

        case 'status':
          setDeviceInfo(msg);
          break;

        case 'calibration':
          // Handle calibration step responses
          break;

        case 'error':
          setError(msg.message || 'Device error');
          break;

        default:
          console.log('Unknown message type:', msg.type);
      }
    } catch (e) {
      console.error('Message parse error:', e);
    }
  }, []);

  // Send command to device
  const sendCommand = useCallback(async (command) => {
    if (!device || !connected) {
      setError('Not connected to device');
      return;
    }

    try {
      const json = JSON.stringify(command) + '\n';
      const base64 = btoa(json);
      await device.writeCharacteristicWithResponseForService(
        NUS_SERVICE_UUID,
        NUS_RX_CHAR_UUID,
        base64
      );
    } catch (e) {
      console.error('Command send error:', e);
      setError(e.message);
    }
  }, [device, connected]);

  // Trigger a scan
  const triggerScan = useCallback(() => {
    sendCommand({ cmd: 'trigger' });
  }, [sendCommand]);

  // Start calibration
  const startCalibration = useCallback((step) => {
    sendCommand({ cmd: 'calibrate', step: step });
  }, [sendCommand]);

  // Set device mode
  const setMode = useCallback((mode) => {
    sendCommand({ cmd: 'mode', mode: mode });
  }, [sendCommand]);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (device) {
      try {
        await device.cancelConnection();
      } catch (e) {
        console.error('Disconnect error:', e);
      }
    }
    setDevice(null);
    setConnected(false);
    setScanResult(null);
  }, [device]);

  // Clear scan history
  const clearHistory = useCallback(() => {
    setScanHistory([]);
  }, []);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (device) {
        device.cancelConnection().catch(() => {});
      }
      manager.destroy();
    };
  }, []);

  const value = {
    manager,
    device,
    connected,
    scanning,
    scanResult,
    scanHistory,
    deviceInfo,
    error,
    startScan,
    triggerScan,
    startCalibration,
    setMode,
    disconnect,
    clearHistory,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
};

export const useBle = () => {
  const ctx = useContext(BleContext);
  if (!ctx) {
    throw new Error('useBle must be used within BleProvider');
  }
  return ctx;
};

// Base64 helpers (React Native may not have atob/btoa natively)
function atob(input) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let output = '';
  let str = input.replace(/=+$/, '');
  for (let bc = 0, bs = 0, i = 0; i < str.length; i++) {
    const buffer = chars.indexOf(str[i]);
    bs = (bc % 4) ? bs * 64 + buffer : buffer;
    if (bc++ % 4) output += String.fromCharCode(255 & bs >> ((-2 * bc) & 6));
  }
  return output;
}

function btoa(input) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let output = '';
  for (let block = 0, charCode, i = 0; i < input.length; block = 0) {
    for (let bit = 18; bit >= 0; bit -= 6) {
      block = block << 8;
      charCode = input.charCodeAt(i++);
      block = block | (charCode || 0);
      output += chars[(block >> bit) & 0x3F] || '=';
    }
  }
  return output;
}