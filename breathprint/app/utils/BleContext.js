/**
 * BleContext.js — BLE connection management and state
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Provides a React Context that manages BLE scanning, connection,
 * and data reception from the BreathPrint device.
 */

import React, {
  createContext,
  useContext,
  useState,
  useRef,
  useCallback,
  useEffect,
} from 'react';
import { BleManager, State as BleState } from 'react-native-ble-plx';
import { decodeBreathResult, decodeSampleData, encodeCommand } from './protocol';
import AsyncStorage from '@react-native-async-storage/async-storage';

// BreathPrint BLE Service UUID
const SERVICE_UUID = '1b7f0000-0000-1000-8000-00805f9b34fb';
const CHAR_CMD_UUID = '1b7f0001-0000-1000-8000-00805f9b34fb';
const CHAR_SAMPLE_UUID = '1b7f0002-0000-1000-8000-00805f9b34fb';
const CHAR_RESULT_UUID = '1b7f0003-0000-1000-8000-00805f9b34fb';
const CHAR_STATUS_UUID = '1b7f0004-0000-1000-8000-00805f9b34fb';
const CHAR_LOG_UUID = '1b7f0005-0000-1000-8000-00805f9b34fb';
const CHAR_OTA_UUID = '1b7f0006-0000-1000-8000-00805f9b34fb';

const BleContext = createContext(null);

export const BleProvider = ({ children }) => {
  const manager = useRef(null);
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState({
    state: 'idle',
    battery: 0,
    charging: false,
  });
  const [lastResult, setLastResult] = useState(null);
  const [liveSample, setLiveSample] = useState(null);
  const [history, setHistory] = useState([]);
  const [error, setError] = useState(null);

  // Initialize BLE manager
  useEffect(() => {
    manager.current = new BleManager();

    // Load history from storage
    loadHistory();

    return () => {
      if (manager.current) {
        manager.current.destroy();
      }
    };
  }, []);

  // Load breath history from AsyncStorage
  const loadHistory = async () => {
    try {
      const stored = await AsyncStorage.getItem('breathprint_history');
      if (stored) {
        setHistory(JSON.parse(stored));
      }
    } catch (e) {
      console.error('Failed to load history:', e);
    }
  };

  // Save history to AsyncStorage
  const saveHistory = async (newHistory) => {
    try {
      await AsyncStorage.setItem(
        'breathprint_history',
        JSON.stringify(newHistory.slice(-500)) // Keep last 500
      );
    } catch (e) {
      console.error('Failed to save history:', e);
    }
  };

  // Start scanning for BreathPrint devices
  const startScan = useCallback(async () => {
    if (!manager.current || scanning) return;

    setScanning(true);
    setError(null);

    try {
      manager.current.startDeviceScan(
        [SERVICE_UUID],
        { allowDuplicates: false },
        (error, scannedDevice) => {
          if (error) {
            setError(error.message);
            setScanning(false);
            return;
          }

          if (scannedDevice && scannedDevice.name) {
            if (scannedDevice.name.includes('BreathPrint')) {
              manager.current.stopDeviceScan();
              setScanning(false);
              connectToDevice(scannedDevice);
            }
          }
        }
      );

      // Stop scan after 10 seconds
      setTimeout(() => {
        if (scanning) {
          manager.current.stopDeviceScan();
          setScanning(false);
        }
      }, 10000);
    } catch (e) {
      setError(e.message);
      setScanning(false);
    }
  }, [scanning]);

  // Connect to a BreathPrint device
  const connectToDevice = useCallback(async (targetDevice) => {
    try {
      setError(null);
      const connectedDevice = await targetDevice.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();

      // Set up notification listeners
      await setupNotifications(connectedDevice);

      setDevice(connectedDevice);
      setConnected(true);

      // Read initial status
      await readStatus(connectedDevice);
    } catch (e) {
      setError(e.message);
      setConnected(false);
    }
  }, []);

  // Set up BLE notification handlers
  const setupNotifications = async (dev) => {
    // Sample data notifications (real-time sensor stream)
    await dev.monitorCharacteristicForService(
      SERVICE_UUID,
      CHAR_SAMPLE_UUID,
      (error, characteristic) => {
        if (error || !characteristic?.value) return;
        const sample = decodeSampleData(characteristic.value);
        setLiveSample(sample);
      }
    );

    // Result notifications (final analysis)
    await dev.monitorCharacteristicForService(
      SERVICE_UUID,
      CHAR_RESULT_UUID,
      (error, characteristic) => {
        if (error || !characteristic?.value) return;
        const result = decodeBreathResult(characteristic.value);
        setLastResult(result);

        // Add to history
        setHistory((prev) => {
          const newHistory = [...prev, result];
          saveHistory(newHistory);
          return newHistory;
        });
      }
    );

    // Status notifications
    await dev.monitorCharacteristicForService(
      SERVICE_UUID,
      CHAR_STATUS_UUID,
      (error, characteristic) => {
        if (error || !characteristic?.value) return;
        // Parse status: [state, battery, charging, reserved]
        const bytes = base64ToBytes(characteristic.value);
        setStatus({
          state: bytes[0],
          battery: bytes[1],
          charging: bytes[2] === 1,
        });
      }
    );
  };

  // Read device status
  const readStatus = async (dev) => {
    try {
      const char = await dev.readCharacteristicForService(
        SERVICE_UUID,
        CHAR_STATUS_UUID
      );
      if (char?.value) {
        const bytes = base64ToBytes(char.value);
        setStatus({
          state: bytes[0],
          battery: bytes[1],
          charging: bytes[2] === 1,
        });
      }
    } catch (e) {
      console.error('Failed to read status:', e);
    }
  };

  // Send a command to the device
  const sendCommand = useCallback(
    async (command, data = null) => {
      if (!device || !connected) {
        setError('Not connected to device');
        return false;
      }

      try {
        const encoded = encodeCommand(command, data);
        await device.writeCharacteristicWithResponseForService(
          SERVICE_UUID,
          CHAR_CMD_UUID,
          encoded
        );
        return true;
      } catch (e) {
        setError(e.message);
        return false;
      }
    },
    [device, connected]
  );

  // Start a breath analysis
  const startAnalysis = useCallback(() => {
    setLiveSample(null);
    return sendCommand(0x01); // BLE_CMD_START_SAMPLE
  }, [sendCommand]);

  // Start calibration
  const startCalibration = useCallback(() => {
    return sendCommand(0x02); // BLE_CMD_CALIBRATE
  }, [sendCommand]);

  // Set device time
  const setDeviceTime = useCallback(
    (timestamp) => {
      const data = new Uint8Array(4);
      data[0] = timestamp & 0xff;
      data[1] = (timestamp >> 8) & 0xff;
      data[2] = (timestamp >> 16) & 0xff;
      data[3] = (timestamp >> 24) & 0xff;
      return sendCommand(0x09, data); // BLE_CMD_SET_TIME
    },
    [sendCommand]
  );

  // Disconnect from device
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
    setStatus({ state: 'idle', battery: 0, charging: false });
  }, [device]);

  // Clear history
  const clearHistory = useCallback(async () => {
    setHistory([]);
    await AsyncStorage.removeItem('breathprint_history');
  }, []);

  // Helper: base64 to byte array
  const base64ToBytes = (base64) => {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const bytes = [];
    for (let i = 0; i < base64.length; i += 4) {
      const bits =
        (chars.indexOf(base64[i]) << 18) |
        (chars.indexOf(base64[i + 1]) << 12) |
        (chars.indexOf(base64[i + 2]) << 6) |
        chars.indexOf(base64[i + 3]);
      bytes.push((bits >> 16) & 0xff);
      if (base64[i + 2] !== '=') bytes.push((bits >> 8) & 0xff);
      if (base64[i + 3] !== '=') bytes.push(bits & 0xff);
    }
    return bytes;
  };

  const value = {
    manager: manager.current,
    device,
    connected,
    scanning,
    status,
    lastResult,
    liveSample,
    history,
    error,
    startScan,
    disconnect,
    sendCommand,
    startAnalysis,
    startCalibration,
    setDeviceTime,
    clearHistory,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
};

export const useBle = () => {
  const context = useContext(BleContext);
  if (!context) {
    throw new Error('useBle must be used within BleProvider');
  }
  return context;
};