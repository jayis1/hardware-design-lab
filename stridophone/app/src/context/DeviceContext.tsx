/**
 * DeviceContext.tsx — app-wide device list + active selection.
 *
 * Author : jayis1
 * License: MIT
 */
import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { StridoClient, DeviceInfo } from '../api';

export interface PairedDevice {
  id: string;          // mDNS hostname or BLE MAC
  ip: string;
  name: string;
}

interface DeviceCtx {
  devices: PairedDevice[];
  active: PairedDevice | null;
  client: StridoClient | null;
  addDevice: (d: PairedDevice) => Promise<void>;
  removeDevice: (id: string) => Promise<void>;
  setActive: (d: PairedDevice | null) => void;
}

const Ctx = createContext<DeviceCtx | undefined>(undefined);
const STORAGE_KEY = '@stridophone/devices';

export function DeviceProvider({ children }: { children: ReactNode }) {
  const [devices, setDevices] = useState<PairedDevice[]>([]);
  const [active, setActiveState] = useState<PairedDevice | null>(null);

  useEffect(() => {
    AsyncStorage.getItem(STORAGE_KEY).then((raw) => {
      if (raw) setDevices(JSON.parse(raw));
    });
  }, []);

  const persist = (list: PairedDevice[]) =>
    AsyncStorage.setItem(STORAGE_KEY, JSON.stringify(list));

  const addDevice = async (d: PairedDevice) => {
    setDevices((prev) => {
      const next = [...prev.filter((p) => p.id !== d.id), d];
      persist(next);
      return next;
    });
  };

  const removeDevice = async (id: string) => {
    setDevices((prev) => {
      const next = prev.filter((p) => p.id !== id);
      persist(next);
      return next;
    });
  };

  const setActive = (d: PairedDevice | null) => setActiveState(d);

  const client = active ? new StridoClient(active.ip) : null;

  return (
    <Ctx.Provider value={{ devices, active, client, addDevice, removeDevice, setActive }}>
      {children}
    </Ctx.Provider>
  );
}

export function useDevices() {
  const c = useContext(Ctx);
  if (!c) throw new Error('useDevices must be inside DeviceProvider');
  return c;
}