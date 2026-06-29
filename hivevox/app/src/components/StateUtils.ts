/*
 * components/StateUtils.ts — Colony state helpers and colors
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
import type { HiveData } from '../App';

export const STATE_NAMES: Record<number, string> = {
  0: 'Queenright',
  1: 'Queenless',
  2: 'Swarm Prep',
  3: 'Winter Cluster',
  4: 'Dead/Empty',
  5: 'Unknown',
  6: 'Baseline',
};

export const STATE_COLORS: Record<number, string> = {
  0: '#4caf50',   // green
  1: '#f44336',   // red
  2: '#ff9800',   // orange
  3: '#2196f3',   // blue
  4: '#9e9e9e',   // grey
  5: '#ffc107',   // amber
  6: '#9c27b0',   // purple
};

export const STATE_ICONS: Record<number, string> = {
  0: 'checkmark-circle',
  1: 'alert-circle',
  2: 'git-branch',
  3: 'snow',
  4: 'close-circle',
  5: 'help-circle',
  6: 'pulse',
};

export function stateName(s: number): string {
  return STATE_NAMES[s] || 'Unknown';
}

export function stateColor(s: number): string {
  return STATE_COLORS[s] || '#888';
}

export function formatWeight(g: number): string {
  if (g >= 1000) return (g / 1000).toFixed(1) + ' kg';
  return g + ' g';
}

export function formatTemp(c: number): string {
  return c.toFixed(1) + ' °C';
}

export function formatRelativeTime(epoch: number): string {
  const now = Math.floor(Date.now() / 1000);
  const diff = now - epoch;
  if (diff < 60) return 'just now';
  if (diff < 3600) return Math.floor(diff / 60) + ' min ago';
  if (diff < 86400) return Math.floor(diff / 3600) + ' h ago';
  return Math.floor(diff / 86400) + ' d ago';
}

export function formatBattery(mv: number): string {
  const pct = Math.max(0, Math.min(100,
    ((mv - 2400) / (3600 - 2400)) * 100));
  return Math.round(pct) + '%';
}

export function formatHz(hz: number): string {
  return hz + ' Hz';
}

export function hivesNeedingAttention(hives: HiveData[]): HiveData[] {
  return hives.filter((h) => h.state === 1 || h.state === 2 || h.state === 4);
}

export function totalHoneyWeight(hives: HiveData[]): number {
  // Subtract estimated hive weight (empty: ~15 kg) to approximate honey weight
  const EMPTY_HIVE_G = 15000;
  return hives.reduce((sum, h) => sum + Math.max(0, h.weightG - EMPTY_HIVE_G), 0);
}

export function averageBroodTemp(hives: HiveData[]): number {
  if (hives.length === 0) return 0;
  return hives.reduce((sum, h) => sum + h.broodTempC, 0) / hives.length;
}