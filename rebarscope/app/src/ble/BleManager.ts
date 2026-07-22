/**
 * BleManager — BLE 5 connection manager for RebarScope
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import {
  BLE_SERVICE_UUID,
  BLE_TX_CHAR_UUID,
  BLE_RX_CHAR_UUID,
  CMD,
  RESP,
} from './protocol';

type ResponseHandler = (respId: number, payload: Uint8Array) => void;

class RebarScopeBle {
  private manager: BleManager;
  private device: Device | null = null;
  private rxChar: Characteristic | null = null;
  private txChar: Characteristic | null = null;
  private responseHandler: ResponseHandler | null = null;
  private rxBuffer: number[] = [];
  private isConnected: boolean = false;

  constructor() {
    this.manager = new BleManager();
  }

  async scanAndConnect(timeoutMs = 10000): Promise<Device | null> {
    return new Promise((resolve, reject) => {
      let found = false;
      this.manager.startDeviceScan([BLE_SERVICE_UUID], null, (err, dev) => {
        if (err) {
          this.manager.stopDeviceScan();
          reject(err);
          return;
        }
        if (dev && dev.name && dev.name.startsWith('RebarScope')) {
          this.manager.stopDeviceScan();
          found = true;
          this.connect(dev).then(resolve).catch(reject);
        }
      });
      setTimeout(() => {
        if (!found) {
          this.manager.stopDeviceScan();
          resolve(null);
        }
      }, timeoutMs);
    });
  }

  async connect(device: Device): Promise<Device> {
    this.device = await device.connect();
    await this.device.discoverAllServicesAndCharacteristics();
    const services = await this.device.services();
    const service = services.find((s) => s.uuid.toLowerCase() === BLE_SERVICE_UUID.toLowerCase());
    if (!service) throw new Error('RebarScope service not found');
    const chars = await service.characteristics();
    this.txChar = chars.find((c) => c.uuid.toLowerCase() === BLE_TX_CHAR_UUID.toLowerCase()) ?? null;
    this.rxChar = chars.find((c) => c.uuid.toLowerCase() === BLE_RX_CHAR_UUID.toLowerCase()) ?? null;
    if (!this.txChar || !this.rxChar) throw new Error('RebarScope characteristics not found');

    /* Monitor TX notifications */
    this.txChar.monitor((err, char) => {
      if (err || !char?.value) return;
      const bytes = Buffer.from(char.value, 'base64');
      for (const b of bytes) {
        if (b === 0x00) {
          /* end of COBS frame */
          if (this.rxBuffer.length > 0) {
            this.handleFrame(new Uint8Array(this.rxBuffer));
            this.rxBuffer = [];
          }
        } else {
          this.rxBuffer.push(b);
        }
      }
    });

    this.isConnected = true;
    return this.device;
  }

  setResponseHandler(handler: ResponseHandler): void {
    this.responseHandler = handler;
  }

  private handleFrame(frame: Uint8Array): void {
    /* Frame: [respId][payload] (after COBS decode) — here we simplify and assume
     * the device already decoded COBS; in production we'd do COBS here. */
    if (frame.length < 1) return;
    const respId = frame[0];
    const payload = frame.slice(1);
    if (this.responseHandler) this.responseHandler(respId, payload);
  }

  async sendCommand(cmd: number, payload?: Uint8Array): Promise<void> {
    if (!this.rxChar) throw new Error('Not connected');
    const data = payload ? [cmd, ...payload] : [cmd];
    const b64 = Buffer.from(data).toString('base64');
    await this.rxChar.writeWithoutResponse(b64);
  }

  async ping(): Promise<boolean> {
    return new Promise((resolve) => {
      const timeout = setTimeout(() => resolve(false), 2000);
      this.setResponseHandler((respId) => {
        if (respId === RESP.PONG) {
          clearTimeout(timeout);
          resolve(true);
        }
      });
      this.sendCommand(CMD.PING);
    });
  }

  async getStatus(): Promise<Uint8Array | null> {
    return new Promise((resolve) => {
      const timeout = setTimeout(() => resolve(null), 2000);
      this.setResponseHandler((respId, payload) => {
        if (respId === RESP.STATUS) {
          clearTimeout(timeout);
          resolve(payload);
        }
      });
      this.sendCommand(CMD.GET_STATUS);
    });
  }

  async startSurvey(): Promise<void> {
    await this.sendCommand(CMD.START_SURVEY);
  }

  async stopSurvey(): Promise<void> {
    await this.sendCommand(CMD.STOP_SURVEY);
  }

  async triggerPoint(): Promise<void> {
    await this.sendCommand(CMD.TRIGGER);
  }

  async startLpr(ecorrMv: number): Promise<Uint8Array | null> {
    return new Promise((resolve) => {
      const timeout = setTimeout(() => resolve(null), 600000); /* 10 min */
      this.setResponseHandler((respId, payload) => {
        if (respId === RESP.LPR) {
          clearTimeout(timeout);
          resolve(payload);
        }
      });
      const dv = new DataView(new ArrayBuffer(4));
      dv.setInt32(0, Math.round(ecorrMv), true);
      this.sendCommand(CMD.LPR, new Uint8Array(dv.buffer));
    });
  }

  async getHash(): Promise<Uint8Array | null> {
    return new Promise((resolve) => {
      const timeout = setTimeout(() => resolve(null), 2000);
      this.setResponseHandler((respId, payload) => {
        if (respId === RESP.HASH) {
          clearTimeout(timeout);
          resolve(payload);
        }
      });
      this.sendCommand(CMD.GET_HASH);
    });
  }

  isConnected_(): boolean {
    return this.isConnected;
  }

  async disconnect(): Promise<void> {
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
    }
    this.isConnected = false;
  }

  destroy(): void {
    this.manager.destroy();
  }
}

export default new RebarScopeBle();