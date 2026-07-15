/**
 * Device store — Zustand state management for ThermoGlyph app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import { create } from 'zustand';
import type {
  Telemetry, Buddy, ThermalFrame, DeviceConfig, GestureType,
  GlyphCommand, GlyphTemplate, AppSettings,
} from '../types';
import { getThermoGlyphSDK } from '../services/ThermoGlyphSDK';

interface DeviceStore {
  // Connection state
  connected: boolean;
  connecting: boolean;

  // Device data
  telemetry: Telemetry | null;
  thermalFrame: ThermalFrame | null;
  lastGesture: GestureType;

  // Configuration
  config: DeviceConfig;
  settings: AppSettings;

  // Glyph templates
  templates: GlyphTemplate[];
  currentGlyph: GlyphCommand | null;
  glyphQueue: GlyphCommand[];

  // Buddies (LoRa)
  buddies: Buddy[];

  // Actions
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendGlyph: (cmd: GlyphCommand) => Promise<void>;
  clearGlyph: () => Promise<void>;
  sendArrow: (dir: GlyphCommand['direction']) => Promise<void>;
  sendSOS: () => Promise<void>;
  updateConfig: (config: Partial<DeviceConfig>) => Promise<void>;
  updateSettings: (settings: Partial<AppSettings>) => void;
  saveTemplate: (template: GlyphTemplate) => void;
  deleteTemplate: (id: string) => void;
  refreshBuddies: () => void;
  _setTelemetry: (t: Telemetry) => void;
  _setGesture: (g: GestureType) => void;
}

const DEFAULT_CONFIG: DeviceConfig = {
  intensityScale: 128,
  pidKp: 120,
  pidKi: 8,
  pidKd: 15,
  maxTempC: 42,
  minTempC: 18,
  powerMode: 'auto',
};

const DEFAULT_SETTINGS: AppSettings = {
  author: 'jayis1',
  autoReconnect: true,
  telemetryIntervalMs: 1000,
  navigationAutoStream: true,
  emergencySosGesture: 'shake',
  darkMode: true,
};

// Generate a simulated thermal frame for preview
function generateSimFrame(): ThermalFrame {
  const cells = [];
  for (let row = 0; row < 8; row++) {
    for (let col = 0; col < 12; col++) {
      cells.push({
        row, col,
        targetTempC: 25,
        measuredTempC: 25 + Math.random() * 2 - 1,
      });
    }
  }
  return { cells, active: false };
}

export const useDeviceStore = create<DeviceStore>((set, get) => ({
  connected: false,
  connecting: false,
  telemetry: null,
  thermalFrame: generateSimFrame(),
  lastGesture: 'none',
  config: DEFAULT_CONFIG,
  settings: DEFAULT_SETTINGS,
  templates: [
    {
      id: 'tpl-1',
      name: 'Turn Left',
      command: { type: 'arrow', direction: 'west', polarity: 'warm', intensity: 160, durationMs: 2000 },
      createdBy: 'jayis1',
    },
    {
      id: 'tpl-2',
      name: 'Arrived',
      command: { type: 'shape', shape: 'check', polarity: 'warm', intensity: 200, durationMs: 3000 },
      createdBy: 'jayis1',
    },
    {
      id: 'tpl-3',
      name: 'Emergency',
      command: { type: 'anim', shape: 'ring', polarity: 'warm', intensity: 255, durationMs: 10000 },
      createdBy: 'jayis1',
    },
  ],
  currentGlyph: null,
  glyphQueue: [],
  buddies: [
    { id: '1', name: 'Buddy Alpha', lastSeen: new Date(), lastRssi: -75, online: true },
    { id: '2', name: 'Buddy Bravo', lastSeen: new Date(Date.now() - 300000), lastRssi: -90, online: false },
  ],

  connect: async () => {
    set({ connecting: true });
    try {
      const sdk = getThermoGlyphSDK();
      await sdk.connect();

      // Register callbacks
      sdk.onGesture((g) => {
        set({ lastGesture: g });
      });

      sdk.onTelemetry((t) => {
        set({ telemetry: t });
        // Update thermal frame based on telemetry
        const frame = get().thermalFrame;
        if (frame) {
          const updatedCells = frame.cells.map(c => ({
            ...c,
            measuredTempC: t.skinTempAvgC + Math.random() * 2 - 1,
          }));
          set({ thermalFrame: { cells: updatedCells, active: t.currentGlyphCmd !== 0 } });
        }
      });

      set({ connected: true, connecting: false });
    } catch (e) {
      set({ connecting: false });
      throw e;
    }
  },

  disconnect: async () => {
    const sdk = getThermoGlyphSDK();
    await sdk.disconnect();
    set({ connected: false, telemetry: null });
  },

  sendGlyph: async (cmd) => {
    const sdk = getThermoGlyphSDK();
    await sdk.renderGlyph(cmd);
    set({ currentGlyph: cmd });
  },

  clearGlyph: async () => {
    const sdk = getThermoGlyphSDK();
    await sdk.clearGlyph();
    set({ currentGlyph: null });
  },

  sendArrow: async (dir) => {
    const sdk = getThermoGlyphSDK();
    await sdk.renderArrow(dir, get().config.intensityScale, 2000);
    set({ currentGlyph: { type: 'arrow', direction: dir, polarity: 'warm', intensity: 128, durationMs: 2000 } });
  },

  sendSOS: async () => {
    const sdk = getThermoGlyphSDK();
    await sdk.sendSOS(0, 0);
    await sdk.renderAlert(10000);
  },

  updateConfig: async (config) => {
    const sdk = getThermoGlyphSDK();
    await sdk.updateConfig(config);
    set((state) => ({ config: { ...state.config, ...config } }));
  },

  updateSettings: (settings) => {
    set((state) => ({ settings: { ...state.settings, ...settings } }));
  },

  saveTemplate: (template) => {
    set((state) => ({
      templates: [...state.templates.filter(t => t.id !== template.id), template],
    }));
  },

  deleteTemplate: (id) => {
    set((state) => ({
      templates: state.templates.filter(t => t.id !== id),
    }));
  },

  refreshBuddies: () => {
    // In production, this would query the LoRa buddy list from the device
    set((state) => ({
      buddies: state.buddies.map(b => ({
        ...b,
        lastRssi: b.online ? -70 - Math.floor(Math.random() * 20) : b.lastRssi,
      })),
    }));
  },

  _setTelemetry: (t) => set({ telemetry: t }),
  _setGesture: (g) => set({ lastGesture: g }),
}));