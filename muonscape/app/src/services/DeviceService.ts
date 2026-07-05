// DeviceService.ts — BLE + Wi-Fi TCP interface to the MûonScape device
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT
//
// Provides a single API for the UI: connect over BLE, send JSON commands,
// receive JSON responses, subscribe to status notifications, and download
// binary event/image files over Wi-Fi TCP.
//
// BLE handles low-bandwidth control + status (port 7000 not needed).
// Wi-Fi TCP (port 7000) handles high-bandwidth image + event downloads.

import { BleManager, State as BleState } from 'react-native-ble-plx';
import TcpSocket from 'react-native-tcp-socket';
import { EventEmitter } from 'events';

// GATT service + characteristic UUIDs (match firmware/ble.h)
const SVC_UUID = '0000181a-0000-1000-8000-00805f9b34fb';
const CHAR_STATUS = '00002a6e-0000-1000-8000-00805f9b34fb';
const CHAR_CMD = '00002a6f-0000-1000-8000-00805f9b34fb';
const CHAR_TRACK = '00002a70-0000-1000-8000-00805f9b34fb';

export interface MuonStatus {
  ok: boolean;
  state: string;
  fw: string;
  author: string;
  tracks: number;
  events: number;
  elapsed_s: number;
  remain_s: number;
  soc: number;
  pack_mv: number;
  current_ma: number;
  temp_c: number;
  sipm_mv: number;
  coinc_ns: number;
  gps_fix: number;
  sats: number;
  lat: number;
  lon: number;
  roll: number;
  pitch: number;
  lux: number;
  fbp_tracks: number;
  fbp_max: number;
  rssi_dbm: number;
}

class DeviceService {
  private ble: BleManager;
  private connectedDevice: any = null;
  private tcpClient: any = null;
  private statusCallbacks: ((s: MuonStatus) => void)[] = [];
  private trackCallbacks: ((t: any) => void)[] = [];
  private wifiHost: string = '';
  private wifiPort: number = 7000;
  public events = new EventEmitter();

  constructor() {
    this.ble = new BleManager();
  }

  /** Scan for a MûonScape device advertising the service UUID, connect. */
  async scanAndConnect(timeoutMs = 10000): Promise<boolean> {
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.ble.stopDeviceScan();
        resolve(false);
      }, timeoutMs);

      this.ble.startDeviceScan([SVC_UUID], null, (err, device) => {
        if (err) { clearTimeout(timer); reject(err); return; }
        if (device && device.name && device.name.includes('MuonScape')) {
          this.ble.stopDeviceScan();
          clearTimeout(timer);
          device.connect()
            .then((d) => d.discoverAllServicesAndCharacteristics())
            .then((d) => {
              this.connectedDevice = d;
              this.setupNotifications();
              resolve(true);
            })
            .catch((e) => resolve(false));
        }
      });
    });
  }

  /** Set up BLE notifications for status and track characteristics. */
  private setupNotifications() {
    if (!this.connectedDevice) return;
    this.connectedDevice.monitorCharacteristicForService(
      SVC_UUID, CHAR_STATUS,
      (_err: any, char: any) => {
        if (char && char.value) {
          const json = Buffer.from(char.value, 'base64').toString('utf-8');
          try {
            const status = JSON.parse(json) as MuonStatus;
            this.statusCallbacks.forEach((cb) => cb(status));
            this.events.emit('status', status);
          } catch (e) { /* ignore parse error */ }
        }
      }
    );
    this.connectedDevice.monitorCharacteristicForService(
      SVC_UUID, CHAR_TRACK,
      (_err: any, char: any) => {
        if (char && char.value) {
          const json = Buffer.from(char.value, 'base64').toString('utf-8');
          try {
            const track = JSON.parse(json);
            this.trackCallbacks.forEach((cb) => cb(track));
          } catch (e) { /* ignore */ }
        }
      }
    );
  }

  /** Send a JSON command over BLE and return the response JSON. */
  async sendCommand(cmd: object): Promise<any> {
    if (!this.connectedDevice) throw new Error('Not connected');
    const json = JSON.stringify(cmd);
    const bytes = Buffer.from(json, 'utf-8').toString('base64');
    await this.connectedDevice.writeCharacteristicWithResponseForService(
      SVC_UUID, CHAR_CMD, bytes
    );
    // Response comes back via the status notification; we wait for it
    return new Promise((resolve) => {
      const handler = (status: any) => {
        this.events.removeListener('status', handler);
        resolve(status);
      };
      this.events.once('status', handler);
    });
  }

  /** Connect over Wi-Fi TCP for high-bandwidth downloads. */
  async connectWifi(host: string, port: number = 7000): Promise<void> {
    return new Promise((resolve, reject) => {
      this.wifiHost = host;
      this.wifiPort = port;
      this.tcpClient = TcpSocket.createConnection(
        { host, port },
        () => resolve()
      );
      this.tcpClient.on('error', (err: any) => reject(err));
    });
  }

  /** Download a binary file (event log or image) over TCP. */
  async downloadFile(path: string): Promise<Uint8Array> {
    if (!this.tcpClient) throw new Error('Wi-Fi not connected');
    return new Promise((resolve, reject) => {
      const chunks: Buffer[] = [];
      this.tcpClient.write(JSON.stringify({ cmd: 'download', path }) + '\n');
      this.tcpClient.on('data', (data: Buffer) => {
        chunks.push(data);
        if (data.toString().includes('"done":true')) {
          resolve(new Uint8Array(Buffer.concat(chunks)));
        }
      });
      this.tcpClient.on('error', (e: any) => reject(e));
    });
  }

  /** Subscribe to status updates. Returns an unsubscribe function. */
  onStatus(cb: (s: MuonStatus) => void): () => void {
    this.statusCallbacks.push(cb);
    return () => {
      this.statusCallbacks = this.statusCallbacks.filter((c) => c !== cb);
    };
  }

  onTrack(cb: (t: any) => void): () => void {
    this.trackCallbacks.push(cb);
    return () => {
      this.trackCallbacks = this.trackCallbacks.filter((c) => c !== cb);
    };
  }

  /** Convenience: send 'start' command. */
  async startAcquisition(durationSec: number, target: string, distanceCm: number): Promise<any> {
    return this.sendCommand({
      cmd: 'start',
      duration: durationSec,
      target,
      distance_cm: distanceCm,
    });
  }

  async stopAcquisition(): Promise<any> {
    return this.sendCommand({ cmd: 'stop' });
  }

  async startCalibration(): Promise<any> {
    return this.sendCommand({ cmd: 'calibrate' });
  }

  async getStatus(): Promise<MuonStatus> {
    return this.sendCommand({ cmd: 'status' });
  }

  async getPreview(slice: number): Promise<any> {
    return this.sendCommand({ cmd: 'preview', slice });
  }

  async setConfig(sipmMv: number, coincNs: number): Promise<any> {
    return this.sendCommand({ cmd: 'config', sipm_mv: sipmMv, coinc_ns: coincNs });
  }

  async sleep(): Promise<any> {
    return this.sendCommand({ cmd: 'sleep' });
  }

  async setWifi(ssid: string, pass: string): Promise<any> {
    return this.sendCommand({ cmd: 'wifi', ssid, pass });
  }

  isBleConnected(): boolean {
    return this.connectedDevice !== null;
  }

  isWifiConnected(): boolean {
    return this.tcpClient !== null;
  }

  disconnect() {
    if (this.connectedDevice) {
      this.connectedDevice.cancelConnection();
      this.connectedDevice = null;
    }
    if (this.tcpClient) {
      this.tcpClient.destroy();
      this.tcpClient = null;
    }
  }
}

export default new DeviceService();