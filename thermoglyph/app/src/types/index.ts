/**
 * ThermoGlyph companion app — types
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

export type GlyphType = 'pixel' | 'arrow' | 'text' | 'shape' | 'anim' | 'bar' | 'clear';
export type GlyphDirection = 'north' | 'northeast' | 'east' | 'southeast' | 'south' | 'southwest' | 'west' | 'northwest';
export type GlyphPolarity = 'warm' | 'cool' | 'neutral';
export type GlyphShape = 'circle' | 'ring' | 'cross' | 'wave' | 'check' | 'x';

export interface GlyphCommand {
  type: GlyphType;
  direction?: GlyphDirection;
  shape?: GlyphShape;
  polarity: GlyphPolarity;
  intensity: number; // 0–255
  durationMs: number;
  text?: string;
  scroll?: boolean;
}

export interface Telemetry {
  batteryPct: number;
  solarMv: number;
  ambientTempC: number;
  skinTempAvgC: number;
  skinTempMaxC: number;
  currentGlyphCmd: number;
  gestureLast: number;
  loraRssi: number;
  state: PowerState;
}

export type PowerState = 'active' | 'idle' | 'sleep' | 'solar_sustain' | 'fault';

export type GestureType = 'none' | 'tap' | 'double_tap' | 'flip' | 'shake';

export interface Buddy {
  id: string;
  name: string;
  lastSeen: Date;
  lastRssi: number;
  online: boolean;
}

export interface ThermalCell {
  row: number;
  col: number;
  targetTempC: number;
  measuredTempC: number;
}

export interface ThermalFrame {
  cells: ThermalCell[]; // 96 cells (12×8)
  active: boolean;
}

export interface DeviceConfig {
  intensityScale: number; // 0–255
  pidKp: number;
  pidKi: number;
  pidKd: number;
  maxTempC: number;
  minTempC: number;
  powerMode: 'auto' | 'active' | 'idle' | 'sleep';
}

export interface NavigationStep {
  direction: GlyphDirection;
  distanceM: number;
  streetName: string;
  isArrival: boolean;
}

export interface GlyphTemplate {
  id: string;
  name: string;
  command: GlyphCommand;
  createdBy: string;
}

export interface AppSettings {
  author: string;
  autoReconnect: boolean;
  telemetryIntervalMs: number;
  navigationAutoStream: boolean;
  emergencySosGesture: 'shake' | 'double_tap';
  darkMode: boolean;
}