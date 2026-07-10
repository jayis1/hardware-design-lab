/*
 * ble.ts — BLE communication layer for the Occlusograph companion app.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Wraps react-native-ble-plx to connect to the Occlusograph device,
 * subscribe to GATT notifications, parse force-stream and event payloads,
 * and send calibration commands. Exposes a clean TypeScript API to the UI.
 */

import BleManager, { Peripheral, Service, Characteristic } from 'react-native-ble-plx';

// ---- GATT UUIDs (must match firmware/board.h) ----
export const OCCLUSOGRAPH_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
export const OCCLUSAL_SERVICE_UUID = '9a010001-b5a3-f393-e0a9-e50e24dcca9e';
export const FORCE_STREAM_UUID = '9a020001-b5a3-f393-e0a9-e50e24dcca9e';
export const FORCE_CONFIG_UUID = '9a030001-b5a3-f393-e0a9-e50e24dcca9e';
export const CALIBRATION_UUID = '9a040001-b5a3-f393-e0a9-e50e24dcca9e';
export const EVENT_LOG_SERVICE_UUID = '9a050001-b5a3-f393-e0a9-e50e24dcca9e';
export const EVENTS_UUID = '9a060001-b5a3-f393-e0a9-e50e24dcca9e';
export const EVENT_RANGE_UUID = '9a070001-b5a3-f393-e0a9-e50e24dcca9e';
export const SYNC_STATUS_UUID = '9a080001-b5a3-f393-e0a9-e50e24dcca9e';
export const DEVICE_INFO_UUID = '0000180a-0000-1000-8000-00805f9b34fb';
export const BATTERY_UUID = '00002a19-0000-1000-8000-00805f9b34fb';

// ---- Types ----
export interface ForceFrame {
  forces_mN: number[];    // 64 elements
  timestamp: number;
}

export interface EventRecord {
  timestamp: number;
  duration_ms: number;
  class_id: number;
  class_name: string;
  peak_element: number;
  peak_force_mN: number;
  rms_force_mN: number;
  jaw_open_deg: number;
}

export interface DeviceInfo {
  model: string;
  serial: string;
  firmware: string;
  battery_pct: number;
  charging: boolean;
}

export type ConnectionState = 'disconnected' | 'scanning' | 'connecting' | 'connected';

// ---- Activity class names (must match firmware/classify.h) ----
const CLASS_NAMES = [
  'Rest', 'Light Contact', 'Chewing', 'Clenching',
  'Bruxism (Phasic)', 'Bruxism (Tonic)', 'Swallowing',
];

// ---- Singleton BLE manager ----
const bleManager = new BleManager();

class OcclusographBLE {
  private device: any = null;
  private connectionState: ConnectionState = 'disconnected';
  private forceCallback: ((frame: ForceFrame) => void) | null = null;
  private eventCallback: ((evt: EventRecord) => void) | null = null;
  private stateCallback: ((state: ConnectionState) => void) | null = null;
  private batteryCallback: ((pct: number) => void) | null = null;

  // ---- Connection state ----
  setConnectionStateCallback(cb: (state: ConnectionState) => void): void {
    this.stateCallback = cb;
  }

  private setState(state: ConnectionState): void {
    this.connectionState = state;
    if (this.stateCallback) this.stateCallback(state);
  }

  getConnectionState(): ConnectionState {
    return this.connectionState;
  }

  // ---- Scanning ----
  async scan(timeoutMs: number = 5000): Promise<any[]> {
    this.setState('scanning');
    const devices: any[] = [];
    return new Promise((resolve) => {
      bleManager.startDeviceScan([OCCLUSAL_SERVICE_UUID], null, (error, device) => {
        if (error) { console.warn('BLE scan error:', error); return; }
        if (device && !devices.find(d => d.id === device.id)) {
          devices.push(device);
        }
      });
      setTimeout(() => {
        bleManager.stopDeviceScan();
        this.setState('disconnected');
        resolve(devices);
      }, timeoutMs);
    });
  }

  // ---- Connect ----
  async connect(deviceId: string): Promise<void> {
    this.setState('connecting');
    try {
      this.device = await bleManager.connectToDevice(deviceId);
      await this.device.discoverAllServicesAndCharacteristics();
      this.setState('connected');

      // Subscribe to battery level.
      await this.device.readCharacteristicForService(DEVICE_INFO_UUID, BATTERY_UUID)
        .then((c: any) => {
          if (c && c.value && this.batteryCallback) {
            this.batteryCallback(parseInt(c.value, 16));
          }
        })
        .catch(() => {});

      // Set up disconnect listener.
      this.device.onDisconnected(() => { this.setState('disconnected'); });
    } catch (e) {
      this.setState('disconnected');
      throw e;
    }
  }

  async disconnect(): Promise<void> {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
    }
    this.setState('disconnected');
  }

  // ---- Force stream subscription ----
  async subscribeForce(cb: (frame: ForceFrame) => void): Promise<void> {
    this.forceCallback = cb;
    if (!this.device) return;
    await this.device.monitorCharacteristicForService(
      OCCLUSAL_SERVICE_UUID, FORCE_STREAM_UUID,
      (error: any, characteristic: any) => {
        if (error || !characteristic || !characteristic.value) return;
        const frame = this.parseForceFrame(characteristic.value);
        if (this.forceCallback) this.forceCallback(frame);
      }
    );
  }

  private parseForceFrame(base64Value: string): ForceFrame {
    // Decode base64 to bytes.
    const raw = this.base64ToBytes(base64Value);
    const forces: number[] = new Array(64);
    for (let i = 0; i < 64; i++) {
      // Big-endian int16.
      const hi = raw[2 * i];
      const lo = raw[2 * i + 1];
      let val = (hi << 8) | lo;
      if (val > 32767) val -= 65536;  // sign-extend
      forces[i] = val;
    }
    const timestamp = (raw[128] << 24) | (raw[129] << 16) | (raw[130] << 8) | raw[131];
    return { forces_mN: forces, timestamp };
  }

  // ---- Event log subscription ----
  async subscribeEvents(cb: (evt: EventRecord) => void): Promise<void> {
    this.eventCallback = cb;
    if (!this.device) return;
    await this.device.monitorCharacteristicForService(
      EVENT_LOG_SERVICE_UUID, EVENTS_UUID,
      (error: any, characteristic: any) => {
        if (error || !characteristic || !characteristic.value) return;
        const events = this.parseEvents(characteristic.value);
        for (const evt of events) {
          if (this.eventCallback) this.eventCallback(evt);
        }
      }
    );
  }

  private parseEvents(base64Value: string): EventRecord[] {
    const raw = this.base64ToBytes(base64Value);
    // Each event record is 14 bytes (from firmware event_record_t).
    const RECORD_SIZE = 14;
    const count = Math.floor(raw.length / RECORD_SIZE);
    const events: EventRecord[] = [];
    for (let i = 0; i < count; i++) {
      const off = i * RECORD_SIZE;
      const timestamp = (raw[off] << 24) | (raw[off+1] << 16) | (raw[off+2] << 8) | raw[off+3];
      const duration_ms = (raw[off+4] << 8) | raw[off+5];
      const class_id = raw[off+6];
      const peak_element = raw[off+7];
      const peak_force_mN = ((raw[off+8] << 8) | raw[off+9]) << 16 >> 16;
      const rms_force_mN = ((raw[off+10] << 8) | raw[off+11]) << 16 >> 16;
      const jaw_open_deg = ((raw[off+12] << 8) | raw[off+13]) << 16 >> 16;
      events.push({
        timestamp, duration_ms, class_id,
        class_name: CLASS_NAMES[class_id] || 'Unknown',
        peak_element, peak_force_mN, rms_force_mN, jaw_open_deg,
      });
    }
    return events;
  }

  // ---- Calibration ----
  async sendCalibration(element: number, offset_mN: number): Promise<void> {
    if (!this.device) return;
    const data = new Uint8Array(3);
    data[0] = element & 0xFF;
    data[1] = (offset_mN >> 8) & 0xFF;
    data[2] = offset_mN & 0xFF;
    await this.device.writeCharacteristicWithResponseForService(
      OCCLUSAL_SERVICE_UUID, CALIBRATION_UUID,
      this.bytesToBase64(data)
    );
  }

  // ---- Battery ----
  setBatteryCallback(cb: (pct: number) => void): void {
    this.batteryCallback = cb;
  }

  async readBattery(): Promise<number> {
    if (!this.device) return 0;
    const c = await this.device.readCharacteristicForService(DEVICE_INFO_UUID, BATTERY_UUID);
    if (c && c.value) return parseInt(c.value, 16);
    return 0;
  }

  // ---- Helpers ----
  private base64ToBytes(b64: string): Uint8Array {
    const chars = atob(b64);
    const bytes = new Uint8Array(chars.length);
    for (let i = 0; i < chars.length; i++) bytes[i] = chars.charCodeAt(i);
    return bytes;
  }

  private bytesToBase64(bytes: Uint8Array): string {
    let str = '';
    for (let i = 0; i < bytes.length; i++) str += String.fromCharCode(bytes[i]);
    return btoa(str);
  }
}

export const ble = new OcclusographBLE();
export default ble;