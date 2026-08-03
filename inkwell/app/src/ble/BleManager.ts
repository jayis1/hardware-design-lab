// BleManager.ts — BLE connection manager for the Inkwell smart pen
//
// Scans for the Inkwell GATT service, connects, subscribes to stroke-data
// notifications, parses the 20-byte segment payload into a StrokeSegment,
// and exposes a subscribe() API that the UI screens consume. Also handles
// the Journal Replay characteristic so missed segments can be pulled after
// a BLE dropout.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import { BleError, BleManager, Device, Subscription } from 'react-native-ble-plx';
import { StrokeSegment, UUIDS, parseStrokeSegment, serializeReplayRequest } from './protocol';

const SERVICE_UUID        = UUIDS.INKWELL_SERVICE;
const STROKE_CHAR_UUID    = UUIDS.STROKE_DATA;
const STATUS_CHAR_UUID    = UUIDS.STATUS;
const CONTROL_CHAR_UUID   = UUIDS.CONTROL;
const REPLAY_CHAR_UUID    = UUIDS.JOURNAL_REPLAY;

export type PenStatus = {
  batteryPct: number;
  powerState: number;   // 0=off,1=adv,2=connIdle,3=writing,4=charging
  flashFillPct: number;
};

export type StrokeCallback = (seg: StrokeSegment) => void;
export type StatusCallback = (status: PenStatus) => void;
export type ConnectionCallback = (connected: boolean, device?: Device) => void;

class InkwellBleManager {
  private ble: BleManager;
  private device: Device | null = null;
  private strokeSubs: Subscription | null = null;
  private statusSubs: Subscription | null = null;
  private strokeCallbacks: Set<StrokeCallback> = new Set();
  private statusCallbacks: Set<StatusCallback> = new Set();
  private connCallbacks: Set<ConnectionCallback> = new Set();
  private lastSeq: number = 0;

  constructor() {
    this.ble = new BleManager();
  }

  /** Begin scanning and connect to the first Inkwell pen found. */
  async connect(): Promise<void> {
    this.ble.startDeviceScan([SERVICE_UUID], null, (error, device) => {
      if (error) { console.warn('[Inkwell] scan error', error); return; }
      if (device && device.name === 'Inkwell') {
        this.ble.stopDeviceScan();
        this.establishConnection(device);
      }
    });
  }

  private async establishConnection(device: Device): Promise<void> {
    try {
      const connected = await device.connect();
      await connected.discoverAllServicesAndCharacteristics();
      this.device = connected;
      this.notifyConnection(true, connected);

      // Subscribe to stroke data notifications
      this.strokeSubs = connected.monitorCharacteristicForService(
        SERVICE_UUID, STROKE_CHAR_UUID,
        (err, char) => {
          if (err || !char || !char.value) return;
          const seg = parseStrokeSegment(char.value);
          this.lastSeq = seg.seq;
          this.strokeCallbacks.forEach(cb => cb(seg));
        });

      // Subscribe to status notifications
      this.statusSubs = connected.monitorCharacteristicForService(
        SERVICE_UUID, STATUS_CHAR_UUID,
        (err, char) => {
          if (err || !char || !char.value) return;
          const raw = Buffer.from(char.value, 'base64');
          const status: PenStatus = {
            batteryPct: raw.readUInt8(0),
            powerState: raw.readUInt8(1),
            flashFillPct: raw.readUInt8(2),
          };
          this.statusCallbacks.forEach(cb => cb(status));
        });
    } catch (e) {
      console.warn('[Inkwell] connect failed', e);
    }
  }

  async disconnect(): Promise<void> {
    if (this.strokeSubs) this.strokeSubs.remove();
    if (this.statusSubs) this.statusSubs.remove();
    if (this.device) await this.device.cancelConnection();
    this.device = null;
    this.notifyConnection(false);
  }

  /** Send a control command to start/stop a session. */
  async sendControl(cmd: number): Promise<void> {
    if (!this.device) return;
    const data = String.fromCharCode(cmd);
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, CONTROL_CHAR_UUID, btoa(data));
  }

  /** Request the pen to replay missing segments from its flash journal. */
  async requestReplay(seqStart: number, seqEnd: number): Promise<void> {
    if (!this.device) return;
    const payload = serializeReplayRequest(seqStart, seqEnd);
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, REPLAY_CHAR_UUID, payload);
    // Replay records arrive on the same Stroke Data characteristic.
  }

  onStroke(cb: StrokeCallback): () => void {
    this.strokeCallbacks.add(cb);
    return () => { this.strokeCallbacks.delete(cb); };
  }

  onStatus(cb: StatusCallback): () => void {
    this.statusCallbacks.add(cb);
    return () => { this.statusCallbacks.delete(cb); };
  }

  onConnection(cb: ConnectionCallback): () => void {
    this.connCallbacks.add(cb);
    return () => { this.connCallbacks.delete(cb); };
  }

  private notifyConnection(connected: boolean, device?: Device): void {
    this.connCallbacks.forEach(cb => cb(connected, device));
  }

  isConnected(): boolean { return this.device !== null; }
  getLastSeq(): number { return this.lastSeq; }
}

export const bleManager = new InkwellBleManager();
export default bleManager;