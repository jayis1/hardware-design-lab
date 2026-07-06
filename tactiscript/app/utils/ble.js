/**
 * ble.js — BLE Manager for TactiScript companion app
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Manages the BLE 5.0 connection to the TactiScript ring.
 * Uses react-native-ble-plx for cross-platform BLE support.
 *
 * GATT Service UUIDs (custom 128-bit):
 *   Text Pipe:    0001 — write UTF-8 text for Braille rendering
 *   Command:      0002 — write control bytes
 *   Status:       0003 — notify: battery, mode, connection
 *   Texture:      0004 — write raw 4×6 frame data
 *   Gesture:      0005 — notify: detected gesture events
 *   Nav Vector:   0006 — write navigation direction byte
 */

import { BleManager, State } from 'react-native-ble-plx';

// Custom service UUID base
const SERVICE_UUID_BASE = 'aabbccddee11-4201-0000-000000000000';
const SERVICE_UUID = 'aabbccddee1142010000000000000000';

// Characteristic UUIDs (short form, last 4 hex digits)
const CHAR_TEXT = 'aabbccddee1142010001000000000000';
const CHAR_CMD = 'aabbccddee1142010002000000000000';
const CHAR_STATUS = 'aabbccddee1142010003000000000000';
const CHAR_TEXTURE = 'aabbccddee1142010004000000000000';
const CHAR_GESTURE = 'aabbccddee1142010005000000000000';
const CHAR_NAV = 'aabbccddee1142010006000000000000';

// Device name to scan for
const DEVICE_NAME = 'TactiScript';

// Connection states
export const ConnectionState = {
  DISCONNECTED: 'disconnected',
  SCANNING: 'scanning',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  ERROR: 'error',
};

// Command bytes (must match firmware board.h)
export const Commands = {
  MODE_READER: 0x01,
  MODE_NAV: 0x02,
  MODE_MUSIC: 0x03,
  MODE_TUTOR: 0x04,
  MODE_TEXTURE: 0x05,
  MODE_SLEEP: 0x06,
  SPEED_UP: 0x10,
  SPEED_DOWN: 0x11,
  INTENSITY_UP: 0x12,
  INTENSITY_DOWN: 0x13,
  BRAILLE_GRADE1: 0x20,
  BRAILLE_GRADE2: 0x21,
  PAUSE: 0x30,
  RESUME: 0x31,
  CLEAR: 0x40,
};

// Navigation directions
export const NavDirection = {
  STOP: 0,
  FORWARD: 1,
  RIGHT: 2,
  BACKWARD: 3,
  LEFT: 4,
  NE: 5,
  SE: 6,
  SW: 7,
  NW: 8,
};

class BLEManagerClass {
  constructor() {
    this.bleManager = null;
    this.device = null;
    this.connectionState = ConnectionState.DISCONNECTED;
    this.connectionListeners = [];
    this.statusListeners = [];
    this.gestureListeners = [];
    this.statusSubscription = null;
    this.gestureSubscription = null;
  }

  init() {
    if (this.bleManager) return;
    this.bleManager = new BleManager();

    // Check Bluetooth adapter state
    this.bleManager.state().then((state) => {
      if (state === State.PoweredOn) {
        this.connect();
      }
    });

    // Listen for adapter state changes
    this.bleManager.onStateChange((state) => {
      if (state === State.PoweredOn && this.connectionState === ConnectionState.DISCONNECTED) {
        this.connect();
      }
    });
  }

  onConnectionChange(callback) {
    this.connectionListeners.push(callback);
    // Return unsubscribe function
    return () => {
      this.connectionListeners = this.connectionListeners.filter(l => l !== callback);
    };
  }

  onStatusUpdate(callback) {
    this.statusListeners.push(callback);
    return () => {
      this.statusListeners = this.statusListeners.filter(l => l !== callback);
    };
  }

  onGesture(callback) {
    this.gestureListeners.push(callback);
    return () => {
      this.gestureListeners = this.gestureListeners.filter(l => l !== callback);
    };
  }

  _setConnectionState(state) {
    this.connectionState = state;
    this.connectionListeners.forEach(l => l(state));
  }

  async connect() {
    if (!this.bleManager) return;
    this._setConnectionState(ConnectionState.SCANNING);

    try {
      // Scan for TactiScript devices
      this.bleManager.startDeviceScan([SERVICE_UUID], null, (error, device) => {
        if (error) {
          console.error('[BLE] Scan error:', error);
          this._setConnectionState(ConnectionState.ERROR);
          return;
        }

        if (device && device.name === DEVICE_NAME) {
          this.bleManager.stopDeviceScan();
          this._connectToDevice(device);
        }
      });
    } catch (e) {
      console.error('[BLE] Connect failed:', e);
      this._setConnectionState(ConnectionState.ERROR);
    }
  }

  async _connectToDevice(device) {
    this._setConnectionState(ConnectionState.CONNECTING);

    try {
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Subscribe to status notifications
      const statusChar = await this.device.readCharacteristic(CHAR_STATUS);
      this.statusSubscription = this.device.monitorCharacteristicForService(
        SERVICE_UUID, CHAR_STATUS, (error, characteristic) => {
          if (error || !characteristic?.value) return;
          const decoded = this._decodeStatus(characteristic.value);
          this.statusListeners.forEach(l => l(decoded));
        }
      );

      // Subscribe to gesture notifications
      this.gestureSubscription = this.device.monitorCharacteristicForService(
        SERVICE_UUID, CHAR_GESTURE, (error, characteristic) => {
          if (error || !characteristic?.value) return;
          const gesture = this._base64ToBytes(characteristic.value)[0];
          this.gestureListeners.forEach(l => l(gesture));
        }
      );

      this._setConnectionState(ConnectionState.CONNECTED);
    } catch (e) {
      console.error('[BLE] Device connect failed:', e);
      this._setConnectionState(ConnectionState.ERROR);
      // Retry after 3 seconds
      setTimeout(() => this.connect(), 3000);
    }
  }

  _base64ToBytes(base64) {
    // Simple base64 decode (in production, use a proper library)
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const bytes = [];
    for (let i = 0; i < base64.length; i += 4) {
      const a = chars.indexOf(base64[i]);
      const b = chars.indexOf(base64[i + 1]);
      const c = chars.indexOf(base64[i + 2]);
      const d = chars.indexOf(base64[i + 3]);
      if (a >= 0) bytes.push((a << 2) | (b >> 4));
      if (c >= 0) bytes.push(((b & 0xf) << 4) | (c >> 2));
      if (d >= 0) bytes.push(((c & 0x3) << 6) | d);
    }
    return bytes;
  }

  _bytesToBase64(bytes) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let result = '';
    for (let i = 0; i < bytes.length; i += 3) {
      const a = bytes[i] || 0;
      const b = bytes[i + 1] || 0;
      const c = bytes[i + 2] || 0;
      result += chars[(a >> 2) & 0x3f];
      result += chars[((a & 0x3) << 4) | ((b >> 4) & 0xf)];
      result += (i + 1 < bytes.length) ? chars[((b & 0xf) << 2) | ((c >> 6) & 0x3)] : '=';
      result += (i + 2 < bytes.length) ? chars[c & 0x3f] : '=';
    }
    return result;
  }

  _decodeStatus(base64Value) {
    const bytes = this._base64ToBytes(base64Value);
    return {
      batteryPct: bytes[0] || 0,
      charging: bytes[1] === 1,
      mode: bytes[2] || 0,
      connected: bytes[3] === 1,
    };
  }

  async sendText(text) {
    if (!this.device || this.connectionState !== ConnectionState.CONNECTED) return;
    try {
      // Convert text to UTF-8 bytes, then base64
      const encoder = new TextEncoder();
      const bytes = encoder.encode(text);
      // BLE MTU is typically 20 bytes; chunk the text
      const chunkSize = 20;
      for (let i = 0; i < bytes.length; i += chunkSize) {
        const chunk = bytes.slice(i, i + chunkSize);
        const base64 = this._bytesToBase64(Array.from(chunk));
        await this.device.writeCharacteristicWithResponseForService(
          SERVICE_UUID, CHAR_TEXT, base64
        );
      }
    } catch (e) {
      console.error('[BLE] sendText failed:', e);
    }
  }

  async sendCommand(cmd) {
    if (!this.device || this.connectionState !== ConnectionState.CONNECTED) return;
    try {
      const base64 = this._bytesToBase64([cmd]);
      await this.device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_CMD, base64
      );
    } catch (e) {
      console.error('[BLE] sendCommand failed:', e);
    }
  }

  async sendTexture(frame) {
    // frame: array of 24 bytes (4×6 array)
    if (!this.device || this.connectionState !== ConnectionState.CONNECTED) return;
    if (frame.length !== 24) {
      console.error('[BLE] Texture frame must be 24 bytes');
      return;
    }
    try {
      const base64 = this._bytesToBase64(frame);
      await this.device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_TEXTURE, base64
      );
    } catch (e) {
      console.error('[BLE] sendTexture failed:', e);
    }
  }

  async sendNavDirection(direction) {
    if (!this.device || this.connectionState !== ConnectionState.CONNECTED) return;
    try {
      const base64 = this._bytesToBase64([direction]);
      await this.device.writeCharacteristicWithResponseForService(
        SERVICE_UUID, CHAR_NAV, base64
      );
    } catch (e) {
      console.error('[BLE] sendNav failed:', e);
    }
  }

  async disconnect() {
    if (this.statusSubscription) this.statusSubscription.remove();
    if (this.gestureSubscription) this.gestureSubscription.remove();
    if (this.device) {
      try {
        await this.device.cancelConnection();
      } catch (e) {
        // ignore
      }
      this.device = null;
    }
    this._setConnectionState(ConnectionState.DISCONNECTED);
  }
}

export const BLEManager = new BLEManagerClass();