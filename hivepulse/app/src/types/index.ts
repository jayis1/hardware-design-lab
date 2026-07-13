/**
 * HivePulse App type definitions
 * Author: jayis1
 * License: MIT
 */

export enum ColonyState {
  QueenrightHealthy = 0,
  Queenless = 1,
  PreparingSwarm = 2,
  VarroaHigh = 3,
  WinterCluster = 4,
  DeadInactive = 5,
}

export interface HiveData {
  id: string;
  name: string;
  location: {
    lat: number;
    lng: number;
    label: string;
  };
  lastUpdated: number; // Unix timestamp
  colonyState: ColonyState;
  confidence: number; // 0.0 - 1.0
  battery: number; // mV
  solar: number; // mV
  weight: number; // kg
  temperatures: number[]; // 8 sensors in °C
  humidity: number; // %RH
  co2: number; // ppm
  beesIn: number;
  beesOut: number;
  winterMode: boolean;
  swarmAlert: boolean;
  queenlessAlert: boolean;
}

export interface HiveHistory {
  hiveId: string;
  points: HistoryPoint[];
}

export interface HistoryPoint {
  timestamp: number;
  colonyState: ColonyState;
  confidence: number;
  weight: number;
  ambientTemp: number;
  broodTemp: number;
  beesIn: number;
  beesOut: number;
}

export interface AudioLevel {
  channel: number;
  level: number; // 0.0 - 1.0
  timestamp: number;
}

export interface SpectrogramData {
  frequencies: number[]; // Hz
  magnitudes: number[][]; // [time][freq] in dB
  sampleRate: number;
  duration: number; // seconds
}

export interface BLEDevice {
  id: string;
  name: string;
  rssi: number;
  connected: boolean;
}

export interface AlertData {
  hiveId: string;
  hiveName: string;
  alertType: 'swarm' | 'queenless' | 'varroa' | 'dead';
  severity: 'info' | 'warning' | 'critical';
  message: string;
  recommendedAction: string;
  timestamp: number;
  confidence: number;
}