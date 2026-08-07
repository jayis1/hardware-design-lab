/**
 * ble.js — BLE Connection Manager for FermenTiq
 *
 * Manages the BLE connection to the FermenTiq device, handles scanning,
 * connection, GATT characteristic reads/writes, and live data
 * notifications. Provides a React Context for sharing state across
 * the app.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { createContext } from 'react';
import { BleManager as RNBleManager, State } from 'react-native-ble-plx';
import { Platform, PermissionsAndroid } from 'react-native';
import { parseLiveData, parseConfig, packConfig, packCommand } from './protocol';

// Custom service/characteristic UUIDs (match firmware)
const SERVICE_LIVE_DATA = '0000caf1-0000-1000-8000-00805f9b34fb';
const SERVICE_CONFIG    = '0000caf2-0000-1000-8000-00805f9b34fb';
const SERVICE_ALERT     = '0000caf3-0000-1000-8000-00805f9b34fb';

const CHAR_LIVE_FUSION  = '0000cb05-0000-1000-8000-00805f9b34fb';
const CHAR_CONFIG_BATCH = '0000cc01-0000-1000-8000-00805f9b34fb';
const CHAR_CONFIG_CMD   = '0000cc03-0000-1000-8000-00805f9b34fb';
const CHAR_ALERT        = '0000cd01-0000-1000-8000-00805f9b34fb';

const DEVICE_NAME_PREFIX = 'FermenTiq';

// React Context for sharing BLE state
export const FermenTiqContext = createContext(null);

export class BleManager {
  constructor() {
    this.manager = new RNBleManager();
    this.connectedDevice = null;
    this.liveDataCallbacks = new Set();
    this.alertCallbacks = new Set();
    this.scanCallbacks = new Set();
    this.subscription = null;
    this.alertSubscription = null;
  }

  /**
   * Request Android BLE permissions
   */
  async requestPermissions() {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(
        p => p === PermissionsAndroid.RESULTS.GRANTED
      );
    }
    return true;
  }

  /**
   * Start scanning for FermenTiq devices
   */
  async startScan(onDeviceFound) {
    const hasPermission = await this.requestPermissions();
    if (!hasPermission) {
      console.error('BLE permissions denied');
      return;
    }

    const state = await this.manager.state();
    if (state !== State.PoweredOn) {
      console.error('Bluetooth is not powered on');
      return;
    }

    if (onDeviceFound) {
      this.scanCallbacks.add(onDeviceFound);
    }

    this.manager.startDeviceScan(null, { allowDuplicates: false }, (error, device) => {
      if (error) {
        console.error('Scan error:', error);
        return;
      }
      if (device.name && device.name.startsWith(DEVICE_NAME_PREFIX)) {
        this.scanCallbacks.forEach(cb => cb({
          id: device.id,
          name: device.name,
          rssi: device.rssi,
        }));
      }
    });
  }

  stopScan() {
    this.manager.stopDeviceScan();
  }

  /**
   * Connect to a device by ID
   */
  async connect(deviceId) {
    this.stopScan();
    const device = await this.manager.connectToDevice(deviceId);
    await device.discoverAllServicesAndCharacteristics();
    this.connectedDevice = device;

    // Subscribe to live data notifications
    const liveChar = await device.readCharacteristicForService(
      SERVICE_LIVE_DATA, CHAR_LIVE_FUSION
    );
    this.subscription = liveChar.monitor((error, char) => {
      if (error) {
        console.error('Live data monitor error:', error);
        return;
      }
      if (char && char.value) {
        const data = parseLiveData(char.value);
        this.liveDataCallbacks.forEach(cb => cb(data));
      }
    });

    // Subscribe to alert notifications
    const alertChar = await device.readCharacteristicForService(
      SERVICE_ALERT, CHAR_ALERT
    );
    this.alertSubscription = alertChar.monitor((error, char) => {
      if (error) {
        console.error('Alert monitor error:', error);
        return;
      }
      if (char && char.value) {
        const alert = this.parseAlert(char.value);
        this.alertCallbacks.forEach(cb => cb(alert));
      }
    });

    console.log('Connected to FermenTiq:', device.name);
    return device;
  }

  /**
   * Disconnect from current device
   */
  async disconnect() {
    if (this.subscription) {
      this.subscription.remove();
      this.subscription = null;
    }
    if (this.alertSubscription) {
      this.alertSubscription.remove();
      this.alertSubscription = null;
    }
    if (this.connectedDevice) {
      await this.manager.cancelDeviceConnection(this.connectedDevice.id);
      this.connectedDevice = null;
    }
  }

  /**
   * Read configuration from device
   */
  async readConfig() {
    if (!this.connectedDevice) return null;
    const char = await this.connectedDevice.readCharacteristicForService(
      SERVICE_CONFIG, CHAR_CONFIG_BATCH
    );
    return parseConfig(char.value);
  }

  /**
   * Write configuration to device
   */
  async writeConfig(config) {
    if (!this.connectedDevice) return;
    const data = packConfig(config);
    await this.connectedDevice.writeCharacteristicWithResponseForService(
      SERVICE_CONFIG, CHAR_CONFIG_BATCH, data
    );
  }

  /**
   * Send a command to the device
   */
  async sendCommand(command) {
    if (!this.connectedDevice) return;
    const data = packCommand(command);
    await this.connectedDevice.writeCharacteristicWithResponseForService(
      SERVICE_CONFIG, CHAR_CONFIG_CMD, data
    );
  }

  /**
   * Register callback for live data updates
   */
  onLiveData(callback) {
    this.liveDataCallbacks.add(callback);
    return () => this.liveDataCallbacks.delete(callback);
  }

  /**
   * Register callback for alerts
   */
  onAlert(callback) {
    this.alertCallbacks.add(callback);
    return () => this.alertCallbacks.delete(callback);
  }

  /**
   * Parse alert notification from base64
   */
  parseAlert(base64Value) {
    // Decode base64 to bytes (simplified — in production use a base64 decoder)
    const bytes = this.base64ToBytes(base64Value);
    return {
      severity: bytes[0],
      message: String.fromCharCode(...bytes.slice(1)).replace(/\0/g, ''),
      timestamp: Date.now(),
    };
  }

  /**
   * Simple base64 to byte array converter
   */
  base64ToBytes(base64) {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const clean = base64.replace(/=+$/, '');
    const bytes = [];
    for (let i = 0; i < clean.length; i += 4) {
      const n = (chars.indexOf(clean[i]) << 18) |
                (chars.indexOf(clean[i+1]) << 12) |
                ((clean[i+2] ? chars.indexOf(clean[i+2]) : 0) << 6) |
                (clean[i+3] ? chars.indexOf(clean[i+3]) : 0);
      bytes.push((n >> 16) & 0xFF);
      if (clean[i+2]) bytes.push((n >> 8) & 0xFF);
      if (clean[i+3]) bytes.push(n & 0xFF);
    }
    return bytes;
  }

  /**
   * Cleanup
   */
  destroy() {
    this.disconnect();
    this.manager.destroy();
  }
}