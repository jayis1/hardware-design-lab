/*
 * src/types.ts — Shared TypeScript types for the Cryo-Sentinel app.
 *
 * Author: jayis1
 * License: MIT
 */
export type RootStackParamList = {
  Dashboard: undefined;
  DewarDetail: { dewarId: string };
  Commissioning: undefined;
  EscalationChain: { dewarId: string };
  AuditLog: { dewarId: string };
  AlertInbox: undefined;
};

export type AlarmState = 'OK' | 'WATCH' | 'WARN' | 'CRITICAL';

export interface AlarmReason {
  code: string;
  label: string;
  severity: AlarmState;
}

export interface DewarReading {
  dewarId: string;
  label: string;
  state: AlarmState;
  reason: string;
  levelPct: number;
  levelRatePerHour: number;
  rtdTopC: number;
  rtdMidC: number;
  rtdBotC: number;
  gradientSlope: number;
  tiltDeg: number;
  vibRmsG: number;
  ambientC: number;
  ambientRh: number;
  battPct: number;
  mainsPresent: boolean;
  lidOpen: boolean;
  enclosureOpen: boolean;
  lastHeartbeatEpoch: number;
  serial: string;
}

export interface AuditRecord {
  seq: number;
  epochMin: number;
  event: string;
  aux: number;
}

export interface EscalationEntry {
  technicianId: string;
  name: string;
  order: number;
  push: boolean;
  sms: boolean;
  email: boolean;
  call: boolean;
  delayMin: number;
}

export interface CalibrationPoint {
  index: number;
  pct: number;
  raw: number;
}

export interface CalibrationState {
  dewarId: string;
  label: string;
  points: CalibrationPoint[];
  currentStep: number;
}

export const STATE_COLOR: Record<AlarmState, string> = {
  OK: '#16A34A',
  WATCH: '#EAB308',
  WARN: '#EA580C',
  CRITICAL: '#DC2626',
};

export const STATE_ICON: Record<AlarmState, string> = {
  OK: 'check-circle',
  WATCH: 'eye',
  WARN: 'alert',
  CRITICAL: 'alert-octagon',
};