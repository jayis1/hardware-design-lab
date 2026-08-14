/*
 * types.ts — shared MusselWatch data types
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. MIT License.
 */

export interface ChannelState {
  channel: number;
  rawHall: number;
  rawBaseline: number;
  gapeUm: number;
  activityScore: number;
  anomalyFlag: number;      // bit0 = clamp, bit1 = stall
  anomalyScore: number;     // 0..100
  lastEventS: number;
  species: string;          // e.g. "Unio pictorum"
  location: string;          // reach / cage label
}

export interface Telemetry {
  nodeId: string;
  seq: number;
  batteryMv: number;
  solarMv: number;
  waterTempC10: number;     // 0.1 °C
  chargerState: number;     // 0=idle 1=charging 2=full 3=fault
  flags: number;
  activeChannels: number;   // bitmask
  maxAnomaly: number;
  uptimeS: number;
  receivedAt: number;       // epoch ms
}

export type AlertLevel = 'nominal' | 'advisory' | 'warning' | 'critical';

export interface AlertEvent {
  id: string;
  nodeId: string;
  channel: number;
  level: AlertLevel;
  anomalyFlag: number;
  gapeUm: number;
  activityScore: number;
  waterTempC10: number;
  timestamp: number;
  acknowledged: boolean;
  note: string;
}

export interface Node {
  nodeId: string;
  label: string;
  river: string;
  reach: string;
  lastSeenS: number;
  channels: ChannelState[];
  telemetry: Telemetry | null;
  species: string;
}

export interface AppConfig {
  gatewayUrl: string;
  pollIntervalS: number;
  alertThreshold: number;   // anomaly score 0..100 to trigger push alert
  temperatureUnitC: boolean;
  darkMode: boolean;
}

export const ANOMALY_CLAMP = 0x01;
export const ANOMALY_STALL = 0x02;

export function anomalyLabel(flag: number): string {
  if (flag === 0) return 'Nominal';
  const parts: string[] = [];
  if (flag & ANOMALY_CLAMP) parts.push('Shell clamp');
  if (flag & ANOMALY_STALL) parts.push('Gape stall');
  return parts.join(' + ');
}

export function alertLevelFromScore(score: number): AlertLevel {
  if (score >= 80) return 'critical';
  if (score >= 50) return 'warning';
  if (score >= 20) return 'advisory';
  return 'nominal';
}

export function chargerStateLabel(state: number): string {
  switch (state) {
    case 0: return 'Idle';
    case 1: return 'Charging';
    case 2: return 'Full';
    case 3: return 'Fault';
    default: return 'Unknown';
  }
}

export function formatTemp(c10: number, useC: boolean): string {
  const c = c10 / 10;
  if (useC) return `${c.toFixed(1)} °C`;
  return `${(c * 9 / 5 + 32).toFixed(1)} °F`;
}

export function formatUptime(s: number): string {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  if (d > 0) return `${d}d ${h}h ${m}m`;
  if (h > 0) return `${h}h ${m}m`;
  return `${m}m`;
}

export function batteryPct(mv: number): number {
  const lo = 3200, hi = 4200;
  const p = ((mv - lo) / (hi - lo)) * 100;
  return Math.max(0, Math.min(100, Math.round(p)));
}