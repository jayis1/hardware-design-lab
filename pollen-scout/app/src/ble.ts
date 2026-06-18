/**
 * ble.ts - BLE interface for Pollen Scout stations
 *
 * Uses expo-bluetooth (or react-native-ble-plx in a native build).
 * Service UUID 0x1001 (Pollen Scout GATT profile):
 *   Characteristic 0x1002 - state (read/notify)
 *   Characteristic 0x1003 - command (write)
 *   Characteristic 0x1004 - provisioning (write)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import { StationState } from './types';

export interface BleDevice {
  id: string;
  name: string;
  rssi: number;
}

export const SCOUT_SERVICE_UUID = '00001000-0000-1000-8000-00805f9b34fb';
export const CHAR_STATE   = '00001002-0000-1000-8000-00805f9b34fb';
export const CHAR_COMMAND = '00001003-0000-1000-8000-00805f9b34fb';
export const CHAR_PROV    = '00001004-0000-1000-8000-00805f9b34fb';

/* In a production native build these wrap the platform BLE manager.
 * The Expo shim here provides a typed surface and demo fallback. */

export async function scan(timeoutMs: number): Promise<BleDevice[]> {
  /* placeholder: would call BleManager.startDeviceScan() and accumulate. */
  await new Promise((r) => setTimeout(r, Math.min(timeoutMs, 200)));
  return [
    { id: 'AA:BB:CC:DD:EE:01', name: 'PollenScout-0001', rssi: -54 },
    { id: 'AA:BB:CC:DD:EE:02', name: 'PollenScout-0002', rssi: -71 },
  ];
}

export async function connect(deviceId: string): Promise<void> {
  console.log(`[BLE] connect ${deviceId}`);
}

export async function pollState(deviceId: string): Promise<StationState> {
  /* In production: read CHAR_STATE, decode the 28-byte binary uplink
   * payload (same format as the LoRa uplink in firmware/main.c). */
  console.log(`[BLE] pollState ${deviceId}`);
  const { makeDemoStation } = await import('./context/StationContext');
  return makeDemoStation();
}

export interface ProvisionPayload {
  wifiSsid: string;
  wifiPw: string;
  joinEui: string;
  appKey: string;
}

export async function provision(
  deviceId: string, p: ProvisionPayload,
): Promise<void> {
  const frame = JSON.stringify(p);
  console.log(`[BLE] provision ${deviceId} ssid=${p.wifiSsid}`);
  /* write CHAR_PROV with `frame` as UTF-8 bytes */
  void frame;
}

export async function sendCommand(deviceId: string, cmd: string): Promise<void> {
  console.log(`[BLE] command ${deviceId} = ${cmd}`);
  /* write CHAR_COMMAND with first byte = cmd[0] ('O','P','R') */
}