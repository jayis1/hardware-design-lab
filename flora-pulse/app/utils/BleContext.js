/**
 * BleContext.js — BLE Connection Context Provider
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Provides a React Context for managing BLE connection to the FloraPulse
 * device. Handles scanning, connection, command sending, and data reception.
 */

import React, { createContext, useState, useRef, useCallback } from 'react';
import { PermissionsAndroid, Platform } from 'react-native';
import { BleManager } from 'react-native-ble-plx';
import { buildFrame, parseFrame, CMD, RSP } from './protocol';

// FloraPulse BLE Service UUID (Nordic UART Service)
const FLORAPULSE_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const FLORAPULSE_TX_CHAR = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // Write
const FLORAPULSE_RX_CHAR = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // Notify

const BleContext = createContext(null);

export function BleProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const [lastSample, setLastSample] = useState(null);
  const [lastStream, setLastStream] = useState(null);
  const [events, setEvents] = useState([]);

  const managerRef = useRef(null);
  const deviceRef = useRef(null);
  const frameBufferRef = useRef([]);
  const frameHandlerRef = useRef(null);

  // Initialize BLE manager
  const getManager = useCallback(() => {
    if (!managerRef.current) {
      managerRef.current = new BleManager();
    }
    return managerRef.current;
  }, []);

  // Request permissions (Android)
  const requestPermissions = useCallback(async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        (v) => v === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  }, []);

  // Scan for FloraPulse devices
  const scanAndConnect = useCallback(async () => {
    const hasPerm = await requestPermissions();
    if (!hasPerm) return;

    const manager = getManager();
    setScanning(true);

    manager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.error('Scan error:', error);
        setScanning(false);
        return;
      }

      if (device.name && device.name.includes('FloraPulse')) {
        manager.stopDeviceScan();
        setScanning(false);
        setDeviceInfo({ id: device.id, name: device.name });

        // Connect
        device
          .connect()
          .then((d) => d.discoverAllServicesAndCharacteristics())
          .then((d) => {
            deviceRef.current = d;
            setConnected(true);
            setupNotifications(d);
          })
          .catch((e) => console.error('Connection error:', e));
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      if (scanning) {
        manager.stopDeviceScan();
        setScanning(false);
      }
    }, 10000);
  }, [requestPermissions, getManager]);

  // Setup notification listener
  const setupNotifications = useCallback((device) => {
    device.monitorCharacteristicForService(
      FLORAPULSE_SERVICE_UUID,
      FLORAPULSE_RX_CHAR,
      (error, characteristic) => {
        if (error) {
          console.error('Notification error:', error);
          return;
        }

        if (characteristic && characteristic.value) {
          // Decode base64 to bytes
          const bytes = decodeBase64(characteristic.value);
          processIncoming(bytes);
        }
      }
    );
  }, []);

  // Process incoming BLE data bytes
  const processIncoming = useCallback((bytes) => {
    const buf = frameBufferRef.current;
    buf.push(...bytes);

    // Look for complete frames
    while (buf.length >= 5) {
      // Find SOF
      const sofIdx = buf.indexOf(0xAA);
      if (sofIdx === -1) {
        buf.length = 0;
        break;
      }
      if (sofIdx > 0) {
        buf.splice(0, sofIdx);
      }

      if (buf.length < 5) break;

      const len = buf[1];
      const frameLen = len + 4; // SOF + LEN + CRC + EOF

      if (buf.length < frameLen) break;

      // Check EOF
      if (buf[frameLen - 1] !== 0x55) {
        buf.shift();
        continue;
      }

      // Extract and parse frame
      const frame = buf.splice(0, frameLen);
      const parsed = parseFrame(frame);

      if (parsed && frameHandlerRef.current) {
        frameHandlerRef.current(parsed.opcode, parsed.payload);
      }
    }
  }, []);

  // Decode base64 to byte array
  function decodeBase64(str) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const bytes = [];
    let buffer = 0, bits = 0;

    for (const c of str) {
      if (c === '=') break;
      const val = chars.indexOf(c);
      if (val === -1) continue;
      buffer = (buffer << 6) | val;
      bits += 6;
      if (bits >= 8) {
        bytes.push((buffer >> (bits - 8)) & 0xFF);
        bits -= 8;
      }
    }
    return bytes;
  }

  // Encode byte array to base64
  function encodeBase64(bytes) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let result = '';
    let i = 0;
    while (i < bytes.length) {
      const a = bytes[i++] || 0;
      const b = bytes[i++] || 0;
      const c = bytes[i++] || 0;
      result += chars[a >> 2];
      result += chars[((a & 3) << 4) | (b >> 4)];
      result += i < bytes.length + 2 ? chars[((b & 15) << 2) | (c >> 6)] : '=';
      result += i < bytes.length + 1 ? chars[c & 63] : '=';
    }
    return result;
  }

  // Send a command to the device
  const sendCommand = useCallback(async (opcode, payload = []) => {
    if (!deviceRef.current || !connected) return;

    const frame = buildFrame(opcode, payload);
    const base64 = encodeBase64(frame);

    try {
      await deviceRef.current.writeCharacteristicWithResponseForService(
        FLORAPULSE_SERVICE_UUID,
        FLORAPULSE_TX_CHAR,
        base64
      );
    } catch (e) {
      console.error('Write error:', e);
    }
  }, [connected]);

  // Set frame handler
  const setFrameHandler = useCallback((handler) => {
    frameHandlerRef.current = handler;
  }, []);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (deviceRef.current) {
      await deviceRef.current.cancelConnection();
      deviceRef.current = null;
    }
    setConnected(false);
    setDeviceInfo(null);
    setLastSample(null);
    setLastStream(null);
  }, []);

  // Add an event to the events list
  const addEvent = useCallback((event) => {
    setEvents((prev) => [event, ...prev].slice(0, 100));
  }, []);

  const value = {
    connected,
    scanning,
    deviceInfo,
    lastSample,
    lastStream,
    events,
    scanAndConnect,
    disconnect,
    sendCommand,
    setFrameHandler,
    addEvent,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = React.useContext(BleContext);
  if (!ctx) {
    throw new Error('useBle must be used within BleProvider');
  }
  return ctx;
}