/**
 * BleManager.ts — BLE connection manager for the LithoCore device.
 *
 * Handles scanning, connection, and data exchange over BLE 5.2 using
 * the LithoCore GATT service. Manages the characteristic subscriptions
 * and provides a high-level API for the screens.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import {
  BleManager,
  Device,
  Characteristic,
  BleError,
  State as BleState,
} from 'react-native-ble-plx';
import {
  LITHOCORE_SERVICE_UUID,
  CMD_CHARACTERISTIC_UUID,
  SWEEP_DATA_CHARACTERISTIC_UUID,
  RESULT_CHARACTERISTIC_UUID,
  STATUS_CHARACTERISTIC_UUID,
  unpackSweepPoint,
  unpackResult,
  unpackStatus,
  packCommand,
  CellResult,
  SweepPoint,
  DeviceStatus,
} from './protocol';

type ConnectionCallback = (connected: boolean) => void;
type SweepPointCallback = (point: SweepPoint) => void;
type ResultCallback = (result: CellResult) => void;
type StatusCallback = (status: DeviceStatus) => void;

class LithoCoreBleManager {
  private bleManager: BleManager;
  private device: Device | null = null;
  private isConnected: boolean = false;

  private connectionCallbacks: ConnectionCallback[] = [];
  private sweepPointCallbacks: SweepPointCallback[] = [];
  private resultCallbacks: ResultCallback[] = [];
  private statusCallbacks: StatusCallback[] = [];

  constructor() {
    this.bleManager = new BleManager();
  }

  /**
   * Start scanning for LithoCore devices.
   * Author: jayis1
   */
  async startScan(onDeviceFound: (device: Device) => void): Promise<void> {
    this.bleManager.startDeviceScan(
      [LITHOCORE_SERVICE_UUID],
      { allowDuplicates: false },
      (error: BleError | null, device: Device | null) => {
        if (error) {
          console.error('[LithoCore] Scan error:', error.message);
          return;
        }
        if (device && device.name?.startsWith('LithoCore')) {
          onDeviceFound(device);
        }
      }
    );
  }

  stopScan(): void {
    this.bleManager.stopDeviceScan();
  }

  /**
   * Connect to a LithoCore device and set up notification handlers.
   * Author: jayis1
   */
  async connect(device: Device): Promise<boolean> {
    try {
      this.device = await device.connect();
      await this.device.discoverAllServicesAndCharacteristics();

      // Subscribe to sweep data notifications
      await this.device.setupNotifications(SWEEP_DATA_CHARACTERISTIC_UUID);
      this.device.monitorCharacteristicForService(
        LITHOCORE_SERVICE_UUID,
        SWEEP_DATA_CHARACTERISTIC_UUID,
        (error, char) => this.onSweepData(error, char)
      );

      // Subscribe to result notifications
      this.device.monitorCharacteristicForService(
        LITHOCORE_SERVICE_UUID,
        RESULT_CHARACTERISTIC_UUID,
        (error, char) => this.onResult(error, char)
      );

      // Subscribe to status notifications
      this.device.monitorCharacteristicForService(
        LITHOCORE_SERVICE_UUID,
        STATUS_CHARACTERISTIC_UUID,
        (error, char) => this.onStatus(error, char)
      );

      this.isConnected = true;
      this.notifyConnectionCallbacks(true);
      return true;
    } catch (error) {
      console.error('[LithoCore] Connection failed:', error);
      return false;
    }
  }

  /**
   * Disconnect from the device.
   */
  async disconnect(): Promise<void> {
    if (this.device && this.isConnected) {
      await this.device.cancelConnection();
      this.isConnected = false;
      this.device = null;
      this.notifyConnectionCallbacks(false);
    }
  }

  /**
   * Send a command to the device.
   */
  async sendCommand(command: number, data?: Uint8Array): Promise<void> {
    if (!this.device || !this.isConnected) {
      throw new Error('Not connected to device');
    }
    const payload = packCommand(command, data);
    await this.device.writeCharacteristicWithResponseForService(
      LITHOCORE_SERVICE_UUID,
      CMD_CHARACTERISTIC_UUID,
      payload
    );
  }

  /**
   * Start a fast sweep (10 Hz – 100 kHz, ~20 seconds).
   */
  async startFastSweep(): Promise<void> {
    await this.sendCommand(0x01);
  }

  /**
   * Start a full sweep (0.01 Hz – 100 kHz, ~12 minutes).
   */
  async startFullSweep(): Promise<void> {
    await this.sendCommand(0x02);
  }

  /**
   * Abort the current sweep.
   */
  async abortSweep(): Promise<void> {
    await this.sendCommand(0x03);
  }

  /**
   * Request the latest result.
   */
  async requestResult(): Promise<void> {
    await this.sendCommand(0x05);
  }

  /**
   * Request history from the device.
   */
  async requestHistory(): Promise<void> {
    await this.sendCommand(0x06);
  }

  // --- Notification handlers ---

  private onSweepData(error: BleError | null, char: Characteristic | null): void {
    if (error || !char?.value) return;
    const point = unpackSweepPoint(char.value);
    if (point) {
      this.sweepPointCallbacks.forEach((cb) => cb(point));
    }
  }

  private onResult(error: BleError | null, char: Characteristic | null): void {
    if (error || !char?.value) return;
    const result = unpackResult(char.value);
    if (result) {
      this.resultCallbacks.forEach((cb) => cb(result));
    }
  }

  private onStatus(error: BleError | null, char: Characteristic | null): void {
    if (error || !char?.value) return;
    const status = unpackStatus(char.value);
    if (status) {
      this.statusCallbacks.forEach((cb) => cb(status));
    }
  }

  // --- Callback registration ---

  onConnectionChange(cb: ConnectionCallback): void {
    this.connectionCallbacks.push(cb);
  }

  onSweepPoint(cb: SweepPointCallback): void {
    this.sweepPointCallbacks.push(cb);
  }

  onResult(cb: ResultCallback): void {
    this.resultCallbacks.push(cb);
  }

  onStatus(cb: StatusCallback): void {
    this.statusCallbacks.push(cb);
  }

  private notifyConnectionCallbacks(connected: boolean): void {
    this.connectionCallbacks.forEach((cb) => cb(connected));
  }

  get connected(): boolean {
    return this.isConnected;
  }
}

// Singleton instance
export const bleManager = new LithoCoreBleManager();
export default bleManager;