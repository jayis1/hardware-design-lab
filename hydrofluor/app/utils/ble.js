// utils/ble.js — HydroFluor BLE connection manager
// Author: jayis1  License: MIT
//
// Wraps react-native-ble-plx into a small HydroFluor-specific connection
// manager: scans for the HydroFluor service UUID, connects, enables
// notifications on the sample characteristic, and exposes a simple
// subscribe API for screens.
import { BleManager } from 'react-native-ble-plx';
import { UUIDS, parseSample, parseDeviceInfo, encodeCommand, Commands } from './protocol';

const HF_SERVICE = UUIDS.SERVICE_CONTROL;

class HydroFluorBLE {
  constructor() {
    this.manager = new BleManager();
    this.device  = null;
    this.subscribers = new Set();
    this.statusSubscribers = new Set();
    this.connected = false;
  }

  async requestPermissions() {
    // On Android, location permission is required for BLE scanning.
    // The Expo Permissions API is delegated to the platform shell.
    return true;
  }

  async scan(timeoutMs = 5000) {
    await this.requestPermissions();
    return new Promise((resolve, reject) => {
      const found = [];
      this.manager.startDeviceScan([HF_SERVICE], null, (err, dev) => {
        if (err) { reject(err); return; }
        if (dev && !found.find(d => d.id === dev.id)) {
          found.push({ id: dev.id, name: dev.name || 'HydroFluor', rssi: dev.rssi });
        }
      });
      setTimeout(() => {
        this.manager.stopDeviceScan();
        resolve(found);
      }, timeoutMs);
    });
  }

  async connect(deviceId) {
    this.device = await this.manager.connectToDevice(deviceId);
    await this.device.discoverAllServicesAndCharacteristics();
    this.connected = true;
    // Subscribe to sample notifications
    await this.device.monitorCharacteristicForService(
      UUIDS.SERVICE_DATA, UUIDS.CHAR_SAMPLE,
      (err, char) => {
        if (err || !char?.value) return;
        const buf = base64ToArrayBuffer(char.value);
        const rec = parseSample(buf);
        if (rec) this.subscribers.forEach(cb => cb(rec));
      }
    );
    // Subscribe to status notifications
    await this.device.monitorCharacteristicForService(
      UUIDS.SERVICE_CONTROL, UUIDS.CHAR_STATUS,
      (err, char) => {
        if (err || !char?.value) return;
        const buf = base64ToArrayBuffer(char.value);
        const status = parseStatus(buf);
        this.statusSubscribers.forEach(cb => cb(status));
      }
    );
    return this.device;
  }

  async sendCommand(cmdStr) {
    if (!this.device) throw new Error('not connected');
    const data = base64FromBytes(encodeCommand(cmdStr));
    await this.device.writeCharacteristicWithResponseForService(
      UUIDS.SERVICE_CONTROL, UUIDS.CHAR_CMD, data);
  }

  async startSurvey()        { return this.sendCommand(Commands.START); }
  async stopSurvey()         { return this.sendCommand(Commands.STOP); }
  async pumpOn()             { return this.sendCommand(Commands.PUMP_ON); }
  async pumpOff()            { return this.sendCommand(Commands.PUMP_OFF); }
  async setPeriodMs(ms)     { return this.sendCommand(Commands.PERIOD(ms)); }
  async calibrate(a, ref)   { return this.sendCommand(Commands.CAL(a, ref)); }

  async readDeviceInfo() {
    if (!this.device) return null;
    const c = await this.device.readCharacteristicForService(
      UUIDS.SERVICE_INFO, UUIDS.CHAR_INFO);
    return parseDeviceInfo(base64ToString(c.value));
  }

  subscribeSamples(cb) {
    this.subscribers.add(cb);
    return () => this.subscribers.delete(cb);
  }
  subscribeStatus(cb) {
    this.statusSubscribers.add(cb);
    return () => this.statusSubscribers.delete(cb);
  }

  async disconnect() {
    if (this.device) {
      await this.device.cancelConnection();
      this.connected = false;
      this.device = null;
    }
  }

  isConnected() { return this.connected; }
}

// ---- helpers ----
// react-native doesn't ship atob/btoa globally; provide minimal base64 codecs.
function b64Lookup(c) {
  if (c >= 65 && c <= 90)  return c - 65;
  if (c >= 97 && c <= 122) return c - 71;
  if (c >= 48 && c <= 57)  return c + 4;
  if (c === 43) return 62;
  if (c === 47) return 63;
  return 0;
}
function base64ToArrayBuffer(b64) {
  const s = (b64 || '').replace(/[^A-Za-z0-9+/]/g, '');
  const len = s.length * 3 / 4 | 0;
  const out = new Uint8Array(len);
  for (let i = 0, j = 0; i < s.length; i += 4) {
    const a = b64Lookup(s.charCodeAt(i));
    const b = b64Lookup(s.charCodeAt(i+1));
    const c = b64Lookup(s.charCodeAt(i+2));
    const d = b64Lookup(s.charCodeAt(i+3));
    const n = (a << 18) | (b << 12) | (c << 6) | d;
    if (j < len) out[j++] = (n >> 16) & 255;
    if (j < len) out[j++] = (n >> 8)  & 255;
    if (j < len) out[j++] =  n        & 255;
  }
  return out.buffer;
}
function base64FromBytes(bytes) {
  const tbl = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let out = '';
  for (let i = 0; i < bytes.length; i += 3) {
    const b0 = bytes[i], b1 = bytes[i+1] || 0, b2 = bytes[i+2] || 0;
    out += tbl[(b0 >> 2) & 63];
    out += tbl[((b0 & 3) << 4) | ((b1 >> 4) & 15)];
    out += (i + 1 < bytes.length) ? tbl[((b1 & 15) << 2) | ((b2 >> 6) & 3)] : '=';
    out += (i + 2 < bytes.length) ? tbl[b2 & 63] : '=';
  }
  return out;
}
function base64ToString(b64) {
  const buf = base64ToArrayBuffer(b64);
  return String.fromCharCode.apply(null, new Uint8Array(buf));
}
function parseStatus(buf) {
  const dv = new DataView(buf);
  return {
    state:       dv.getUint8(0),
    batteryMv:   dv.getUint16(1, true),
    depthCm:     dv.getUint16(3, true),
    tempC01:     dv.getInt16(5, true),
    errorFlags:  dv.getUint16(7, true),
  };
}

export const ble = new HydroFluorBLE();
export default ble;