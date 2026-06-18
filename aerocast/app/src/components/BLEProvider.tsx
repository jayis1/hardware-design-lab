/**
 * BLEProvider.tsx — BLE scan/pair/connect context for AeroCast
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Wraps react-native-ble-plx and exposes a single connect() function
 * plus a list of discovered AeroCast devices. The AeroCast GATT
 * service UUID is 0xFE5A; characteristics include:
 *   - 0xFE5B  live status (notify)
 *   - 0xFE5C  command inbox (write)
 *   - 0xFE5D  calibration blob (write)
 *   - 0xFE5E  device info (read)
 */
import React, { createContext, useContext, useState, useCallback, ReactNode } from 'react';
import { BleManager, Device, State as BLEState } from 'react-native-ble-plx';

export const AEROCAST_SERVICE = '0000fe5a-0000-1000-8000-00805f9b34fb';
export const CHR_STATUS  = '0000fe5b-0000-1000-8000-00805f9b34fb';
export const CHR_CMD     = '0000fe5c-0000-1000-8000-00805f9b34fb';
export const CHR_CALIB   = '0000fe5d-0000-1000-8000-00805f9b34fb';
export const CHR_INFO    = '0000fe5e-0000-1000-8000-00805f9b34fb';

interface BLECtx {
  scanning: boolean;
  devices: Device[];
  connected: Device | null;
  scan: () => Promise<void>;
  connect: (d: Device) => Promise<void>;
  disconnect: () => Promise<void>;
  sendCommand: (cmd: string) => Promise<void>;
  statusJson: any | null;
}

const Ctx = createContext<BLECtx | null>(null);
let manager: BleManager | null = null;

export function BLEProvider({ children }: { children: ReactNode }) {
  const [scanning, setScanning] = useState(false);
  const [devices, setDevices]   = useState<Device[]>([]);
  const [connected, setConnected] = useState<Device | null>(null);
  const [statusJson, setStatusJson] = useState<any | null>(null);

  if (!manager) manager = new BleManager();

  const scan = useCallback(async () => {
    setScanning(true);
    setDevices([]);
    manager!.startDeviceScan([AEROCAST_SERVICE], null, (err, dev) => {
      if (err) { setScanning(false); return; }
      if (dev && dev.name?.startsWith('AeroCast')) {
        setDevices(prev => prev.find(d => d.id === dev.id) ? prev : [...prev, dev]);
      }
    });
    setTimeout(() => { manager!.stopDeviceScan(); setScanning(false); }, 8000);
  }, []);

  const connect = useCallback(async (d: Device) => {
    const dev = await manager!.connectToDevice(d.id);
    await dev.discoverAllServicesAndCharacteristics();
    setConnected(dev);
    // Subscribe to live status notifications
    dev.monitorCharacteristicForService(AEROCAST_SERVICE, CHR_STATUS, (_e, ch) => {
      if (ch?.value) {
        try {
          const json = JSON.parse(atob(ch.value));
          setStatusJson(json);
        } catch { /* ignore malformed */ }
      }
    });
  }, []);

  const disconnect = useCallback(async () => {
    if (connected) { await connected.cancelConnection(); setConnected(null); setStatusJson(null); }
  }, [connected]);

  const sendCommand = useCallback(async (cmd: string) => {
    if (!connected) return;
    const b64 = btoa(cmd);
    await connected.writeCharacteristicWithResponseForService(
      AEROCAST_SERVICE, CHR_CMD, b64);
  }, [connected]);

  return (
    <Ctx.Provider value={{ scanning, devices, connected, scan, connect, disconnect, sendCommand, statusJson }}>
      {children}
    </Ctx.Provider>
  );
}

export function useBLE() {
  const c = useContext(Ctx);
  if (!c) throw new Error('useBLE must be inside BLEProvider');
  return c;
}