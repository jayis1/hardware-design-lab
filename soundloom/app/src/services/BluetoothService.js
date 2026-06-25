/**
 * BluetoothService.js — BLE communication with SoundLom pods
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Manages BLE scanning, connection, and data transfer with buried
 * SoundLoom sensor pods. Uses a protocol over Nordic UART Service (NUS)
 * for configuration and high-rate event offload.
 */

import { BleManager } from 'react-native-ble-plx';

// SoundLoom BLE service UUID (Nordic UART Service)
const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const TX_CHAR_UUID  = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write
const RX_CHAR_UUID  = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify

// Protocol commands (over BLE NUS)
const CMD_PING        = 0x01;
const CMD_GET_STATUS  = 0x02;
const CMD_GET_SVI     = 0x03;
const CMD_GET_EVENTS  = 0x04;
const CMD_SET_CADENCE = 0x05;
const CMD_SET_SENS    = 0x06;
const CMD_START_OTA   = 0x07;
const CMD_GET_SOUNDF  = 0x08;
const CMD_TAP_TEST    = 0x09;
const CMD_REBOOT      = 0x0A;

class BluetoothService {
  constructor() {
    this.manager = null;
    this.connectedDevice = null;
    this.rxCallback = null;
    this.rxBuffer = [];
  }

  async initialise() {
    this.manager = new BleManager();
    // Wait for adapter to power on
    return new Promise((resolve, reject) => {
      const sub = this.manager.onStateChange((state) => {
        if (state === 'PoweredOn') {
          sub.remove();
          resolve();
        } else if (state === 'Unsupported' || state === 'Unauthorized') {
          sub.remove();
          reject(new Error(`Bluetooth ${state}`));
        }
      }, true);
    });
  }

  /**
   * Scan for nearby SoundLoom pods.
   * Returns an array of discovered devices.
   */
  async scanForPods(timeoutMs = 10000) {
    if (!this.manager) throw new Error('BLE not initialised');
    return new Promise((resolve) => {
      const devices = new Map();
      this.manager.startDeviceScan([SERVICE_UUID], null, (error, device) => {
        if (error) { console.error('BLE scan error:', error); return; }
        if (device && !devices.has(device.id)) {
          devices.set(device.id, {
            id: device.id,
            name: device.name || 'SoundLoom Pod',
            rssi: device.rssi,
            advertising: device.advertisingData,
          });
        }
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        resolve(Array.from(devices.values()));
      }, timeoutMs);
    });
  }

  /**
   * Connect to a SoundLoom pod by device ID.
   */
  async connect(deviceId) {
    if (!this.manager) throw new Error('BLE not initialised');
    this.connectedDevice = await this.manager.connectToDevice(deviceId);
    await this.connectedDevice.discoverAllServicesAndCharacteristics();
    // Subscribe to RX notifications
    this.connectedDevice.monitorCharacteristicForService(SERVICE_UUID, RX_CHAR_UUID,
      (error, char) => {
        if (error) { console.error('BLE notify error:', error); return; }
        if (char && char.value) {
          this._handleRxData(char.value);
        }
      });
    return this.connectedDevice;
  }

  /**
   * Disconnect from the current pod.
   */
  async disconnect() {
    if (this.connectedDevice) {
      await this.connectedDevice.cancelConnection();
      this.connectedDevice = null;
    }
  }

  /**
   * Send a command to the connected pod.
   */
  async sendCommand(cmd, payload = []) {
    if (!this.connectedDevice) throw new Error('Not connected');
    const data = new Uint8Array([cmd, ...payload]);
    const base64 = this._bytesToBase64(data);
    await this.connectedDevice.writeCharacteristicWithResponseForService(
      SERVICE_UUID, TX_CHAR_UUID, base64);
  }

  /**
   * Get pod status (SVI, battery, cycle count, uptime).
   */
  async getStatus() {
    await this.sendCommand(CMD_GET_STATUS);
    return this._waitForResponse(2000).then(this._parseStatus.bind(this));
  }

  /**
   * Get current Soil Vitality Index.
   */
  async getSVI() {
    await this.sendCommand(CMD_GET_SVI);
    return this._waitForResponse(2000);
  }

  /**
   * Get recent classified events (from SD card or live buffer).
   */
  async getEvents(count = 100) {
    await this.sendCommand(CMD_GET_EVENTS, [count & 0xFF, (count >> 8) & 0xFF]);
    return this._waitForResponse(5000).then(this._parseEvents.bind(this));
  }

  /**
   * Set the measurement cadence (minutes).
   */
  async setCadence(minutes) {
    const mins = Math.max(1, Math.min(1440, minutes));
    await this.sendCommand(CMD_SET_CADENCE, [mins & 0xFF, (mins >> 8) & 0xFF]);
  }

  /**
   * Set the classifier sensitivity (threshold factor, fixed point ×100).
   */
  async setSensitivity(factor) {
    const val = Math.round(factor * 100);
    await this.sendCommand(CMD_SET_SENS, [val & 0xFF, (val >> 8) & 0xFF]);
  }

  /**
   * Start the tap-test calibration sequence.
   */
  async startTapTest() {
    await this.sendCommand(CMD_TAP_TEST);
    return this._waitForResponse(10000);
  }

  /**
   * Reboot the pod.
   */
  async reboot() {
    await this.sendCommand(CMD_REBOOT);
  }

  // ---- Private helpers ----

  _handleRxData(base64) {
    const bytes = this._base64ToBytes(base64);
    for (const b of bytes) this.rxBuffer.push(b);
    // Check for complete response (simple framing: 0xAA, cmd, len_hi, len_lo, data..., CRC)
    while (this.rxBuffer.length >= 5) {
      if (this.rxBuffer[0] !== 0xAA) { this.rxBuffer.shift(); continue; }
      const dataLen = (this.rxBuffer[3] << 8) | this.rxBuffer[2];
      const totalLen = 5 + dataLen;
      if (this.rxBuffer.length < totalLen) break;
      const response = this.rxBuffer.slice(0, totalLen);
      this.rxBuffer = this.rxBuffer.slice(totalLen);
      if (this._rxResolve) {
        this._rxResolve(response);
        this._rxResolve = null;
      }
    }
  }

  _waitForResponse(timeout) {
    return new Promise((resolve, reject) => {
      this._rxResolve = resolve;
      setTimeout(() => {
        if (this._rxResolve === resolve) {
          this._rxResolve = null;
          reject(new Error('BLE response timeout'));
        }
      }, timeout);
    });
  }

  _parseStatus(response) {
    // Status: [0xAA, cmd, len_h, len_l, svi, bat_pct, cycles(4), uptime(4), rssi, snr, flags(2)]
    if (response.length < 14) return null;
    return {
      svi: response[4],
      batteryPct: response[5],
      cycles: (response[6] << 24) | (response[7] << 16) | (response[8] << 8) | response[9],
      uptime: (response[10] << 24) | (response[11] << 16) | (response[12] << 8) | response[13],
      rssi: response.length > 14 ? (response[14] > 127 ? response[14] - 256 : response[14]) : 0,
      snr: response.length > 15 ? (response[15] > 127 ? response[15] - 256 : response[15]) : 0,
      flags: response.length > 17 ? (response[16] << 8) | response[17] : 0,
    };
  }

  _parseEvents(response) {
    // Events: [0xAA, cmd, len_h, len_l, count, events...]
    if (response.length < 6) return [];
    const count = response[4];
    const events = [];
    let offset = 5;
    for (let i = 0; i < count && offset + 20 <= response.length; i++) {
      events.push({
        id: offset,
        timestamp: (response[offset] << 24) | (response[offset+1] << 16) | (response[offset+2] << 8) | response[offset+3],
        classId: response[offset + 4],
        channel: response[offset + 5],
        confidence: response[offset + 6],
        posX: (response[offset + 7] << 8 | response[offset + 8]) / 100 - 128,
        posY: (response[offset + 9] << 8 | response[offset + 10]) / 100 - 128,
        posZ: (response[offset + 11] << 8 | response[offset + 12]) / 100 - 128,
        residual: ((response[offset + 13] << 8) | response[offset + 14]) / 100,
      });
      offset += 20;
    }
    return events;
  }

  _bytesToBase64(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
    return btoa(binary);
  }

  _base64ToBytes(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
    return bytes;
  }
}

export default new BluetoothService();