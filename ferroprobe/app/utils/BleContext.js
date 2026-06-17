/**
 * BleContext.js — BLE Connection Manager
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Manages the BLE connection to the FerroProbe device, providing
 * connection state, command sending, and data streaming to all
 * screens via React Context.
 */

import React, { createContext, useState, useEffect, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { PermissionsAndroid, Platform } from 'react-native';
import * as protocol from './protocol';

// FerroProbe BLE service and characteristic UUIDs
const FERROPROBE_SERVICE_UUID = '0000f3b0-0000-1000-8000-00805f9b34fb';
const FERROPROBE_CMD_CHAR_UUID = '0000f3b1-0000-1000-8000-00805f9b34fb';
const FERROPROBE_DATA_CHAR_UUID = '0000f3b2-0000-1000-8000-00805f9b34fb';

const BleContext = createContext(null);

export const BleProvider = ({ children }) => {
  const manager = useRef(null);
  const [connectionState, setConnectionState] = useState('disconnected');
  const [device, setDevice] = useState(null);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [fieldData, setFieldData] = useState(null);
  const [status, setStatus] = useState(null);
  const [calibData, setCalibData] = useState(null);
  const [scanResults, setScanResults] = useState([]);
  const [error, setError] = useState(null);
  const rxBuffer = useRef([]);

  // Initialize BLE manager
  useEffect(() => {
    manager.current = new BleManager();
    return () => {
      if (manager.current) {
        manager.current.destroy();
      }
    };
  }, []);

  // Request Android permissions
  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        (r) => r === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  };

  // Scan for FerroProbe devices
  const scan = useCallback(async () => {
    const hasPerm = await requestPermissions();
    if (!hasPerm) {
      setError('Bluetooth permissions denied');
      return;
    }
    setScanResults([]);
    setError(null);
    manager.current.startDeviceScan(null, null, (err, foundDevice) => {
      if (err) {
        setError(err.message);
        return;
      }
      if (foundDevice.name && foundDevice.name.includes('FerroProbe')) {
        setScanResults((prev) => {
          const exists = prev.find((d) => d.id === foundDevice.id);
          if (exists) return prev;
          return [...prev, foundDevice];
        });
      }
    });
    // Stop scan after 10 seconds
    setTimeout(() => {
      manager.current.stopDeviceScan();
    }, 10000);
  }, []);

  // Connect to a device
  const connect = useCallback(async (deviceId) => {
    try {
      setConnectionState('connecting');
      const connectedDevice = await manager.current.connectToDevice(deviceId);
      await connectedDevice.discoverAllServicesAndCharacteristics();

      // Set up notification listener on data characteristic
      manager.current.startCharacteristicMonitoring(
        deviceId,
        FERROPROBE_SERVICE_UUID,
        FERROPROBE_DATA_CHAR_UUID,
        (err, characteristic) => {
          if (err) {
            setError(err.message);
            return;
          }
          if (characteristic && characteristic.value) {
            // Decode base64 to byte array
            const rawBytes = base64ToBytes(characteristic.value);
            processReceivedData(rawBytes);
          }
        }
      );

      setDevice(connectedDevice);
      setConnectionState('connected');

      // Request device version
      sendCommand(protocol.buildGetVersion());
      // Request status
      sendCommand(protocol.buildGetStatus());
    } catch (err) {
      setError(err.message);
      setConnectionState('disconnected');
    }
  }, []);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (device) {
      await manager.current.cancelDeviceConnection(device.id);
    }
    setDevice(null);
    setConnectionState('disconnected');
    setDeviceInfo(null);
    setFieldData(null);
    setStatus(null);
  }, [device]);

  // Send a command to the device
  const sendCommand = useCallback(async (frameBytes) => {
    if (!device || connectionState !== 'connected') return;
    try {
      // Convert to base64 for BLE transmission
      const base64 = bytesToBase64(frameBytes);
      await manager.current.writeCharacteristicWithResponse(
        device.id,
        FERROPROBE_SERVICE_UUID,
        FERROPROBE_CMD_CHAR_UUID,
        base64
      );
    } catch (err) {
      setError(err.message);
    }
  }, [device, connectionState]);

  // Process received data (accumulate into frames)
  const processReceivedData = useCallback((rawBytes) => {
    for (const byte of rawBytes) {
      if (byte === protocol.SOF) {
        rxBuffer.current = [];
      }
      rxBuffer.current.push(byte);
      if (byte === protocol.EOF && rxBuffer.current.length >= 5) {
        const frame = [...rxBuffer.current];
        const parsed = protocol.parseFrame(frame);
        if (parsed) {
          handleResponse(parsed);
        }
        rxBuffer.current = [];
      }
    }
  }, []);

  // Handle parsed response frames
  const handleResponse = useCallback((parsed) => {
    switch (parsed.opcode) {
      case protocol.RSP.STATUS:
        setStatus(protocol.parseStatus(parsed.payload));
        break;
      case protocol.RSP.STREAM:
        setFieldData(protocol.parseFieldData(parsed.payload));
        break;
      case protocol.RSP.VERSION:
        setDeviceInfo(protocol.parseVersion(parsed.payload));
        break;
      case protocol.RSP.CALIB_DATA:
        setCalibData(protocol.parseCalibData(parsed.payload));
        break;
      case protocol.RSP.LOG_CHUNK:
        // Handle log chunk (for data download)
        break;
      case protocol.RSP.OK:
        // Command acknowledged
        break;
      case protocol.RSP.ERROR:
        setError(`Device error: opcode=${parsed.payload[0]}, code=${parsed.payload[1]}`);
        break;
      default:
        break;
    }
  }, []);

  // Convenience methods for common commands
  const startSurvey = useCallback(() => sendCommand(protocol.buildStartSurvey()), [sendCommand]);
  const stopSurvey = useCallback(() => sendCommand(protocol.buildStopSurvey()), [sendCommand]);
  const startCalib = useCallback(() => sendCommand(protocol.buildStartCalib()), [sendCommand]);
  const startStream = useCallback(() => sendCommand(protocol.buildStreamStart()), [sendCommand]);
  const stopStream = useCallback(() => sendCommand(protocol.buildStreamStop()), [sendCommand]);
  const setRate = useCallback((rate) => sendCommand(protocol.buildSetRate(rate)), [sendCommand]);
  const setThreshold = useCallback((nt) => sendCommand(protocol.buildSetThreshold(nt)), [sendCommand]);
  const getCalib = useCallback(() => sendCommand(protocol.buildGetCalib()), [sendCommand]);
  const getStatus = useCallback(() => sendCommand(protocol.buildGetStatus()), [sendCommand]);

  // Reconnect on disconnection
  useEffect(() => {
    if (device) {
      const subscription = manager.current.onDeviceDisconnected(device.id, (err, dev) => {
        setConnectionState('disconnected');
        setDevice(null);
      });
      return () => subscription.remove();
    }
  }, [device]);

  const value = {
    connectionState,
    device,
    deviceInfo,
    fieldData,
    status,
    calibData,
    scanResults,
    error,
    scan,
    connect,
    disconnect,
    sendCommand,
    startSurvey,
    stopSurvey,
    startCalib,
    startStream,
    stopStream,
    setRate,
    setThreshold,
    getCalib,
    getStatus,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
};

export const useBle = () => {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
};

// Utility: base64 string → byte array
function base64ToBytes(base64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const result = [];
  for (let i = 0; i < base64.length; i += 4) {
    const enc1 = chars.indexOf(base64[i]);
    const enc2 = chars.indexOf(base64[i + 1]);
    const enc3 = chars.indexOf(base64[i + 2]);
    const enc4 = chars.indexOf(base64[i + 3]);
    const byte1 = (enc1 << 2) | (enc2 >> 4);
    result.push(byte1);
    if (enc3 !== 64) {
      const byte2 = ((enc2 & 15) << 4) | (enc3 >> 2);
      result.push(byte2);
    }
    if (enc4 !== 64) {
      const byte3 = ((enc3 & 3) << 6) | enc4;
      result.push(byte3);
    }
  }
  return result;
}

// Utility: byte array → base64 string
function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const byte1 = bytes[i] || 0;
    const byte2 = bytes[i + 1] || 0;
    const byte3 = bytes[i + 2] || 0;
    const enc1 = byte1 >> 2;
    const enc2 = ((byte1 & 3) << 4) | (byte2 >> 4);
    const enc3 = ((byte2 & 15) << 2) | (byte3 >> 6);
    const enc4 = byte3 & 63;
    result += chars[enc1] + chars[enc2];
    result += (i + 1 < bytes.length) ? chars[enc3] : '=';
    result += (i + 2 < bytes.length) ? chars[enc4] : '=';
  }
  return result;
}