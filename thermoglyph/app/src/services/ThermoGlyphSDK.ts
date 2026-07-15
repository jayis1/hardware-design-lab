/**
 * ThermoGlyph BLE SDK — communication layer with the hardware device
 *
 * Implements the GATT service protocol for:
 *   - Sending glyph commands to the device
 *   - Receiving gesture notifications
 *   - Receiving telemetry
 *   - Configuration writes
 *   - OTA DFU control
 *
 * Uses react-native-ble-plx for BLE communication.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import type { GlyphCommand, Telemetry, DeviceConfig, GestureType } from '../types';

// GATT UUIDs (must match firmware board.h)
const SERVICE_UUID = '0000TG01-0000-1000-8000-00805F9B34FB';
const CHAR_GLYPH    = '0000TG01-0001-1000-8000-00805F9B34FB';
const CHAR_GESTURE  = '0000TG02-0001-1000-8000-00805F9B34FB';
const CHAR_TELEMETRY= '0000TG03-0001-1000-8000-00805F9B34FB';
const CHAR_CONFIG   = '0000TG04-0001-1000-8000-00805F9B34FB';
const CHAR_DFU      = '0000TG05-0001-1000-8000-00805F9B34FB';

// Protocol constants (must match firmware registers.h)
const GLYPH_CMD_PIXEL = 0x01;
const GLYPH_CMD_ARROW = 0x02;
const GLYPH_CMD_TEXT  = 0x03;
const GLYPH_CMD_SHAPE = 0x04;
const GLYPH_CMD_ANIM  = 0x05;
const GLYPH_CMD_BAR   = 0x06;
const GLYPH_CMD_CLEAR = 0x07;

const GLYPH_WARM    = 0x01;
const GLYPH_COOL    = 0x02;
const GLYPH_NEUTRAL = 0x00;

const DIRECTION_MAP: Record<string, number> = {
  north: 0, northeast: 1, east: 2, southeast: 3,
  south: 4, southwest: 5, west: 6, northwest: 7,
};

const SHAPE_MAP: Record<string, number> = {
  circle: 0, ring: 1, cross: 2, wave: 3, check: 4, x: 5,
};

const GESTURE_NAMES: Record<number, GestureType> = {
  0: 'none', 1: 'tap', 2: 'double_tap', 3: 'flip', 4: 'shake',
};

/**
 * Encode a GlyphCommand into the binary packet format expected by the firmware.
 * Packet: [cmd] [polarity] [intensity] [direction/shape] [dur_lo] [dur_hi]
 *         [text_len] [text[0..15]] [scroll]
 */
function encodeGlyph(cmd: GlyphCommand): Buffer {
  const buf = Buffer.alloc(24, 0);
  let offset = 0;

  // Command type
  switch (cmd.type) {
    case 'pixel':  buf[offset] = GLYPH_CMD_PIXEL; break;
    case 'arrow':  buf[offset] = GLYPH_CMD_ARROW; break;
    case 'text':   buf[offset] = GLYPH_CMD_TEXT; break;
    case 'shape':  buf[offset] = GLYPH_CMD_SHAPE; break;
    case 'anim':   buf[offset] = GLYPH_CMD_ANIM; break;
    case 'bar':    buf[offset] = GLYPH_CMD_BAR; break;
    case 'clear':  buf[offset] = GLYPH_CMD_CLEAR; break;
  }
  offset++;

  // Polarity
  buf[offset++] = cmd.polarity === 'warm' ? GLYPH_WARM :
                   cmd.polarity === 'cool' ? GLYPH_COOL : GLYPH_NEUTRAL;

  // Intensity
  buf[offset++] = Math.min(255, Math.max(0, cmd.intensity));

  // Direction/shape (same byte, context-dependent)
  if (cmd.direction) {
    buf[offset++] = DIRECTION_MAP[cmd.direction] ?? 0;
  } else if (cmd.shape) {
    buf[offset++] = SHAPE_MAP[cmd.shape] ?? 0;
  } else {
    buf[offset++] = 0;
  }

  // Duration (16-bit little-endian)
  const dur = Math.min(65535, Math.max(0, cmd.durationMs));
  buf[offset++] = dur & 0xFF;
  buf[offset++] = (dur >> 8) & 0xFF;

  // Text
  const text = cmd.text ?? '';
  buf[offset++] = Math.min(15, text.length);
  for (let i = 0; i < Math.min(15, text.length); i++) {
    buf[offset++] = text.charCodeAt(i);
  }
  offset = 23; // skip to scroll byte

  // Scroll flag
  buf[offset] = cmd.scroll ? 1 : 0;

  return buf;
}

/**
 * Decode a telemetry notification packet (10 bytes) from the firmware.
 */
function decodeTelemetry(data: Buffer): Telemetry {
  return {
    batteryPct: data[0],
    solarMv: data[1] | (data[2] << 8),
    ambientTempC: data[3],
    skinTempAvgC: data[4],
    skinTempMaxC: data[5],
    currentGlyphCmd: data[6],
    gestureLast: data[7],
    loraRssi: data[8] - 128, // offset decode
    state: ['active', 'idle', 'sleep', 'solar_sustain', 'fault'][data[9]] as any,
  };
}

/**
 * Encode a configuration packet.
 */
function encodeConfig(type: number, data: number[]): Buffer {
  const buf = Buffer.alloc(1 + data.length, 0);
  buf[0] = type;
  for (let i = 0; i < data.length; i++) {
    buf[1 + i] = data[i] & 0xFF;
  }
  return buf;
}

/**
 * Main ThermoGlyph BLE SDK class.
 * In production, this wraps react-native-ble-plx. The interface is designed
 * to be drop-in compatible with the actual BLE library.
 */
export class ThermoGlyphSDK {
  private device: any = null;
  private connected: boolean = false;
  private gestureCallbacks: ((g: GestureType) => void)[] = [];
  private telemetryCallbacks: ((t: Telemetry) => void)[] = [];

  // BLE library reference (injected in production)
  private bleManager: any = null;

  constructor(bleManager?: any) {
    if (bleManager) this.bleManager = bleManager;
  }

  /**
   * Scan for and connect to a ThermoGlyph device.
   */
  async connect(): Promise<void> {
    if (!this.bleManager) {
      // Simulation mode (for development/testing)
      this.connected = true;
      this.device = { id: 'simulated-thermoglyph', name: 'ThermoGlyph' };
      return;
    }

    // Real BLE scanning
    const device = await this.bleManager.scanForDevice(SERVICE_UUID, 5000);
    if (!device) throw new Error('No ThermoGlyph device found');
    await device.connect();
    await device.discoverAllServicesAndCharacteristics();
    this.device = device;
    this.connected = true;

    // Subscribe to gesture notifications
    await device.setupNotification(CHAR_GESTURE).then((sub: any) => {
      sub.on('data', (data: Buffer) => {
        const gesture = GESTURE_NAMES[data[0]] ?? 'none';
        this.gestureCallbacks.forEach(cb => cb(gesture));
      });
    });

    // Subscribe to telemetry notifications
    await device.setupNotification(CHAR_TELEMETRY).then((sub: any) => {
      sub.on('data', (data: Buffer) => {
        const tel = decodeTelemetry(data);
        this.telemetryCallbacks.forEach(cb => cb(tel));
      });
    });
  }

  /**
   * Disconnect from the device.
   */
  async disconnect(): Promise<void> {
    if (this.device && this.bleManager) {
      await this.device.cancelConnection();
    }
    this.connected = false;
    this.device = null;
  }

  /**
   * Render a glyph on the device.
   */
  async renderGlyph(cmd: GlyphCommand): Promise<void> {
    if (!this.connected) throw new Error('Device not connected');
    const data = encodeGlyph(cmd);
    if (this.bleManager && this.device) {
      await this.device.writeCharacteristicWithResponse(CHAR_GLYPH, data);
    }
    // Simulation: no-op
  }

  /**
   * Clear the current glyph.
   */
  async clearGlyph(): Promise<void> {
    await this.renderGlyph({ type: 'clear', polarity: 'neutral', intensity: 0, durationMs: 0 });
  }

  /**
   * Render an arrow for navigation.
   */
  async renderArrow(direction: GlyphCommand['direction'], intensity = 128, durationMs = 2000): Promise<void> {
    await this.renderGlyph({
      type: 'arrow',
      direction,
      polarity: 'warm',
      intensity,
      durationMs,
    });
  }

  /**
   * Render text (scrolled across the thermal array).
   */
  async renderText(text: string, scroll = true, durationMs = 5000): Promise<void> {
    await this.renderGlyph({
      type: 'text',
      text: text.toUpperCase(),
      scroll,
      polarity: 'warm',
      intensity: 128,
      durationMs,
    });
  }

  /**
   * Render a shape.
   */
  async renderShape(shape: GlyphCommand['shape'], polarity: GlyphCommand['polarity'] = 'warm',
                     intensity = 128, durationMs = 3000): Promise<void> {
    await this.renderGlyph({
      type: 'shape',
      shape,
      polarity,
      intensity,
      durationMs,
    });
  }

  /**
   * Render an animated alert (expanding ring).
   */
  async renderAlert(durationMs = 10000): Promise<void> {
    await this.renderGlyph({
      type: 'anim',
      shape: 'ring',
      polarity: 'warm',
      intensity: 255,
      durationMs,
    });
  }

  /**
   * Render a proportional bar (for depth/progress).
   */
  async renderBar(intensity: number, durationMs = 2000): Promise<void> {
    await this.renderGlyph({
      type: 'bar',
      polarity: 'warm',
      intensity: Math.min(255, Math.max(0, intensity)),
      durationMs,
    });
  }

  /**
   * Send a glyph to a buddy device via LoRa.
   */
  async sendToBuddy(buddyId: string, cmd: GlyphCommand): Promise<void> {
    // In production, this sends a LoRa relay command via BLE
    if (!this.connected) throw new Error('Device not connected');
    const glyphData = encodeGlyph(cmd);
    const payload = Buffer.alloc(1 + glyphData.length, 0);
    payload[0] = parseInt(buddyId, 10) & 0xFF;
    glyphData.copy(payload, 1);
    if (this.bleManager && this.device) {
      // Send via a special LoRa relay characteristic
      await this.device.writeCharacteristicWithResponse(CHAR_GLYPH, payload);
    }
  }

  /**
   * Send SOS via LoRa.
   */
  async sendSOS(lat: number, lon: number): Promise<void> {
    if (!this.connected) throw new Error('Device not connected');
    const payload = Buffer.alloc(5, 0);
    payload[0] = 1; // my device ID
    payload.writeInt16BE(Math.round(lat * 100), 1);
    payload.writeInt16BE(Math.round(lon * 100), 3);
    if (this.bleManager && this.device) {
      await this.device.writeCharacteristicWithResponse(CHAR_GLYPH, payload);
    }
  }

  /**
   * Update device configuration.
   */
  async updateConfig(config: Partial<DeviceConfig>): Promise<void> {
    if (!this.connected) throw new Error('Device not connected');
    if (config.intensityScale !== undefined) {
      const data = encodeConfig(0x01, [config.intensityScale]);
      if (this.bleManager && this.device) {
        await this.device.writeCharacteristicWithResponse(CHAR_CONFIG, data);
      }
    }
    if (config.pidKp !== undefined && config.pidKi !== undefined && config.pidKd !== undefined) {
      const data = encodeConfig(0x02, [config.pidKp, config.pidKi, config.pidKd]);
      if (this.bleManager && this.device) {
        await this.device.writeCharacteristicWithResponse(CHAR_CONFIG, data);
      }
    }
    if (config.maxTempC !== undefined) {
      const data = encodeConfig(0x03, [config.maxTempC & 0xFF, (config.maxTempC >> 8) & 0xFF]);
      if (this.bleManager && this.device) {
        await this.device.writeCharacteristicWithResponse(CHAR_CONFIG, data);
      }
    }
  }

  /**
   * Register a callback for gesture events.
   */
  onGesture(cb: (g: GestureType) => void): void {
    this.gestureCallbacks.push(cb);
  }

  /**
   * Register a callback for telemetry updates.
   */
  onTelemetry(cb: (t: Telemetry) => void): void {
    this.telemetryCallbacks.push(cb);
  }

  /**
   * Check if device is connected.
   */
  isConnected(): boolean {
    return this.connected;
  }
}

/**
 * Singleton instance for app-wide use.
 */
let _instance: ThermoGlyphSDK | null = null;
export function getThermoGlyphSDK(): ThermoGlyphSDK {
  if (!_instance) {
    _instance = new ThermoGlyphSDK();
  }
  return _instance;
}