/**
 * BleManager.ts — BLE connection manager for the GlyphFlow wristband
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Wraps react-native-ble-plx into a small typed client that:
 *   - scans for and connects to the GlyphFlow band
 *   - subscribes to the HID keyboard characteristic (text stream)
 *   - subscribes to the trajectory characteristic (for the viewer)
 *   - reads battery and status
 *   - writes active-set and training commands
 *
 * The band advertises service 0x1812 (HID) and a custom service
 * 55GF0001-... for the GlyphFlow-specific characteristics.
 */

import { BleManager, Device, Characteristic } from 'react-native-ble-plx';

// Service & characteristic UUIDs (short-form, expanded for clarity).
export const SERVICE_HID        = '00001812-0000-1000-8000-00805f9b34fb';
export const SERVICE_DEVICE_INFO = '0000180a-0000-1000-8000-00805f9b34fb';
export const SERVICE_GLYPHFLOW   = '556c6f70-466c-6f77-2d31-30302d303030';
export const CHAR_TRAJECTORY     = '556c6f70-0001-1000-8000-00805f9b34fb';
export const CHAR_TRAINING_CMD   = '556c6f70-0002-1000-8000-00805f9b34fb';
export const CHAR_OTA_WEIGHTS     = '556c6f70-0003-1000-8000-00805f9b34fb';
export const CHAR_ACTIVE_SET      = '556c6f70-0004-1000-8000-00805f9b34fb';
export const CHAR_BATTERY         = '00002a19-0000-1000-8000-00805f9b34fb';

export type PenState = 'sleep' | 'idle' | 'capture' | 'infer' | 'group';

export interface GlyphBand {
  device: Device;
  batteryPct: number;
  connected: boolean;
  penState: PenState;
  fwVersion: string;
}

type Listener<T> = (value: T) => void;

export class BleClient {
  private ble: BleManager;
  private device: Device | null = null;
  private textListeners: Listener<string>[] = [];
  private trajListeners: Listener<{ x: number; y: number }[]>[] = [];
  private stateListeners: Listener<PenState>[] = [];
  private battListeners: Listener<number>[] = [];
  private cached: GlyphBand | null = null;

  constructor() {
    this.ble = new BleManager();
  }

  /** Request permissions and start scanning for a device advertising the HID service. */
  async scanAndConnect(timeoutMs = 12000): Promise<GlyphBand> {
    await this.ble.enable();
    const found = await new Promise<Device>((resolve, reject) => {
      const t = setTimeout(() => {
        this.ble.stopDeviceScan();
        reject(new Error('GlyphFlow band not found'));
      }, timeoutMs);

      this.ble.startDeviceScan([SERVICE_HID], null, (err, d) => {
        if (err) { clearTimeout(t); reject(err); return; }
        if (d && d.name && d.name.includes('GlyphFlow')) {
          clearTimeout(t);
          this.ble.stopDeviceScan();
          resolve(d);
        }
      });
    });

    await found.connect();
    await found.discoverAllServicesAndCharacteristics();
    this.device = found;

    // Subscribe to battery.
    const battCh = await found.readCharacteristicForService(SERVICE_DEVICE_INFO, CHAR_BATTERY);
    const battPct = battCh.value ? parseInt(battCh.value, 16) : 0;
    found.monitorCharacteristicForService(
      SERVICE_DEVICE_INFO, CHAR_BATTERY, (err, ch) => {
        if (err || !ch?.value) return;
        const pct = parseInt(ch.value, 16);
        this.battListeners.forEach(l => l(pct));
      }
    );

    // Subscribe to the trajectory characteristic for the viewer.
    found.monitorCharacteristicForService(
      SERVICE_GLYPHFLOW, CHAR_TRAJECTORY, (err, ch) => {
        if (err || !ch?.value) return;
        // ch.value is base64; decode to int16 pairs.
        const raw = Buffer.from(ch.value, 'base64');
        const pts: { x: number; y: number }[] = [];
        for (let i = 0; i + 3 < raw.length; i += 4) {
          const x = raw.readInt16LE(i);
          const y = raw.readInt16LE(i + 2);
          pts.push({ x, y });
        }
        this.trajListeners.forEach(l => l(pts));
      }
    );

    this.cached = {
      device: found,
      batteryPct: battPct,
      connected: true,
      penState: 'idle',
      fwVersion: '1.0',
    };
    return this.cached;
  }

  /** Register a listener that fires whenever a recognized character arrives
   * over the HID report characteristic. */
  onCharacter(listener: Listener<string>) {
    this.textListeners.push(listener);
    return () => { this.textListeners = this.textListeners.filter(l => l !== listener); };
  }

  onTrajectory(listener: Listener<{ x: number; y: number }[]>) {
    this.trajListeners.push(listener);
    return () => { this.trajListeners = this.trajListeners.filter(l => l !== listener); };
  }

  onBattery(listener: Listener<number>) {
    this.battListeners.push(listener);
    return () => { this.battListeners = this.battListeners.filter(l => l !== listener); };
  }

  /** Write the active-set bitmask (lowercase|uppercase|digits|punct|gestures). */
  async setActiveSet(mask: number): Promise<void> {
    if (!this.device) throw new Error('Not connected');
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_GLYPHFLOW, CHAR_ACTIVE_SET,
      mask.toString(16).padStart(2, '0')
    );
  }

  /** Send a training command (e.g. "start", "stop", "label:i"). */
  async sendTrainingCommand(cmd: string): Promise<void> {
    if (!this.device) throw new Error('Not connected');
    const b64 = Buffer.from(cmd, 'utf-8').toString('base64');
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_GLYPHFLOW, CHAR_TRAINING_CMD, b64
    );
  }

  /** Push a new int8 weight blob (base64) to the band over OTA. */
  async pushWeights(int8Weights: Int8Array): Promise<void> {
    if (!this.device) throw new Error('Not connected');
    const buf = Buffer.from(int8Weights);
    const b64 = buf.toString('base64');
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_GLYPHFLOW, CHAR_OTA_WEIGHTS, b64
    );
  }

  async findMyBand(): Promise<void> {
    if (!this.device) return;
    // Toggle the training-command "vibrate" to help locate the band.
    await this.sendTrainingCommand('vibrate');
  }

  async disconnect(): Promise<void> {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.cached = null;
    }
  }

  isConnected(): boolean { return !!this.device && this.cached?.connected === true; }
}

/** A shared singleton instance. */
export const bleClient = new BleClient();