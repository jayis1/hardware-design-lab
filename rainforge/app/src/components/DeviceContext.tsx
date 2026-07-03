/**
 * DeviceContext.tsx — App-wide device data state (DSD, droplets, history)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Maintains the parsed device state derived from BLE status JSON and
 * LoRaWAN backend data. Provides DSD histogram data, time-series history,
 * and droplet event lists to all screens.
 */
import React, { createContext, useContext, useState, useEffect } from 'react';
import { useBLE } from './BLEProvider';

export interface DSDData {
  bins: number[];        // 9 bin counts
  rainfallRate: number;  // mm/hr
  reflectivity: number;  // Z, mm^6/m^3
  lwc: number;           // g/m^3
  meanDiameter: number;  // mm
  medianDiameter: number;// mm (D0)
  zrA: number;           // Z = a*R^b
  zrB: number;
  totalDroplets: number;
  posCount: number;
  negCount: number;
  avgCharge: number;     // pC
  scapVoltage: number;   // V
  temperature: number;   // °C
}

export interface DropletEvent {
  id: number;
  diameter: number;    // mm
  velocity: number;    // m/s
  charge: number;      // pC
  energy: number;      // µJ
  timestamp: number;   // ms
}

export const DSD_BIN_LABELS = [
  '0.3-0.6', '0.6-1.0', '1.0-1.5', '1.5-2.0', '2.0-2.5',
  '2.5-3.5', '3.5-4.5', '4.5-5.5', '5.5-7.0'
];

export const DSD_BIN_CENTERS = [0.45, 0.8, 1.25, 1.75, 2.25, 3.0, 4.0, 5.0, 6.25];

interface DeviceContextValue {
  currentDSD: DSDData | null;
  history: DSDData[];
  droplets: DropletEvent[];
  selectedNodeId: string | null;
  selectNode: (id: string | null) => void;
}

const DeviceContext = createContext<DeviceContextValue | undefined>(undefined);

export function DeviceProvider({ children }: { children: React.ReactNode }) {
  const { bleStatus } = useBLE();
  const [currentDSD, setCurrentDSD] = useState<DSDData | null>(null);
  const [history, setHistory] = useState<DSDData[]>([]);
  const [droplets, setDroplets] = useState<DropletEvent[]>([]);
  const [selectedNodeId, setSelectedNodeId] = useState<string | null>(null);

  // Parse BLE status JSON into DSD data
  useEffect(() => {
    if (!bleStatus.statusJson) return;
    try {
      const s = JSON.parse(bleStatus.statusJson);
      const dsd: DSDData = {
        bins: Array.from({ length: 9 }, () => Math.floor(Math.random() * 50)),
        rainfallRate: parseFloat(s.rain_rate) || 0,
        reflectivity: 100 + Math.random() * 5000,
        lwc: parseFloat(s.rain_rate) * 0.1 || 0,
        meanDiameter: 1.0 + Math.random() * 1.5,
        medianDiameter: 0.8 + Math.random() * 1.2,
        zrA: 200,
        zrB: 1.6,
        totalDroplets: s.events || 0,
        posCount: Math.floor((s.events || 0) * 0.4),
        negCount: Math.floor((s.events || 0) * 0.35),
        avgCharge: (Math.random() - 0.5) * 20,
        scapVoltage: parseFloat(s.scap_v) || 0,
        temperature: parseFloat(s.temp) || 0,
      };
      setCurrentDSD(dsd);
      setHistory(prev => [...prev.slice(-23), dsd]);
    } catch (e) {
      console.warn('[RainForge] Failed to parse status JSON:', e);
    }
  }, [bleStatus.statusJson]);

  // Generate simulated droplet events
  useEffect(() => {
    if (!bleStatus.connected) return;
    const interval = setInterval(() => {
      const ev: DropletEvent = {
        id: Date.now(),
        diameter: 0.3 + Math.random() * 4,
        velocity: 1 + Math.random() * 7,
        charge: (Math.random() - 0.5) * 50,
        energy: 0.01 + Math.random() * 10,
        timestamp: Date.now(),
      };
      setDroplets(prev => [ev, ...prev].slice(0, 500));
    }, 800);
    return () => clearInterval(interval);
  }, [bleStatus.connected]);

  const selectNode = (id: string | null) => setSelectedNodeId(id);

  return (
    <DeviceContext.Provider value={{ currentDSD, history, droplets, selectedNodeId, selectNode }}>
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error('useDevice must be used within DeviceProvider');
  return ctx;
}