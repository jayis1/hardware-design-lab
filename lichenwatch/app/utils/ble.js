/*
 * ble.js — BLE communication utility for Lichenwalk nodes.
 *
 * Author: jayis1
 * License: MIT
 *
 * Uses react-native-ble-plx to scan for advertising Lichenwatch nodes,
 * connect, and read the GATT service:
 *   Service: 0000lic1-0000-1000-8000-00805f9b34fb
 *   Characteristics: lic2 (status), lic3 (bulk), lic4 (config), lic5 (info)
 */

import React, { createContext } from 'react';
import { BleManager } from 'react-native-ble-plx';
import AsyncStorage from '@react-native-async-storage/async-storage';

export const LICHENWATCH_SERVICE_UUID = '0000lic1-0000-1000-8000-00805f9b34fb';
export const STATUS_CHAR_UUID  = '0000lic2-0000-1000-8000-00805f9b34fb';
export const BULK_CHAR_UUID    = '0000lic3-0000-1000-8000-00805f9b34fb';
export const CONFIG_CHAR_UUID  = '0000lic4-0000-1000-8000-00805f9b34fb';
export const INFO_CHAR_UUID    = '0000lic5-0000-1000-8000-00805f9b34fb';

export const BleContext = createContext({ bleState: {}, setBleState: () => {} });

let manager = null;

export function getBleManager() {
  if (!manager) {
    manager = new BleManager();
  }
  return manager;
}

/* ---- State classifier names (mirror firmware) ---- */
export const STATE_NAMES = {
  0: 'Healthy',
  1: 'Stressed',
  2: 'Bleaching',
  3: 'Recovering',
  4: 'Dormant',
};

export const STATE_COLORS = {
  0: '#2e7d32',   /* green   */
  1: '#f9a825',   /* amber   */
  2: '#c62828',   /* red     */
  3: '#1565c0',   /* blue    */
  4: '#6a1b9a',   /* purple  */
};

/* ---- Scan for advertising nodes ---- */
export async function scanForNodes(timeoutMs = 10000) {
  const m = getBleManager();
  const found = new Map();

  return new Promise((resolve) => {
    m.startDeviceScan([LICHENWATCH_SERVICE_UUID], null, (err, device) => {
      if (err) {
        console.warn('BLE scan error:', err);
        return;
      }
      if (device && !found.has(device.id)) {
        found.set(device.id, {
          id: device.id,
          name: device.name || 'Lichenwatch node',
          rssi: device.rssi,
        });
      }
    });

    setTimeout(() => {
      m.stopDeviceScan();
      resolve(Array.from(found.values()));
    }, timeoutMs);
  });
}

/* ---- Connect and read the status characteristic ---- */
export async function readNodeStatus(deviceId) {
  const m = getBleManager();
  const device = await m.connectToDevice(deviceId);
  await device.discoverAllServicesAndCharacteristics();

  const char = await device.readCharacteristicForService(
    LICHENWATCH_SERVICE_UUID,
    STATUS_CHAR_UUID,
  );

  /* The status is a packed binary structure (ble_status_t from firmware).
   * We decode it here. */
  const raw = base64ToBytes(char.value);
  return decodeStatus(raw);
}

/* ---- Decode the 28-byte status payload ---- */
export function decodeStatus(bytes) {
  if (!bytes || bytes.length < 24) return null;
  const view = new DataView(bytes.buffer, 0);
  return {
    fv_fm:      view.getFloat32(0, true),
    lndvi:      view.getFloat32(4, true),
    chl_index:  view.getFloat32(8, true),
    wetness:    view.getFloat32(12, true),
    class_state: bytes[16],
    confidence: bytes[17],
    battery_mv: view.getUint16(18, true),
    seq:        view.getUint16(20, true),
    uptime_s:   view.getUint32(22, true),
  };
}

/* ---- Write config (measurement + uplink interval) ---- */
export async function writeNodeConfig(deviceId, measureMin, uplinkMin) {
  const m = getBleManager();
  const device = await m.connectToDevice(deviceId);
  await device.discoverAllServicesAndCharacteristics();
  const payload = new Uint8Array([measureMin, uplinkMin]);
  await device.writeCharacteristicWithResponseForService(
    LICHENWATCH_SERVICE_UUID,
    CONFIG_CHAR_UUID,
    bytesToBase64(payload),
  );
}

/* ---- Bulk download: subscribe to bulk characteristic notifications ---- */
export async function bulkDownload(deviceId, onRecord, onDone) {
  const m = getBleManager();
  const device = await m.connectToDevice(deviceId);
  await device.discoverAllServicesAndCharacteristics();

  const records = [];
  const subscription = device.monitorCharacteristicForService(
    LICHENWATCH_SERVICE_UUID,
    BULK_CHAR_UUID,
    (err, char) => {
      if (err) {
        console.warn('Bulk read error:', err);
        return;
      }
      if (char && char.value) {
        const raw = base64ToBytes(char.value);
        if (raw.length === 1 && raw[0] === 0x45) {
          /* 'E' end marker */
          onDone(records);
          subscription.remove();
          return;
        }
        const rec = decodeFlashRecord(raw);
        records.push(rec);
        onRecord(rec, records.length);
      }
    },
  );

  /* Trigger the bulk request by writing 0x42 ('B') to the config char. */
  const trigger = new Uint8Array([0x42]);
  await device.writeCharacteristicWithResponseForService(
    LICHENWATCH_SERVICE_UUID,
    CONFIG_CHAR_UUID,
    bytesToBase64(trigger),
  );
}

/* ---- Decode a 64-byte flash record ---- */
export function decodeFlashRecord(bytes) {
  if (!bytes || bytes.length < 32) return null;
  const view = new DataView(bytes.buffer, 0);
  return {
    magic:      view.getUint32(0, true),
    seq:        view.getUint16(4, true),
    fv_fm:      view.getInt16(6, true) / 1000.0,
    lndvi:      view.getInt16(8, true) / 1000.0,
    chl_index:  view.getUint16(10, true) / 100.0,
    wetness:    view.getUint16(12, true) / 1000.0,
    co2:        view.getInt16(14, true),
    temp:       view.getInt8(16),
    rh:         view.getUint8(17),
    uv:         view.getUint8(18),
    clicks:     view.getUint16(19, true),
    bat_mv:     view.getUint16(21, true),
    flags:      bytes[23],
    class_state: bytes[24],
  };
}

/* ---- Persist downloaded records locally ---- */
export async function saveRecords(nodeId, records) {
  const key = `lichenwatch_records_${nodeId}`;
  await AsyncStorage.setItem(key, JSON.stringify(records));
}

export async function loadRecords(nodeId) {
  const key = `lichenwatch_records_${nodeId}`;
  const raw = await AsyncStorage.getItem(key);
  return raw ? JSON.parse(raw) : [];
}

/* ---- Base64 <-> bytes helpers ---- */
function base64ToBytes(b64) {
  const binary = atob(b64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function bytesToBase64(bytes) {
  let binary = '';
  for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
  return btoa(binary);
}