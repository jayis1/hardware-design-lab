/**
 * BleContext.js — BLE connection state and data management
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Provides a React Context that manages the BLE connection lifecycle,
 * parses incoming notifications from the LumiCast device, and exposes
 * the latest measurements to all screens.
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';

// GATT service and characteristic UUIDs (matching firmware).
const LUMICAST_SERVICE_UUID = '0000fc01-0000-1000-8000-00805f9b34fb';

const CHAR_UUIDS = {
  MELANOPIC_EDI:  '0000fc01-0000-1000-8000-00805f9b34fb',
  ILLUMINANCE:    '0000fc02-0000-1000-8000-00805f9b34fb',
  CCT:            '0000fc03-0000-1000-8000-00805f9b34fb',
  CIRCADIAN_CS:   '0000fc04-0000-1000-8000-00805f9b34fb',
  SPECTRUM:        '0000fc05-0000-1000-8000-00805f9b34fb',
  FLICKER:         '0000fc06-0000-1000-8000-00805f9b34fb',
  STATUS:          '0000fc07-0000-1000-8000-00805f9b34fb',
  COMMAND:         '0000fc08-0000-1000-8000-00805f9b34fb',
  TIMESTAMP:       '0000fc09-0000-1000-8000-00805f9b34fb',
};

const BleContext = createContext(null);

export function BleProvider({ children }) {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [measurements, setMeasurements] = useState({
    melanopicEdi: 0,
    illuminance: 0,
    cct: 0,
    duv: 0,
    x: 0,
    y: 0,
    circadianStimulus: 0,
    cla: 0,
    melanopicEr: 0,
    spectrum: new Array(11).fill(0),
    flickerPercent: 0,
    flickerIndex: 0,
    flickerFreq: 0,
    safetyRating: 100,
    gain: 1,
    atime: 100,
    timestamp: 0,
  });
  const [history, setHistory] = useState([]);
  const [logState, setLogState] = useState(false);

  // Request Android BLE permissions.
  const requestPermissions = useCallback(async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        v => v === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  }, []);

  // Scan for LumiCast devices.
  const scanAndConnect = useCallback(async () => {
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      console.warn('BLE permissions denied');
      return;
    }

    setScanning(true);
    manager.stopDeviceScan();

    manager.startDeviceScan(null, { allowDuplicates: false }, (error, dev) => {
      if (error) {
        console.error('Scan error:', error);
        setScanning(false);
        return;
      }

      if (dev.name === 'LumiCast' || dev.localName === 'LumiCast') {
        manager.stopDeviceScan();
        setScanning(false);

        dev.connect()
          .then(d => d.discoverAllServicesAndCharacteristics())
          .then(d => {
            setDevice(d);
            setConnected(true);
            setupNotifications(d);
          })
          .catch(err => console.error('Connect error:', err));
      }
    });
  }, [manager, requestPermissions]);

  // Setup BLE notifications for all measurement characteristics.
  const setupNotifications = useCallback(async (dev) => {
    const chars = await dev.characteristicsForService(LUMICAST_SERVICE_UUID);

    for (const char of chars) {
      if (char.uuid === CHAR_UUIDS.MELANOPIC_EDI) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            setMeasurements(prev => ({
              ...prev,
              melanopicEdi: bytesToFloat(data, 0),
            }));
          }
        });
      } else if (char.uuid === CHAR_UUIDS.ILLUMINANCE) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            setMeasurements(prev => ({
              ...prev,
              illuminance: bytesToFloat(data, 0),
              cct: bytesToFloat(data, 4),
              duv: bytesToFloat(data, 8),
            }));
          }
        });
      } else if (char.uuid === CHAR_UUIDS.CCT) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            setMeasurements(prev => ({
              ...prev,
              cct: bytesToFloat(data, 0),
              x: bytesToFloat(data, 4),
              y: bytesToFloat(data, 8),
            }));
          }
        });
      } else if (char.uuid === CHAR_UUIDS.CIRCADIAN_CS) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            const cs = bytesToFloat(data, 0);
            const cla = bytesToFloat(data, 4);
            const er = bytesToFloat(data, 8);
            setMeasurements(prev => ({
              ...prev,
              circadianStimulus: cs,
              cla,
              melanopicEr: er,
            }));

            // Append to history.
            setHistory(prev => {
              const newEntry = {
                timestamp: Date.now(),
                melanopicEdi: measurements.melanopicEdi,
                cs,
                cla,
              };
              const updated = [...prev, newEntry].slice(-256);
              return updated;
            });
          }
        });
      } else if (char.uuid === CHAR_UUIDS.SPECTRUM) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            const spectrum = [];
            for (let i = 0; i < 10; i++) {
              spectrum.push(data[i*2] | (data[i*2+1] << 8));
            }
            setMeasurements(prev => ({ ...prev, spectrum }));
          }
        });
      } else if (char.uuid === CHAR_UUIDS.FLICKER) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            setMeasurements(prev => ({
              ...prev,
              flickerPercent: bytesToFloat(data, 0),
              flickerFreq: bytesToFloat(data, 4),
              safetyRating: bytesToFloat(data, 8),
            }));
          }
        });
      } else if (char.uuid === CHAR_UUIDS.STATUS) {
        await char.monitor((err, c) => {
          if (c?.value) {
            const data = base64ToBytes(c.value);
            setMeasurements(prev => ({
              ...prev,
              gain: data[1],
              atime: data[2] | (data[3] << 8),
              timestamp: bytesToU32(data, 4),
            }));
          }
        });
      }
    }
  }, [measurements]);

  // Send a command to the device.
  const sendCommand = useCallback(async (cmd, data = []) => {
    if (!device || !connected) return;
    try {
      const char = await device.readCharacteristicForService(
        LUMICAST_SERVICE_UUID,
        CHAR_UUIDS.COMMAND
      );
      const payload = [cmd, ...data];
      await char.writeWithoutResponse(bytesToBase64(payload));
    } catch (err) {
      console.error('Command error:', err);
    }
  }, [device, connected]);

  const startLogging = useCallback(() => {
    sendCommand(0x01);
    setLogState(true);
  }, [sendCommand]);

  const stopLogging = useCallback(() => {
    sendCommand(0x02);
    setLogState(false);
  }, [sendCommand]);

  const calibrate = useCallback(() => {
    sendCommand(0x03);
  }, [sendCommand]);

  const setGain = useCallback((gain) => {
    sendCommand(0x04, [gain]);
  }, [sendCommand]);

  const setAtime = useCallback((atime) => {
    sendCommand(0x05, [atime]);
  }, [sendCommand]);

  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
      setConnected(false);
      setDevice(null);
    }
  }, [device]);

  // Cleanup on unmount.
  useEffect(() => {
    return () => {
      if (device) {
        device.cancelConnection();
      }
      manager.destroy();
    };
  }, []);

  const value = {
    manager,
    device,
    connected,
    scanning,
    measurements,
    history,
    logState,
    scanAndConnect,
    disconnect,
    startLogging,
    stopLogging,
    calibrate,
    setGain,
    setAtime,
    sendCommand,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) {
    throw new Error('useBle must be used within a BleProvider');
  }
  return ctx;
}

// --- Binary helpers ---

function base64ToBytes(b64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const clean = b64.replace(/=+$/, '');
  const bytes = [];
  for (let i = 0; i < clean.length; i += 4) {
    const c1 = chars.indexOf(clean[i]);
    const c2 = chars.indexOf(clean[i+1] || 'A');
    const c3 = chars.indexOf(clean[i+2] || 'A');
    const c4 = chars.indexOf(clean[i+3] || 'A');
    const n = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
    bytes.push((n >> 16) & 0xFF);
    if (clean.length > i + 2) bytes.push((n >> 8) & 0xFF);
    if (clean.length > i + 3) bytes.push(n & 0xFF);
  }
  return bytes;
}

function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let result = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b1 = bytes[i] || 0;
    const b2 = bytes[i+1] || 0;
    const b3 = bytes[i+2] || 0;
    const n = (b1 << 16) | (b2 << 8) | b3;
    result += chars[(n >> 18) & 0x3F];
    result += chars[(n >> 12) & 0x3F];
    result += (i + 1 < bytes.length) ? chars[(n >> 6) & 0x3F] : '=';
    result += (i + 2 < bytes.length) ? chars[n & 0x3F] : '=';
  }
  return result;
}

function bytesToFloat(bytes, offset) {
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  view.setUint8(0, bytes[offset]);
  view.setUint8(1, bytes[offset + 1]);
  view.setUint8(2, bytes[offset + 2]);
  view.setUint8(3, bytes[offset + 3]);
  return view.getFloat32(0, true);  // little-endian
}

function bytesToU32(bytes, offset) {
  return bytes[offset] | (bytes[offset+1] << 8) |
         (bytes[offset+2] << 16) | (bytes[offset+3] << 24);
}