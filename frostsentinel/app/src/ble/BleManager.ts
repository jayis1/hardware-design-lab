// src/ble/BleManager.ts — BLE connection manager for FrostSentinel
//
// Manages the BLE connection to a FrostSentinel node, parses the
// framing protocol, and exposes a simple event-emitter for live data,
// log records, and status updates.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import { BleManager as RNBleManager, Device } from 'react-native-ble-plx';
import { FrostSentinelServiceUUID, FrostSentinelCharUUIDs, parseFrame } from './protocol';

export interface LiveData {
  rfri: number;          // 0.0 - 1.0
  twetC: number;         // °C
  tairC: number;         // °C
  tskyC: number;         // °C
  deltaRadK: number;     // K
  leafWet: number;       // 0-1000
  aeStatus: number;      // 0=idle, 1=armed, 2=nucleation
  flags: number;
  batteryPct: number;
  nodeId: number;
}

export interface StatusResponse {
  nodeId: number;
  meshRole: number;
  meshHops: number;
  sampleInterval: number;
  batteryPct: number;
  batteryMv: number;
  flags: number;
  recordsWritten: number;
}

type Listener = (data: any) => void;

class BleManager {
  private ble: RNBleManager;
  private device: Device | null = null;
  private listeners: Map<string, Listener[]> = new Map();
  private connected: boolean = false;

  constructor() {
    this.ble = new RNBleManager();
  }

  async connect(): Promise<void> {
    const devices = await this.ble.scanForDevices([FrostSentinelServiceUUID], 5000);
    if (devices.length === 0) {
      throw new Error('No FrostSentinel nodes found');
    }
    // Connect to the first found node
    this.device = await this.ble.connectToDevice(devices[0].id);
    await this.device.discoverAllServicesAndCharacteristics();
    this.connected = true;

    // Subscribe to live data notifications
    await this.device.setupNotifications(
      FrostSentinelServiceUUID,
      FrostSentinelCharUUIDs.liveData,
      (data: string) => this.handleNotification(data)
    );

    this.emit('connected', { deviceId: devices[0].id });
  }

  async disconnect(): Promise<void> {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
      this.connected = false;
      this.emit('disconnected', {});
    }
  }

  isConnected(): boolean {
    return this.connected;
  }

  private handleNotification(rawHex: string): void {
    const frame = parseFrame(rawHex);
    if (!frame) return;

    switch (frame.type) {
      case 0x01: // LIVE_DATA
        this.emit('liveData', this.parseLiveData(frame.payload));
        break;
      case 0x02: // LOG_RECORD
        this.emit('logRecord', frame.payload);
        break;
      case 0x08: // STATUS_RSP
        this.emit('status', this.parseStatus(frame.payload));
        break;
      case 0x0F: // ACK
        this.emit('ack', { seq: frame.payload[0] });
        break;
    }
  }

  private parseLiveData(p: Uint8Array): LiveData {
    const dv = new DataView(p.buffer, p.byteOffset, p.byteLength);
    return {
      rfri:       dv.getUint16(0, false) / 256,
      twetC:      dv.getInt16(2, false) / 100,
      tairC:      dv.getInt16(4, false) / 100,
      tskyC:      dv.getInt16(6, false) / 100,
      deltaRadK:  dv.getInt16(8, false) / 100,
      leafWet:    dv.getUint16(10, false),
      aeStatus:   p[12],
      flags:      p[13],
      batteryPct: p[14],
      nodeId:     p[15],
    };
  }

  private parseStatus(p: Uint8Array): StatusResponse {
    const dv = new DataView(p.buffer, p.byteOffset, p.byteLength);
    return {
      nodeId:         p[0],
      meshRole:       p[1],
      meshHops:       p[2],
      sampleInterval: p[3],
      batteryPct:     p[4],
      batteryMv:      dv.getUint16(5, false),
      flags:          p[7],
      recordsWritten: dv.getUint32(8, false),
    };
  }

  async sendCommand(cmd: number, payload: Uint8Array = new Uint8Array(0)): Promise<void> {
    if (!this.device) throw new Error('Not connected');
    const frame = new Uint8Array(1 + payload.length);
    frame[0] = cmd;
    frame.set(payload, 1);
    await this.device.writeCharacteristicWithResponse(
      FrostSentinelServiceUUID,
      FrostSentinelCharUUIDs.command,
      this.bytesToBase64(frame)
    );
  }

  async requestStatus(): Promise<void> {
    await this.sendCommand(0x07);
  }

  async setTime(epoch: number): Promise<void> {
    const p = new Uint8Array(5);
    p[0] = 0x01; // SET_TIME command
    const dv = new DataView(p.buffer, 1, 4);
    dv.setUint32(0, epoch, false);
    await this.sendCommand(0x01, p.slice(1));
  }

  async setSampleInterval(minutes: number): Promise<void> {
    const p = new Uint8Array(2);
    p[0] = 0x02; // SET_INTERVAL
    p[1] = minutes;
    await this.sendCommand(0x02, p.slice(1));
  }

  async startFrostWatch(): Promise<void> {
    await this.sendCommand(0x03);
  }

  async stopFrostWatch(): Promise<void> {
    await this.sendCommand(0x04);
  }

  async provision(nodeId: number, meshRole: number, networkKey: Uint8Array): Promise<void> {
    if (networkKey.length !== 16) throw new Error('Network key must be 16 bytes');
    const p = new Uint8Array(18);
    p[0] = nodeId;
    p[1] = meshRole;
    p.set(networkKey, 2);
    await this.sendCommand(0x10, p);
  }

  async calibrateLeafWetness(threshold: number): Promise<void> {
    const p = new Uint8Array(3);
    p[0] = 0x20; // CALIBRATE_WETNESS
    const dv = new DataView(p.buffer, 1, 2);
    dv.setUint16(0, threshold, false);
    await this.sendCommand(0x20, p.slice(1));
  }

  async resetAEBaseline(): Promise<void> {
    await this.sendCommand(0x30);
  }

  on(event: string, listener: Listener): () => void {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event)!.push(listener);
    return () => {
      const arr = this.listeners.get(event);
      if (arr) {
        const idx = arr.indexOf(listener);
        if (idx >= 0) arr.splice(idx, 1);
      }
    };
  }

  private emit(event: string, data: any): void {
    const arr = this.listeners.get(event);
    if (arr) {
      arr.forEach(fn => fn(data));
    }
  }

  private bytesToBase64(bytes: Uint8Array): string {
    let hex = '';
    for (let i = 0; i < bytes.length; i++) {
      hex += String.fromCharCode(bytes[i]);
    }
    return btoa(hex);
  }
}

export default new BleManager();