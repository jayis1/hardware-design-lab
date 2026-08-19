/**
 * protocol.js — BLE GATT protocol and flow-map tile decoder
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * Implements the BLE communication protocol with the SpeckleFlow device.
 * The device exposes a custom GATT service with three characteristics:
 *   - Flow Map Tile (0xFF01): 128-byte tile notifications
 *   - Status (0xFF02): 8-byte status updates
 *   - Command (0xFF03): 4-byte command writes
 *
 * The 640×480 flow map is divided into 40×60 = 2400 tiles of 16×8 pixels.
 * Each tile is sent as a 130-byte notification (2-byte index + 128-byte data).
 * The app reassembles tiles into a full frame and renders via Canvas.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import BleManager from 'react-native-ble-manager';

// ---- Constants -----------------------------------------------------------

export const SPECKLEFLOW_SERVICE_UUID = '0000ff00-0000-1000-8000-00805f9b34fb';
export const FLOW_TILE_CHAR_UUID       = '0000ff01-0000-1000-8000-00805f9b34fb';
export const STATUS_CHAR_UUID          = '0000ff02-0000-1000-8000-00805f9b34fb';
export const COMMAND_CHAR_UUID         = '0000ff03-0000-1000-8000-00805f9b34fb';

export const TILE_WIDTH = 16;
export const TILE_HEIGHT = 8;
export const TILES_X = 40;   // 640 / 16
export const TILES_Y = 60;   // 480 / 8
export const TILES_PER_FRAME = TILES_X * TILES_Y;  // 2400
export const TILE_DATA_BYTES = TILE_WIDTH * TILE_HEIGHT;  // 128

// Colormaps (RGB triplets for 256 entries)
export const COLORMAPS = {
  jet: generateJetColormap(),
  thermal: generateThermalColormap(),
  grayscale: generateGrayscaleColormap(),
  viridis: generateViridisColormap(),
  inferno: generateInfernoColormap(),
};

// Commands
export const CMD = {
  START_IMAGING:  0x01,
  STOP_IMAGING:   0x02,
  CALIBRATE:      0x03,
  SET_COLORMAP:   0x04,
  SET_WINDOW:     0x05,
  SET_FRAME_RATE: 0x06,
  SET_LASER_PWR:  0x07,
  SET_EXPOSURE:   0x08,
  SET_SD_LOG:     0x09,
  SET_BLE_STREAM: 0x0A,
  SET_ROI:        0x0B,
};

// ---- Colormap generators -------------------------------------------------

function generateJetColormap() {
  const lut = new Uint8ClampedArray(256 * 3);
  for (let i = 0; i < 256; i++) {
    const t = i / 255;
    let r = 1.5 - 4 * (t - 0.75) ** 2;
    let g = 1.0 - 4 * (t - 0.5) ** 2;
    let b = 1.5 - 4 * (t - 0.25) ** 2;
    lut[i * 3]     = Math.max(0, Math.min(255, r * 255));
    lut[i * 3 + 1] = Math.max(0, Math.min(255, g * 255));
    lut[i * 3 + 2] = Math.max(0, Math.min(255, b * 255));
  }
  return lut;
}

function generateThermalColormap() {
  const lut = new Uint8ClampedArray(256 * 3);
  for (let i = 0; i < 256; i++) {
    if (i < 64) {
      lut[i * 3] = i * 4; lut[i * 3 + 1] = 0; lut[i * 3 + 2] = 0;
    } else if (i < 128) {
      lut[i * 3] = 255; lut[i * 3 + 1] = (i - 64) * 4; lut[i * 3 + 2] = 0;
    } else if (i < 192) {
      lut[i * 3] = 255; lut[i * 3 + 1] = 255; lut[i * 3 + 2] = (i - 128) * 4;
    } else {
      lut[i * 3] = 255; lut[i * 3 + 1] = 255; lut[i * 3 + 2] = 255;
    }
  }
  return lut;
}

function generateGrayscaleColormap() {
  const lut = new Uint8ClampedArray(256 * 3);
  for (let i = 0; i < 256; i++) {
    lut[i * 3] = i; lut[i * 3 + 1] = i; lut[i * 3 + 2] = i;
  }
  return lut;
}

function generateViridisColormap() {
  const lut = new Uint8ClampedArray(256 * 3);
  for (let i = 0; i < 256; i++) {
    const t = i / 255;
    lut[i * 3]     = Math.min(255, (t * t * 0.9 + 0.07) * 255);
    lut[i * 3 + 1] = Math.min(255, (t * 0.5 + 0.1) * 255);
    lut[i * 3 + 2] = Math.min(255, (0.5 - t * 0.3 + 0.4) * 255);
  }
  return lut;
}

function generateInfernoColormap() {
  const lut = new Uint8ClampedArray(256 * 3);
  for (let i = 0; i < 256; i++) {
    const t = i / 255;
    lut[i * 3]     = Math.min(255, t * t * 255);
    lut[i * 3 + 1] = Math.min(255, t * t * t * 0.8 * 255);
    lut[i * 3 + 2] = Math.max(0, Math.min(255, t * 0.4 * (1 - t * 2) * 255));
  }
  return lut;
}

// ---- Frame assembler -----------------------------------------------------

export class FrameAssembler {
  constructor() {
    this.frame = new Uint8ClampedArray(640 * 480);
    this.tileCount = 0;
    this.frameNumber = 0;
    this.onFrameComplete = null;
  }

  processTile(data) {
    // data is a Uint8Array of 130 bytes: [idx_lo, idx_hi, ...128 bytes]
    if (data.length < 2 + TILE_DATA_BYTES) return;

    const tileIdx = data[0] | (data[1] << 8);
    if (tileIdx >= TILES_PER_FRAME) return;

    const tx = tileIdx % TILES_X;
    const ty = Math.floor(tileIdx / TILES_X);

    // Copy tile data into the frame buffer
    for (let dy = 0; dy < TILE_HEIGHT; dy++) {
      const frameY = ty * TILE_HEIGHT + dy;
      if (frameY >= 480) continue;
      for (let dx = 0; dx < TILE_WIDTH; dx++) {
        const frameX = tx * TILE_WIDTH + dx;
        if (frameX >= 640) continue;
        this.frame[frameY * 640 + frameX] = data[2 + dy * TILE_WIDTH + dx];
      }
    }

    this.tileCount++;
    if (this.tileCount >= TILES_PER_FRAME) {
      this.frameNumber++;
      this.tileCount = 0;
      if (this.onFrameComplete) {
        this.onFrameComplete(this.frame, this.frameNumber);
      }
    }
  }

  reset() {
    this.tileCount = 0;
    this.frameNumber = 0;
  }
}

// ---- Device context (React Context for BLE state) -----------------------

const DeviceContext = createContext(null);

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [deviceId, setDeviceId] = useState(null);
  const [status, setStatus] = useState({
    battery: 0, laserOn: false, fps: 0, tempC: 25, frameCount: 0,
  });
  const [colormap, setColormap] = useState('jet');
  const [windowSize, setWindowSize] = useState(1);  // 0=5, 1=7, 2=9
  const [fpsMode, setFpsMode] = useState(1);  // 0=30, 1=60, 2=120
  const [laserPower, setLaserPower] = useState(100);
  const [exposure, setExposure] = useState(5);  // ms
  const [sdLogging, setSdLogging] = useState(false);
  const [bleStreaming, setBleStreaming] = useState(true);
  const [roi, setRoi] = useState({ x: 80, y: 60, w: 160, h: 120 });

  const assemblerRef = useRef(new FrameAssembler());

  const connect = useCallback(async (id) => {
    try {
      await BleManager.connect(id);
      setDeviceId(id);
      setConnected(true);

      // Start notifications on flow tile and status characteristics
      await BleManager.startNotification(id, SPECKLEFLOW_SERVICE_UUID, FLOW_TILE_CHAR_UUID);
      await BleManager.startNotification(id, SPECKLEFLOW_SERVICE_UUID, STATUS_CHAR_UUID);

      // Set up notification handler
      BleManager.onDidUpdateValueForCharacteristic(({ characteristic, value }) => {
        if (characteristic === FLOW_TILE_CHAR_UUID) {
          const data = new Uint8Array(value);
          assemblerRef.current.processTile(data);
        } else if (characteristic === STATUS_CHAR_UUID) {
          if (value && value.length >= 8) {
            setStatus({
              battery: value[0],
              laserOn: value[1] !== 0,
              fps: value[2],
              tempC: value[3] | (value[3] > 127 ? -256 : 0),
              frameCount: value[4] | (value[5] << 8) | (value[6] << 16) | (value[7] << 24),
            });
          }
        }
      });
    } catch (error) {
      console.error('Connect failed:', error);
      setConnected(false);
    }
  }, []);

  const disconnect = useCallback(async () => {
    if (deviceId) {
      try {
        await BleManager.disconnect(deviceId);
      } catch (e) { /* ignore */ }
    }
    setConnected(false);
    setDeviceId(null);
  }, [deviceId]);

  const sendCommand = useCallback(async (cmd, p0 = 0, p1 = 0, p2 = 0) => {
    if (!deviceId) return;
    const data = [cmd, p0, p1, p2];
    try {
      await BleManager.write(deviceId, SPECKLEFLOW_SERVICE_UUID, COMMAND_CHAR_UUID, data);
    } catch (e) {
      console.error('Command failed:', e);
    }
  }, [deviceId]);

  const value = {
    connected, deviceId, status,
    colormap, setColormap,
    windowSize, setWindowSize,
    fpsMode, setFpsMode,
    laserPower, setLaserPower,
    exposure, setExposure,
    sdLogging, setSdLogging,
    bleStreaming, setBleStreaming,
    roi, setRoi,
    assembler: assemblerRef.current,
    connect, disconnect, sendCommand,
  };

  return <DeviceContext.Provider value={value}>{children}</DeviceContext.Provider>;
}

export function useDevice() {
  return useContext(DeviceContext);
}

// ---- BLE scanning --------------------------------------------------------

export async function requestBlePermissions() {
  if (Platform.OS === 'android') {
    const granted = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
    ]);
    return Object.values(granted).every(r => r === PermissionsAndroid.RESULTS.GRANTED);
  }
  return true;  // iOS handles via Info.plist
}

export async function scanForDevices(timeoutMs = 5000) {
  await BleManager.start({ showAlert: false });
  return new Promise((resolve) => {
    const found = new Map();
    BleManager.scan([SPECKLEFLOW_SERVICE_UUID], timeoutMs / 1000, true);
    BleManager.onDiscoverPeripheral((peripheral) => {
      if (peripheral.advertising) {
        found.set(peripheral.id, peripheral);
      }
    });
    setTimeout(() => {
      BleManager.stopScan();
      resolve(Array.from(found.values()));
    }, timeoutMs);
  });
}