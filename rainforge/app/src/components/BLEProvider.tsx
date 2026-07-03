/**
 * BLEProvider.tsx — BLE connection manager for RainForge nodes
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Manages scanning for, connecting to, and communicating with RainForge
 * devices over BLE. The RainForge firmware exposes a custom GATT service:
 *
 *   Service UUID: 0000RF01-0000-1000-8000-00805F9B34FB
 *     Characteristic 0000RF02 (notify):  status JSON (27-byte payload + meta)
 *     Characteristic 0000RF03 (write):   command bytes
 *     Characteristic 0000RF04 (notify):  droplet event stream
 *     Characteristic 0000RF05 (read):    calibration table
 *     Characteristic 0000RF06 (write):   calibration coefficients
 */
import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';

const SERVICE_UUID      = '0000rf01-0000-1000-8000-00805f9b34fb';
const CHAR_STATUS_UUID  = '0000rf02-0000-1000-8000-00805f9b34fb';
const CHAR_COMMAND_UUID = '0000rf03-0000-1000-8000-00805f9b34fb';
const CHAR_EVENT_UUID   = '0000rf04-0000-1000-8000-00805f9b34fb';
const CHAR_CALIB_UUID   = '0000rf05-0000-1000-8000-00805f9b34fb';

export interface BLEStatus {
  connected: boolean;
  deviceName: string | null;
  deviceMac: string | null;
  rssi: number;
  statusJson: string | null;
  lastEventHex: string | null;
}

interface BLEContextValue {
  bleStatus: BLEStatus;
  scanning: boolean;
  devices: BLEDevice[];
  startScan: () => void;
  connect: (deviceId: string) => Promise<void>;
  disconnect: () => Promise<void>;
  sendCommand: (bytes: number[]) => Promise<void>;
}

export interface BLEDevice {
  id: string;
  name: string;
  rssi: number;
}

const BLEContext = createContext<BLEContextValue | undefined>(undefined);

export function BLEProvider({ children }: { children: React.ReactNode }) {
  const [bleStatus, setBleStatus] = useState<BLEStatus>({
    connected: false,
    deviceName: null,
    deviceMac: null,
    rssi: 0,
    statusJson: null,
    lastEventHex: null,
  });
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices] = useState<BLEDevice[]>([]);

  // Stub: in a real build this uses react-native-ble-plx's BleManager.
  // We simulate scan/connect for the UI prototype.
  const startScan = useCallback(() => {
    setScanning(true);
    setDevices([]);
    // Simulate discovering RainForge nodes
    setTimeout(() => {
      setDevices([
        { id: 'rf-node-001', name: 'RainForge-001', rssi: -62 },
        { id: 'rf-node-002', name: 'RainForge-002', rssi: -71 },
        { id: 'rf-node-003', name: 'RainForge-003', rssi: -85 },
      ]);
      setScanning(false);
    }, 1500);
  }, []);

  const connect = useCallback(async (deviceId: string) => {
    const dev = devices.find(d => d.id === deviceId);
    if (!dev) return;
    setBleStatus(prev => ({
      ...prev,
      connected: true,
      deviceName: dev.name,
      deviceMac: dev.id,
      rssi: dev.rssi,
    }));
  }, [devices]);

  const disconnect = useCallback(async () => {
    setBleStatus({
      connected: false,
      deviceName: null,
      deviceMac: null,
      rssi: 0,
      statusJson: null,
      lastEventHex: null,
    });
  }, []);

  const sendCommand = useCallback(async (bytes: number[]) => {
    // In a real build: await manager.writeCharacteristicWithResponseForDevice(
    //   bleStatus.deviceMac!, SERVICE_UUID, CHAR_COMMAND_UUID, base64(bytes))
    console.log('[RainForge] BLE command:', bytes.map(b => b.toString(16).padStart(2, '0')).join(' '));
  }, [bleStatus.deviceMac]);

  // Simulated status updates when connected
  useEffect(() => {
    if (!bleStatus.connected) return;
    const interval = setInterval(() => {
      const mockStatus = JSON.stringify({
        state: 'HARVESTING',
        events: Math.floor(Math.random() * 500),
        scap_v: (3.3 + Math.random() * 1.5).toFixed(2),
        temp: (15 + Math.random() * 8).toFixed(1),
        rain_rate: (Math.random() * 5).toFixed(2),
      });
      setBleStatus(prev => ({ ...prev, statusJson: mockStatus }));
    }, 2000);
    return () => clearInterval(interval);
  }, [bleStatus.connected]);

  return (
    <BLEContext.Provider value={{ bleStatus, scanning, devices, startScan, connect, disconnect, sendCommand }}>
      {children}
    </BLEContext.Provider>
  );
}

export function useBLE() {
  const ctx = useContext(BLEContext);
  if (!ctx) throw new Error('useBLE must be used within BLEProvider');
  return ctx;
}