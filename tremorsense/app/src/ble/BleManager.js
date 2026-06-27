/*
 * BleManager.js — BLE communication manager
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import { BleManager: PlxManager, State } from 'react-native-ble-plx';

const TREMOR_SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const TREMOR_STATUS_UUID   = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
const DATA_SERVICE_UUID    = '6e400010-b5a3-f393-e0a9-e50e24dcca9e';
const CONFIG_SERVICE_UUID   = '6e400020-b5a3-f393-e0a9-e50e24dcca9e';

class TremorBleManager {
  constructor() {
    this.manager = null;
    this.device = null;
    this.connectionState = 'disconnected';
    this.listeners = { connectionChange: [], data: [] };
    this.statusSubscription = null;
  }

  initialize() {
    this.manager = new PlxManager();
    this.manager.onStateChange((state) => {
      switch (state) {
        case State.PoweredOn:
          console.log('BLE powered on');
          break;
        case State.PoweredOff:
          this.connectionState = 'disconnected';
          this._notifyConnectionChange('disconnected');
          break;
        case State.Unauthorized:
          console.warn('BLE unauthorized');
          break;
      }
    }, true);
  }

  onConnectionChange(callback) {
    this.listeners.connectionChange.push(callback);
    return () => {
      this.listeners.connectionChange = this.listeners.connectionChange.filter(c => c !== callback);
    };
  }

  onData(callback) {
    this.listeners.data.push(callback);
    return () => {
      this.listeners.data = this.listeners.data.filter(c => c !== callback);
    };
  }

  _notifyConnectionChange(state) {
    this.connectionState = state;
    this.listeners.connectionChange.forEach(cb => cb(state));
  }

  _notifyData(data) {
    this.listeners.data.forEach(cb => cb(data));
  }

  async scanAndConnect() {
    if (!this.manager) this.initialize();

    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.error('Scan error:', error);
        return;
      }

      if (device.name && device.name.startsWith('TremorSense')) {
        this.manager.stopDeviceScan();
        this.connectToDevice(device);
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      this.manager.stopDeviceScan();
    }, 10000);
  }

  async connectToDevice(device) {
    try {
      this._notifyConnectionChange('connecting');
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Subscribe to tremor status notifications
      const statusChar = await this.device.readCharacteristicForService(
        TREMOR_SERVICE_UUID, TREMOR_STATUS_UUID
      );

      this.statusSubscription = statusChar.monitor((error, char) => {
        if (error) {
          console.error('Notification error:', error);
          return;
        }
        if (char && char.value) {
          const data = this._parseStatusData(char.value);
          this._notifyData(data);
        }
      });

      this._notifyConnectionChange('connected');
    } catch (e) {
      console.error('Connection failed:', e);
      this._notifyConnectionChange('disconnected');
    }
  }

  _parseStatusData(base64Value) {
    // Decode base64 to bytes
    const bytes = this._base64ToBytes(base64Value);
    if (bytes.length < 14) return null;

    // Parse 14-byte notification:
    // [0-3] frequency (float32 LE)
    // [4-7] amplitude (float32 LE)
    // [8] tremor class
    // [9] confidence %
    // [10] context
    // [11-12] daily score (uint16 LE)
    // [13] battery %
    const frequency = this._bytesToFloat(bytes, 0);
    const amplitude = this._bytesToFloat(bytes, 4);
    const tremorClass = bytes[8];
    const confidence = bytes[9] / 100.0;
    const context = bytes[10];
    const score = bytes[11] | (bytes[12] << 8);
    const battery = bytes[13];

    return { frequency, amplitude, tremorClass, confidence, context, score, battery };
  }

  _base64ToBytes(base64) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const result = [];
    for (let i = 0; i < base64.length; i += 4) {
      const c1 = chars.indexOf(base64[i]);
      const c2 = chars.indexOf(base64[i + 1]);
      const c3 = chars.indexOf(base64[i + 2]);
      const c4 = chars.indexOf(base64[i + 3]);
      result.push((c1 << 2) | (c2 >> 4));
      if (c3 !== 64) result.push(((c2 & 15) << 4) | (c3 >> 2));
      if (c4 !== 64) result.push(((c3 & 3) << 6) | c4);
    }
    return result;
  }

  _bytesToFloat(bytes, offset) {
    const buf = new ArrayBuffer(4);
    const view = new DataView(buf);
    view.setUint8(0, bytes[offset]);
    view.setUint8(1, bytes[offset + 1]);
    view.setUint8(2, bytes[offset + 2]);
    view.setUint8(3, bytes[offset + 3]);
    return view.getFloat32(0, true);
  }

  async sendCommand(command, params) {
    if (!this.device) return false;
    try {
      const char = await this.device.writeCharacteristicWithResponseForService(
        CONFIG_SERVICE_UUID, command, this._paramsToBase64(params)
      );
      return true;
    } catch (e) {
      console.error('Command failed:', e);
      return false;
    }
  }

  _paramsToBase64(params) {
    // Simple encoding: JSON to base64
    const json = JSON.stringify(params);
    return btoa(json);
  }

  async downloadAllEpisodes(progressCallback) {
    if (!this.device) return [];

    const episodes = [];
    // In production: send download command, receive chunked data
    // For now: return empty array (data is loaded from AsyncStorage)
    return episodes;
  }

  async startDFU() {
    // Nordic Secure DFU would be initiated here
    // Requires nordic-nrf-device-lib or similar
    if (!this.device) throw new Error('Device not connected');
    // Implementation would use nrf-device-lib for DFU
  }

  async disconnect() {
    if (this.statusSubscription) {
      this.statusSubscription.remove();
      this.statusSubscription = null;
    }
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
    }
    this._notifyConnectionChange('disconnected');
  }

  enable() {
    if (this.manager) {
      this.manager.enable();
    }
  }

  disable() {
    if (this.manager) {
      this.manager.disable();
    }
  }
}

export default new TremorBleManager();