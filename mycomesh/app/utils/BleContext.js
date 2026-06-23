/**
 * BleContext.js — BLE connection state and data management for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Provides a React Context that manages the BLE connection lifecycle,
 * parses incoming notifications from the MycoMesh device, and exposes
 * the latest electrophysiology and environmental data to all screens.
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';

// GATT service and characteristic UUIDs (matching firmware).
const MYCOMESH_SERVICE_UUID = '0000fc01-0000-1000-8000-00805f9b34fb';

const CHAR_UUIDS = {
  STATUS:     '0000fc01-0000-1000-8000-00805f9b34fb',
  EPOCH:      '0000fc02-0000-1000-8000-00805f9b34fb',
  SPIKES:     '0000fc03-0000-1000-8000-00805f9b34fb',
  WAVEFORM:   '0000fc04-0000-1000-8000-00805f9b34fb',
  ENV:        '0000fc05-0000-1000-8000-00805f9b34fb',
  COMMAND:    '0000fc06-0000-1000-8000-00805f9b34fb',
  CONFIG:     '0000fc07-0000-1000-8000-00805f9b34fb',
};

// Activity class labels (matching firmware constants).
const CLASS_LABELS = ['IDLE', 'FORAGE', 'TRANSPORT', 'STRESS', 'COMPETE'];
const CLASS_COLORS = ['#666', '#4CAF50', '#2196F3', '#FF5722', '#FF1744'];

const BleContext = createContext(null);

export function BleProvider({ children }) {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState(null);
  const [epoch, setEpoch] = useState(null);
  const [spikes, setSpikes] = useState([]);
  const [waveform, setWaveform] = useState([]);
  const [env, setEnv] = useState(null);
  const [error, setError] = useState(null);

  // Request Android BLE permissions.
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

  // Scan for MycoMesh devices.
  const startScan = useCallback(async () => {
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setError('BLE permissions denied');
      return;
    }

    setScanning(true);
    setError(null);

    manager.startDeviceScan(null, null, (error, foundDevice) => {
      if (error) {
        setError(error.message);
        setScanning(false);
        return;
      }

      if (foundDevice.name && foundDevice.name.includes('MycoMesh')) {
        manager.stopDeviceScan();
        setScanning(false);
        connectToDevice(foundDevice);
      }
    });

    // Stop scan after 10 seconds.
    setTimeout(() => {
      if (scanning) {
        manager.stopDeviceScan();
        setScanning(false);
      }
    }, 10000);
  }, [manager, requestPermissions, scanning]);

  // Connect to a device and set up notifications.
  const connectToDevice = useCallback(async (foundDevice) => {
    try {
      const connectedDevice = await foundDevice.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();
      setDevice(connectedDevice);
      setConnected(true);

      // Subscribe to status notifications.
      connectedDevice.monitorCharacteristicForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.STATUS,
        (err, char) => {
          if (err || !char?.value) return;
          const data = parseStatus(char.value);
          setStatus(data);
        }
      );

      // Subscribe to epoch notifications.
      connectedDevice.monitorCharacteristicForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.EPOCH,
        (err, char) => {
          if (err || !char?.value) return;
          const data = parseEpoch(char.value);
          setEpoch(data);
        }
      );

      // Subscribe to spike notifications.
      connectedDevice.monitorCharacteristicForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.SPIKES,
        (err, char) => {
          if (err || !char?.value) return;
          const data = parseSpikes(char.value);
          setSpikes((prev) => [...prev.slice(-200), ...data]);
        }
      );

      // Subscribe to waveform notifications.
      connectedDevice.monitorCharacteristicForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.WAVEFORM,
        (err, char) => {
          if (err || !char?.value) return;
          const data = parseWaveform(char.value);
          setWaveform(data);
        }
      );

      // Subscribe to environmental data.
      connectedDevice.monitorCharacteristicForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.ENV,
        (err, char) => {
          if (err || !char?.value) return;
          const data = parseEnv(char.value);
          setEnv(data);
        }
      );
    } catch (err) {
      setError(err.message);
      setConnected(false);
    }
  }, [manager]);

  // Send a command to the device.
  const sendCommand = useCallback(async (opcode, payload = []) => {
    if (!device || !connected) return;
    const data = base64Encode([opcode, ...payload]);
    try {
      await device.writeCharacteristicWithResponseForService(
        MYCOMESH_SERVICE_UUID,
        CHAR_UUIDS.COMMAND,
        data
      );
    } catch (err) {
      setError(err.message);
    }
  }, [device, connected]);

  // Disconnect from device.
  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      setConnected(false);
      setStatus(null);
      setEpoch(null);
      setSpikes([]);
      setWaveform([]);
      setEnv(null);
    }
  }, [device]);

  // Cleanup on unmount.
  useEffect(() => {
    return () => {
      if (device) device.cancelConnection();
      manager.destroy();
    };
  }, [device, manager]);

  const value = {
    manager,
    device,
    connected,
    scanning,
    status,
    epoch,
    spikes,
    waveform,
    env,
    error,
    classLabels: CLASS_LABELS,
    classColors: CLASS_COLORS,
    startScan,
    disconnect,
    sendCommand,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
}

// =====================================================================
//  Binary parsing helpers
// =====================================================================

function base64Decode(base64) {
  // React Native has no atob; use a simple decoder.
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = base64.replace(/=+$/, '');
  const bytes = [];
  let buffer = 0, bits = 0;
  for (const c of clean) {
    const idx = chars.indexOf(c);
    if (idx === -1) continue;
    buffer = (buffer << 6) | idx;
    bits += 6;
    if (bits >= 8) {
      bytes.push((buffer >> (bits - 8)) & 0xFF);
      bits -= 8;
    }
  }
  return bytes;
}

function base64Encode(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  let i = 0;
  while (i < bytes.length) {
    const a = bytes[i++] || 0;
    const b = bytes[i++] || 0;
    const c = bytes[i++] || 0;
    result += chars[(a >> 2) & 0x3F];
    result += chars[((a & 0x3) << 4) | ((b >> 4) & 0xF)];
    result += i < bytes.length ? chars[((b & 0xF) << 2) | ((c >> 6) & 0x3)] : '=';
    result += i < bytes.length ? chars[c & 0x3F] : '=';
  }
  return result;
}

function readInt16LE(bytes, offset) {
  const val = bytes[offset] | (bytes[offset + 1] << 8);
  return val > 0x7FFF ? val - 0x10000 : val;
}

function readUInt16LE(bytes, offset) {
  return bytes[offset] | (bytes[offset + 1] << 8);
}

function readUInt32LE(bytes, offset) {
  return (bytes[offset] | (bytes[offset + 1] << 8) |
          (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// Parse a 16-byte status payload.
function parseStatus(base64) {
  const b = base64Decode(base64);
  if (b.length < 16) return null;
  return {
    uptime: readUInt32LE(b, 0),
    batteryMv: readUInt16LE(b, 4),
    state: b[6],
    dutyCyclePct: b[7],
    channels: b[8],
    classLabel: b[10],
    sampleRate: readUInt16LE(b, 14),
  };
}

// Parse a 24-byte epoch summary payload.
function parseEpoch(base64) {
  const b = base64Decode(base64);
  if (b.length < 24) return null;
  return {
    classLabel: b[0],
    spikeRate: b[1],
    propagationCount: b[2],
    meanAmplitudeUv: readUInt16LE(b, 4),
    isiCvX100: readUInt16LE(b, 6),
    soilMoisture: readInt16LE(b, 8) / 10.0,
    soilTempC: readInt16LE(b, 10) / 10.0,
    co2Ppm: readUInt16LE(b, 12),
    batteryMv: readUInt16LE(b, 14),
    timestamp: readUInt32LE(b, 16),
  };
}

// Parse spike event notifications.
function parseSpikes(base64) {
  const b = base64Decode(base64);
  const spikes = [];
  // Each spike event is 8 bytes: channel(1), template(1), amplitude(2), timestamp(4)
  for (let i = 0; i + 8 <= b.length; i += 8) {
    spikes.push({
      channel: b[i],
      templateId: b[i + 1],
      amplitudeUv: readInt16LE(b, i + 2),
      timestampMs: readUInt32LE(b, i + 4),
    });
  }
  return spikes;
}

// Parse waveform data (16 channels × 4 samples = 128 bytes).
function parseWaveform(base64) {
  const b = base64Decode(base64);
  const channels = [];
  const nChannels = 16;
  const nSamples = Math.floor(b.length / (nChannels * 2));
  for (let ch = 0; ch < nChannels; ch++) {
    const samples = [];
    for (let s = 0; s < nSamples; s++) {
      samples.push(readInt16LE(b, (s * nChannels + ch) * 2));
    }
    channels.push(samples);
  }
  return channels;
}

// Parse environmental data (12 bytes).
function parseEnv(base64) {
  const b = base64Decode(base64);
  if (b.length < 12) return null;
  return {
    moisturePct: readInt16LE(b, 0) / 10.0,
    tempC: readInt16LE(b, 2) / 10.0,
    co2Ppm: readUInt16LE(b, 4),
    humidityPct: b[6],
    timestamp: readUInt32LE(b, 8),
  };
}