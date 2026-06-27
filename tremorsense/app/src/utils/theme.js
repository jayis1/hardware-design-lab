/*
 * theme.js — Color definitions for TremorSense Connect
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

export const COLORS = {
  primary: '#0A84FF',
  background: '#1C1C1E',
  surface: '#2C2C2E',
  text: '#FFFFFF',
  textSecondary: '#8E8E93',
  parkinsonian: '#FF3B30',
  essential: '#FF9500',
  cerebellar: '#AF52DE',
  physiological: '#34C759',
  drug: '#FFCC00',
  success: '#34C759',
  warning: '#FF9500',
  danger: '#FF3B30',
};

export const TREMOR_CLASSES = [
  { id: 0, name: 'Parkinsonian', color: '#FF3B30', short: 'PD' },
  { id: 1, name: 'Essential', color: '#FF9500', short: 'ET' },
  { id: 2, name: 'Cerebellar', color: '#AF52DE', short: 'Cer' },
  { id: 3, name: 'Physiological', color: '#34C759', short: 'Phys' },
  { id: 4, name: 'Drug-Induced', color: '#FFCC00', short: 'Drug' },
];

export const CONTEXTS = [
  { id: 0, name: 'Rest', color: '#8E8E93' },
  { id: 1, name: 'Postural', color: '#0A84FF' },
  { id: 2, name: 'Action', color: '#FF9500' },
];

export const DARK_THEME = {
  background: '#1C1C1E',
  surface: '#2C2C2E',
  card: '#2C2C2E',
  primary: '#0A84FF',
  text: '#FFFFFF',
  textSecondary: '#8E8E93',
  separator: '#3A3A3C',
};