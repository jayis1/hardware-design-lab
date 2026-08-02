/*
 * ble.ts — BLE connection to the HydraScan device
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Uses react-native-ble-plx to scan for and connect to the HydraScan
 * device (Nordic UART Service UUID 6E400001-B5A3-F393-E0A9-E50E24DCCA9E).
 * The device pushes ASCII lines of the form:
 *   R,<class_id>,<confidence>,<adulterant>,<ratio>,<temp_c>\n
 * on the TX characteristic (6E400003-...). We parse and emit results.
 */

import { BleManager, Device, Characteristic } from 'react-native-ble-plx';

// Nordic UART Service + RX/TX characteristics
const NUS_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const NUS_RX      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write
const NUS_TX      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify

export interface HydraResult {
  classId: number;
  name: string;
  confidence: number;        // 0..1
  adulterant: boolean;
  adulterantRatio: number;   // 0..1
  tempC: number;
  timestamp: number;
}

export interface HydraFingerprint {
  optical: number[];         // 8 absorbances
  eisReal: number[];         // 20
  eisImag: number[];         // 20
}

type ResultCallback = (r: HydraResult) => void;

class HydraBLE {
  private manager = new BleManager();
  private device: Device | null = null;
  private lineBuf = '';
  private listeners: ResultCallback[] = [];

  /** Scan for a device advertising the HydraScan name. */
  async scanAndConnect(): Promise<Device> {
    return new Promise((resolve, reject) => {
      const sub = this.manager.startDeviceScan(
        [NUS_SERVICE],
        null,
        (error, dev) => {
          if (error) { sub(); reject(error); return; }
          if (dev && dev.name && dev.name.startsWith('HydraScan')) {
            this.manager.stopDeviceScan();
            void this.connect(dev);
            resolve(dev);
          }
        },
      );
      setTimeout(() => { sub(); reject(new Error('scan timeout')); }, 15000);
    });
  }

  private async connect(dev: Device): Promise<void> {
    this.device = await dev.connect();
    await this.device.discoverAllServicesAndCharacteristics();
    const tx = await this.device.readCharacteristicForService(
      NUS_SERVICE, NUS_TX);
    tx?.monitor((err, c) => this.onNotify(err, c));
  }

  private onNotify(err: Error | null, c: Characteristic | null): void {
    if (err || !c || !c.value) return;
    const chunk = atob(c.value);
    this.lineBuf += chunk;
    let nl: number;
    while ((nl = this.lineBuf.indexOf('\n')) >= 0) {
      const line = this.lineBuf.slice(0, nl).trim();
      this.lineBuf = this.lineBuf.slice(nl + 1);
      this.parseLine(line);
    }
  }

  private parseLine(line: string): void {
    if (!line.startsWith('R,')) return;
    const parts = line.split(',');
    if (parts.length < 6) return;
    const r: HydraResult = {
      classId: parseInt(parts[1], 10),
      name: parts[1],                       // app resolves name from library
      confidence: parseFloat(parts[2]),
      adulterant: parts[3] === '1',
      adulterantRatio: parseFloat(parts[4]),
      tempC: parseFloat(parts[5]),
      timestamp: Date.now(),
    };
    for (const l of this.listeners) l(r);
  }

  onResult(cb: ResultCallback): () => void {
    this.listeners.push(cb);
    return () => {
      this.listeners = this.listeners.filter(f => f !== cb);
    };
  }

  /** Send a library add command line. */
  async sendLibraryAdd(payload: string): Promise<void> {
    if (!this.device) return;
    await this.device.writeCharacteristicWithResponseForService(
      NUS_SERVICE, NUS_RX, btoa(payload + '\n'));
  }

  isConnected(): boolean { return !!this.device; }

  disconnect(): void {
    if (this.device) void this.device.cancelConnection();
    this.device = null;
  }
}

export const hydra = new HydraBLE();