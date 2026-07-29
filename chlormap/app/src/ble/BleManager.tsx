// src/ble/BleManager.tsx — BLE connection manager for ChloroMap device
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, {
  createContext,
  useContext,
  useState,
  useEffect,
  useCallback,
  useRef,
} from 'react';
import { Measurement, DeviceStatus, parseMeasurementPacket, parseStatusPacket } from './protocol';

// ChloroMap BLE service UUID
const SERVICE_UUID = '0000c701-1212-efde-1523-785feabcd123';
const CHAR_MEASUREMENT_UUID = '0000c702-1212-efde-1523-785feabcd124';
const CHAR_COMMAND_UUID = '0000c703-1212-efde-1523-785feabcd125';
const CHAR_STATUS_UUID = '0000c704-1212-efde-1523-785feabcd126';

export type ConnectionState = 'disconnected' | 'scanning' | 'connecting' | 'connected';

interface BleContextValue {
  connectionState: ConnectionState;
  deviceName: string | null;
  status: DeviceStatus | null;
  latestMeasurement: Measurement | null;
  measurements: Measurement[];
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendCommand: (cmd: number[]) => Promise<void>;
  clearMeasurements: () => void;
}

const BleContext = createContext<BleContextValue | null>(null);

export function BleProvider({ children }: { children: React.ReactNode }) {
  const [connectionState, setConnectionState] = useState<ConnectionState>('disconnected');
  const [deviceName, setDeviceName] = useState<string | null>(null);
  const [status, setStatus] = useState<DeviceStatus | null>(null);
  const [latestMeasurement, setLatestMeasurement] = useState<Measurement | null>(null);
  const [measurements, setMeasurements] = useState<Measurement[]>([]);
  const reconnectRef = useRef(false);

  // Simulated connection (in production: use react-native-ble-plx or expo-ble)
  const connect = useCallback(async () => {
    setConnectionState('scanning');
    // In production:
    // 1. Start BLE scan for devices advertising SERVICE_UUID
    // 2. Stop scan when device found
    // 3. Connect to device
    // 4. Discover services + characteristics
    // 5. Subscribe to notifications on CHAR_MEASUREMENT_UUID + CHAR_STATUS_UUID
    await new Promise((resolve) => setTimeout(resolve, 800));
    setConnectionState('connecting');
    await new Promise((resolve) => setTimeout(resolve, 600));
    setDeviceName('ChloroMap-001');
    setConnectionState('connected');
    reconnectRef.current = true;

    // Simulate periodic status updates
    // In production: status characteristic notifies every 1s
  }, []);

  const disconnect = useCallback(async () => {
    reconnectRef.current = false;
    setConnectionState('disconnected');
    setDeviceName(null);
    setStatus(null);
    // In production: disconnect from BLE device
  }, []);

  const sendCommand = useCallback(async (cmd: number[]) => {
    if (connectionState !== 'connected') return;
    // In production: write to CHAR_COMMAND_UUID
    console.log('Sending BLE command:', cmd);
  }, [connectionState]);

  const clearMeasurements = useCallback(() => {
    setMeasurements([]);
    setLatestMeasurement(null);
  }, []);

  // Simulated incoming measurement (in production: BLE notification handler)
  useEffect(() => {
    if (connectionState !== 'connected') return;
    const interval = setInterval(() => {
      // Simulate a measurement notification
      const mockMeas: Measurement = {
        spad: 35 + Math.floor(Math.random() * 30),
        ndvi: 0.5 + Math.random() * 0.3,
        nsi: -0.05 + Math.random() * 0.15,
        lwbi: 0.95 + Math.random() * 0.15,
        rededge: 8 + Math.random() * 6,
        lat: 37.7749 + (Math.random() - 0.5) * 0.01,
        lon: -122.4194 + (Math.random() - 0.5) * 0.01,
        timestampMs: Date.now(),
        bands: [
          0.05 + Math.random() * 0.1,  // 450
          0.08 + Math.random() * 0.08, // 531
          0.06 + Math.random() * 0.08, // 660
          0.05 + Math.random() * 0.07, // 680
          0.15 + Math.random() * 0.15, // 700
          0.45 + Math.random() * 0.2,  // 800
          0.40 + Math.random() * 0.15, // 900
          0.35 + Math.random() * 0.15, // 970
        ],
        battMv: 3800 + Math.floor(Math.random() * 200),
        tempC: 22 + Math.random() * 6,
        sats: 8 + Math.floor(Math.random() * 5),
      };
      setLatestMeasurement(mockMeas);
      setMeasurements((prev) => [...prev.slice(-199), mockMeas]);
    }, 5000); // Simulated measurement every 5s

    return () => clearInterval(interval);
  }, [connectionState]);

  // Simulated status updates
  useEffect(() => {
    if (connectionState !== 'connected') return;
    const interval = setInterval(() => {
      setStatus({
        battMv: 3800 + Math.floor(Math.random() * 200),
        state: 0, // idle
        sats: 8 + Math.floor(Math.random() * 5),
        fixType: 3,
        battPct: 75 + Math.floor(Math.random() * 20),
      });
    }, 2000);
    return () => clearInterval(interval);
  }, [connectionState]);

  const value: BleContextValue = {
    connectionState,
    deviceName,
    status,
    latestMeasurement,
    measurements,
    connect,
    disconnect,
    sendCommand,
    clearMeasurements,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle(): BleContextValue {
  const ctx = useContext(BleContext);
  if (!ctx) {
    throw new Error('useBle must be used within BleProvider');
  }
  return ctx;
}