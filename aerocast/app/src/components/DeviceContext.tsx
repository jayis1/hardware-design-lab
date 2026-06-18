/**
 * DeviceContext.tsx — holds the active AeroCast device state + helpers
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
import React, { createContext, useContext, useState, ReactNode } from 'react';

export interface TaxonCount {
  classId: number;
  name: string;
  count: number;
  concentration: number;  // grains/m³
}

export interface MinuteEvent {
  epoch_min: number;
  flow: number;
  T: number;
  RH: number;
  P: number;
  windDir: number;
  windSpeed: number;
  counts: number[];   // 24 classes
}

interface DeviceCtx {
  deviceId: string | null;
  setDeviceId: (id: string | null) => void;
  history: MinuteEvent[];
  pushHistory: (e: MinuteEvent) => void;
  forecast: ForecastDay[];
  setForecast: (f: ForecastDay[]) => void;
}

export interface ForecastDay {
  date: string;
  risk: 'low' | 'moderate' | 'high' | 'very-high';
  topTaxon: string;
  score: number;
}

const Ctx = createContext<DeviceCtx | null>(null);

export const TAXON_NAMES = [
  'unclassified','grass','birch','ragweed','oak','pine','cypress','nettle',
  'plantain','alternaria','cladosporium','aspergillus','penicillium',
  'botrytis','ustilago','ganoderma','mineral_dust','sea_salt','soot',
  'tire_wear','pollen_fragment','water','insect_debris','fiber'
];

export function DeviceProvider({ children }: { children: ReactNode }) {
  const [deviceId, setDeviceId] = useState<string | null>(null);
  const [history, setHistory]   = useState<MinuteEvent[]>([]);
  const [forecast, setForecast] = useState<ForecastDay[]>([]);

  const pushHistory = (e: MinuteEvent) =>
    setHistory(prev => [...prev.slice(-1439), e]);  // keep 24h

  return (
    <Ctx.Provider value={{ deviceId, setDeviceId, history, pushHistory, forecast, setForecast }}>
      {children}
    </Ctx.Provider>
  );
}

export function useDevice() {
  const c = useContext(Ctx);
  if (!c) throw new Error('useDevice must be inside DeviceProvider');
  return c;
}