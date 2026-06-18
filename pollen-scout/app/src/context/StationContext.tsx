/**
 * StationContext.tsx - React context holding the active station state,
 * BLE connection, and MQTT subscription. Provides actions for
 * pairing, provisioning, and refreshing data.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, {
  createContext, useContext, useState, useEffect, useCallback,
} from 'react';
import {
  StationState, AlertRule, TaxaReading, WeatherReading, DeviceHealth,
  TAXA_NAMES, pollenIndex,
} from '../types';
import * as ble from '../ble';
import * as mqtt from '../mqtt';

interface StationContextValue {
  station: StationState | null;
  alerts: AlertRule[];
  scanning: boolean;
  discovered: ble.BleDevice[];
  connectLocal: (device: ble.BleDevice) => Promise<void>;
  connectRemote: (stationId: string) => Promise<void>;
  provision: (wifiSsid: string, wifiPw: string,
              joinEui: string, appKey: string) => Promise<void>;
  startScan: () => Promise<void>;
  setAlert: (rule: AlertRule) => void;
  removeAlert: (id: string) => void;
  triggerOta: () => Promise<void>;
}

const StationContext = createContext<StationContextValue | undefined>(undefined);

export function StationProvider({ children }: { children: React.ReactNode }) {
  const [station, setStation] = useState<StationState | null>(null);
  const [alerts, setAlerts] = useState<AlertRule[]>([
    { id: 'grass-default', classId: 30, thresholdGrainsM3: 50,
      enabled: true, lastTriggeredMs: 0 },
    { id: 'birch-default', classId: 3, thresholdGrainsM3: 50,
      enabled: true, lastTriggeredMs: 0 },
  ]);
  const [scanning, setScanning] = useState(false);
  const [discovered, setDiscovered] = useState<ble.BleDevice[]>([]);

  /* ---- Periodic refresh of station state ---- */
  useEffect(() => {
    if (!station) return;
    const id = setInterval(async () => {
      try {
        const next = station.isLocal
          ? await ble.pollState(station.stationId)
          : await mqtt.pollState(station.stationId);
        setStation(next);
        checkAlerts(next);
      } catch (e) {
        console.warn('poll failed', e);
      }
    }, 10_000);
    return () => clearInterval(id);
  }, [station]);

  const checkAlerts = (s: StationState) => {
    for (const rule of alerts) {
      if (!rule.enabled) continue;
      const t = s.taxa.find((x) => x.classId === rule.classId);
      if (!t) continue;
      if (t.concentration >= rule.thresholdGrainsM3) {
        const now = Date.now();
        if (now - rule.lastTriggeredMs > 60 * 60 * 1000) {
          notifyAlert(t.name, t.concentration);
          rule.lastTriggeredMs = now;
          setAlerts([...alerts]);
        }
      }
    }
  };

  const notifyAlert = (name: string, conc: number) => {
    /* In production uses expo-notifications. */
    console.log(`[Alert] ${name} = ${conc} grains/m³`);
  };

  const startScan = useCallback(async () => {
    setScanning(true);
    setDiscovered([]);
    try {
      const devs = await ble.scan(5000);
      setDiscovered(devs);
    } finally {
      setScanning(false);
    }
  }, []);

  const connectLocal = useCallback(async (device: ble.BleDevice) => {
    await ble.connect(device.id);
    const state = await ble.pollState(device.id);
    setStation(state);
  }, []);

  const connectRemote = useCallback(async (stationId: string) => {
    await mqtt.subscribe(stationId);
    const state = await mqtt.pollState(stationId);
    setStation(state);
  }, []);

  const provision = useCallback(async (
    wifiSsid: string, wifiPw: string,
    joinEui: string, appKey: string,
  ) => {
    if (!station) throw new Error('no station');
    await ble.provision(station.stationId, { wifiSsid, wifiPw, joinEui, appKey });
  }, [station]);

  const setAlert = useCallback((rule: AlertRule) => {
    setAlerts((prev) => {
      const idx = prev.findIndex((r) => r.id === rule.id);
      if (idx >= 0) { prev[idx] = rule; return [...prev]; }
      return [...prev, rule];
    });
  }, []);

  const removeAlert = useCallback((id: string) => {
    setAlerts((prev) => prev.filter((r) => r.id !== id));
  }, []);

  const triggerOta = useCallback(async () => {
    if (!station) return;
    await ble.sendCommand(station.stationId, 'O');
  }, [station]);

  const value: StationContextValue = {
    station, alerts, scanning, discovered,
    connectLocal, connectRemote, provision,
    startScan, setAlert, removeAlert, triggerOta,
  };

  return (
    <StationContext.Provider value={value}>
      {children}
    </StationContext.Provider>
  );
}

export function useStation(): StationContextValue {
  const ctx = useContext(StationContext);
  if (!ctx) throw new Error('useStation must be used within StationProvider');
  return ctx;
}

/* ---- Demo-data factory used by screens before a real station is paired ---- */
export function makeDemoStation(): StationState {
  const taxa: TaxaReading[] = [
    { classId: 30, name: TAXA_NAMES[30], confidence: 0.91, concentration: 73 },
    { classId: 3,  name: TAXA_NAMES[3],  confidence: 0.87, concentration: 41 },
    { classId: 36, name: TAXA_NAMES[36], confidence: 0.83, concentration: 218 },
    { classId: 1,  name: TAXA_NAMES[1],  confidence: 0.79, concentration: 9 },
    { classId: 8,  name: TAXA_NAMES[8],  confidence: 0.75, concentration: 12 },
    { classId: 37, name: TAXA_NAMES[37], confidence: 0.72, concentration: 55 },
  ];
  const weather: WeatherReading = {
    temperatureC: 22.4, humidityPct: 58, pressurePa: 101240,
    uvIndex: 5.2, windSpeedMps: 2.8, windDirDeg: 210,
    pm1_0: 6, pm2_5: 11, pm10: 18,
  };
  const health: DeviceHealth = {
    batteryMv: 6480, solarMv: 5900, chargePct: 82, charging: true,
    flowLpm: 2.01, sdUsagePct: 34, lastUplinkMs: 42_000,
    firmwareVersion: '1.0.0', captureCount: 18_402, roiCountTotal: 241_558,
  };
  const forecast: number[][] = [];
  for (let t = 0; t < 6; t++) {
    const row: number[] = [];
    const base = taxa[t].concentration;
    for (let h = 0; h < 72; h++) {
      const diurnal = 1 + 0.4 * Math.sin((h + t * 3) * Math.PI / 12);
      row.push(Math.max(0, base * diurnal * (1 - 0.005 * h)));
    }
    forecast.push(row);
  }
  return {
    stationId: 'demo-0001', name: 'Demo Station', isLocal: false,
    lastUpdateMs: Date.now(), taxa, forecast, weather, health,
  };
}

export { pollenIndex };