/**
 * BleContext.js — BLE Communication Context for RootTrace
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Manages the BLE connection to the RootTrace device, including
 * command/response framing, CRC-16 verification, and frame chunking.
 */

import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';

// BLE service and characteristic UUIDs
const ROOTTRACE_SERVICE_UUID = '0000a1b2-0000-1000-8000-00805f9b34fb';
const ROOTTRACE_CMD_CHAR_UUID = '0000a1b3-0000-1000-8000-00805f9b34fb';
const ROOTTRACE_RESP_CHAR_UUID = '0000a1b4-0000-1000-8000-00805f9b34fb';
const ROOTTRACE_FRAME_CHAR_UUID = '0000a1b5-0000-1000-8000-00805f9b34fb';

// Command opcodes (must match firmware)
const BLE_CMD_PING = 0x01;
const BLE_CMD_START_SCAN = 0x02;
const BLE_CMD_STOP_SCAN = 0x03;
const BLE_CMD_GET_FRAME = 0x04;
const BLE_CMD_GET_STATUS = 0x05;
const BLE_CMD_GET_CALIB = 0x06;
const BLE_CMD_SET_CALIB = 0x07;
const BLE_CMD_SET_FREQ = 0x08;
const BLE_CMD_SET_CURRENT = 0x09;
const BLE_CMD_GET_ENV = 0x0A;
const BLE_CMD_ERASE_LOGS = 0x0B;
const BLE_CMD_GET_FILE_LIST = 0x0C;
const BLE_CMD_GET_FILE = 0x0D;
const BLE_CMD_SELF_TEST = 0x0E;
const BLE_CMD_SET_MESH = 0x0F;
const BLE_CMD_GET_BATT = 0x10;
const BLE_CMD_ENTER_DFU = 0x11;

// Packet structure constants
const PKT_START = 0xAA;
const PKT_END = 0x55;
const PKT_MAX_PAYLOAD = 240;
const PKT_HEADER_SIZE = 6;
const PKT_CRC_SIZE = 2;
const PKT_TRAILER_SIZE = 1;

// CRC-16 CCITT
function crc16Update(crc, data) {
  crc ^= data << 8;
  for (let i = 0; i < 8; i++) {
    if (crc & 0x8000) {
      crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
    } else {
      crc = (crc << 1) & 0xFFFF;
    }
  }
  return crc;
}

function crc16Compute(data) {
  let crc = 0xFFFF;
  for (let i = 0; i < data.length; i++) {
    crc = crc16Update(crc, data[i]);
  }
  return crc;
}

// Convert a Uint8Array to a hex string
function bytesToHex(bytes) {
  return Array.from(bytes).map(b => b.toString(16).padStart(2, '0')).join('');
}

// Build a command packet
function buildPacket(opcode, payload) {
  const len = payload ? payload.length : 0;
  const totalSize = PKT_HEADER_SIZE + len + PKT_CRC_SIZE + PKT_TRAILER_SIZE;
  const packet = new Uint8Array(totalSize);
  let idx = 0;

  packet[idx++] = PKT_START;
  packet[idx++] = opcode;
  packet[idx++] = 0; // status (command)
  packet[idx++] = 0; // sequence (filled by firmware)
  packet[idx++] = len & 0xFF;
  packet[idx++] = (len >> 8) & 0xFF;

  if (payload) {
    for (let i = 0; i < payload.length; i++) {
      packet[idx++] = payload[i];
    }
  }

  // CRC over header + payload
  const crcData = packet.slice(0, idx);
  const crc = crc16Compute(crcData);
  packet[idx++] = crc & 0xFF;
  packet[idx++] = (crc >> 8) & 0xFF;
  packet[idx++] = PKT_END;

  return packet;
}

// Parse a response packet
function parsePacket(data) {
  if (data.length < 10) return null;
  if (data[0] !== PKT_START || data[data.length - 1] !== PKT_END) return null;

  const opcode = data[1];
  const status = data[2];
  const seq = data[3];
  const len = data[4] | (data[5] << 8);
  const payload = data.slice(PKT_HEADER_SIZE, PKT_HEADER_SIZE + len);
  const expectedCrc = data[PKT_HEADER_SIZE + len] | (data[PKT_HEADER_SIZE + len + 1] << 8);
  const computedCrc = crc16Compute(data.slice(0, PKT_HEADER_SIZE + len));

  if (expectedCrc !== computedCrc) {
    console.warn('CRC mismatch in response packet');
    return null;
  }

  return { opcode, status, seq, payload };
}

// React context for BLE
const BleContext = createContext(null);

export function BleProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [device, setDevice] = useState(null);
  const [status, setStatus] = useState({
    state: 0,
    scanning: false,
    freqIndex: 0,
    sdOk: false,
    frameSeq: 0,
    frameStatus: 0,
    contactMask: 0,
    batteryPct: 0,
    voltageMv: 0,
    charging: false,
    usbConnected: false,
  });
  const [envData, setEnvData] = useState({
    soilMoisture: 0,
    soilTemp: 0,
    ambientTemp: 0,
    ambientRh: 0,
    rtcTime: 0,
  });
  const [frameData, setFrameData] = useState(null);
  const [frameBuffer, setFrameBuffer] = useState([]);

  const bleManager = useRef(null);
  const responseQueue = useRef([]);
  const frameChunkBuffer = useRef([]);

  // Initialize BLE manager
  const initBle = useCallback(() => {
    if (!bleManager.current) {
      bleManager.current = new BleManager();
    }
  }, []);

  // Scan for RootTrace devices
  const scanForDevices = useCallback(async () => {
    initBle();
    setScanning(true);
    try {
      bleManager.current.startDeviceScan([ROOTTRACE_SERVICE_UUID], null, (error, dev) => {
        if (error) {
          console.error('BLE scan error:', error);
          setScanning(false);
          return;
        }
        if (dev && dev.name && dev.name.includes('RootTrace')) {
          bleManager.current.stopDeviceScan();
          setDevice(dev);
          setScanning(false);
        }
      });
    } catch (e) {
      console.error('Scan failed:', e);
      setScanning(false);
    }
  }, [initBle]);

  // Connect to a device
  const connectDevice = useCallback(async (dev) => {
    initBle();
    try {
      const connectedDev = await bleManager.current.connectToDevice(dev.id);
      await connectedDev.discoverAllServicesAndCharacteristics();

      // Setup notification listener for responses
      connectedDev.monitorCharacteristicForService(
        ROOTTRACE_SERVICE_UUID,
        ROOTTRACE_RESP_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Notification error:', error);
            return;
          }
          if (characteristic && characteristic.value) {
            const data = base64ToBytes(characteristic.value);
            const packet = parsePacket(data);
            if (packet) {
              handleResponse(packet);
            }
          }
        }
      );

      // Setup notification listener for frame data
      connectedDev.monitorCharacteristicForService(
        ROOTTRACE_SERVICE_UUID,
        ROOTTRACE_FRAME_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            console.error('Frame notification error:', error);
            return;
          }
          if (characteristic && characteristic.value) {
            const data = base64ToBytes(characteristic.value);
            handleFrameChunk(data);
          }
        }
      );

      setConnected(true);
      setDevice(connectedDev);

      // Send ping to verify connection
      await sendCommand(BLE_CMD_PING, null);
    } catch (e) {
      console.error('Connection failed:', e);
      setConnected(false);
    }
  }, [initBle]);

  // Disconnect
  const disconnect = useCallback(async () => {
    if (device && bleManager.current) {
      try {
        await bleManager.current.cancelDeviceConnection(device.id);
      } catch (e) {
        console.error('Disconnect error:', e);
      }
    }
    setConnected(false);
    setDevice(null);
  }, [device]);

  // Send a command
  const sendCommand = useCallback(async (opcode, payload) => {
    if (!device) return;
    const packet = buildPacket(opcode, payload || new Uint8Array(0));
    const base64 = bytesToBase64(packet);
    try {
      await device.writeCharacteristicWithResponseForService(
        ROOTTRACE_SERVICE_UUID,
        ROOTTRACE_CMD_CHAR_UUID,
        base64
      );
    } catch (e) {
      console.error('Send command failed:', e);
    }
  }, [device]);

  // Handle incoming response packet
  const handleResponse = useCallback((packet) => {
    switch (packet.opcode) {
      case BLE_CMD_PING:
        console.log('Ping response received, device connected');
        break;
      case BLE_CMD_GET_STATUS:
        if (packet.payload.length >= 16) {
          setStatus({
            state: packet.payload[0],
            scanning: packet.payload[1],
            freqIndex: packet.payload[2],
            sdOk: packet.payload[3],
            frameSeq: packet.payload[4] | (packet.payload[5] << 8),
            frameStatus: packet.payload[6] | (packet.payload[7] << 8),
            contactMask: packet.payload[8] | (packet.payload[9] << 8),
            batteryPct: packet.payload[10] | (packet.payload[11] << 8),
            voltageMv: packet.payload[12] | (packet.payload[13] << 8),
            charging: packet.payload[14],
            usbConnected: packet.payload[15],
          });
        }
        break;
      case BLE_CMD_GET_ENV:
        if (packet.payload.length >= 20) {
          const view = new DataView(packet.payload.buffer);
          setEnvData({
            soilMoisture: view.getFloat32(0, true),
            soilTemp: view.getFloat32(4, true),
            ambientTemp: view.getFloat32(8, true),
            ambientRh: view.getFloat32(12, true),
            rtcTime: view.getFloat32(16, true),
          });
        }
        break;
      case BLE_CMD_GET_BATT:
        if (packet.payload.length >= 8) {
          setStatus(prev => ({
            ...prev,
            voltageMv: packet.payload[0] | (packet.payload[1] << 8),
            batteryPct: packet.payload[2],
            charging: packet.payload[4],
            usbConnected: packet.payload[5],
          }));
        }
        break;
      case BLE_CMD_GET_FILE_LIST:
        if (packet.payload.length >= 2) {
          const count = packet.payload[0] | (packet.payload[1] << 8);
          console.log(`SD card has ${count} files`);
        }
        break;
      case BLE_CMD_SELF_TEST:
        console.log('Self-test result:', packet.status === 0 ? 'PASS' : 'FAIL');
        break;
      default:
        break;
    }
  }, []);

  // Handle incoming frame chunk
  const handleFrameChunk = useCallback((data) => {
    if (data.length < 2) return;
    const chunkType = data[0];
    if (chunkType === 0x00) {
      // Frame header
      const view = new DataView(data.buffer, 0, Math.min(data.length, 16));
      const frameSeq = view.getUint16(1, true);
      const timestamp = view.getUint32(3, true);
      const freqIndex = data[7];
      const frameStatus = data[8];
      const contactMask = view.getUint16(9, true);
      console.log(`Frame ${frameSeq}, timestamp ${timestamp}, freq ${freqIndex}`);
      frameChunkBuffer.current = [];
      setFrameData(prev => ({
        ...prev,
        frameSeq,
        timestamp,
        freqIndex,
        frameStatus,
        contactMask,
      }));
    } else if (chunkType === 0x01) {
      // Measurement data chunk
      frameChunkBuffer.current.push(data.slice(1));
    } else if (chunkType === 0x02) {
      // Conductivity data chunk
      frameChunkBuffer.current.push(data.slice(1));
    } else if (chunkType === 0x03) {
      // Image data (one row of 64x64 image)
      const rowIdx = data[1];
      const rowData = data.slice(2);
      setFrameData(prev => {
        const image = prev ? [...prev.image] : new Array(64).fill(new Array(64).fill(0));
        image[rowIdx] = Array.from(rowData);
        return { ...prev, image };
      });
    } else if (chunkType === 0xFF) {
      // End of frame
      const allChunks = frameChunkBuffer.current;
      // Process chunks into measurements and conductivity arrays
      const totalBytes = allChunks.reduce((sum, c) => sum + c.length, 0);
      const fullBuffer = new Uint8Array(totalBytes);
      let offset = 0;
      for (const chunk of allChunks) {
        fullBuffer.set(chunk, offset);
        offset += chunk.length;
      }
      setFrameData(prev => ({
        ...prev,
        rawMeasurements: fullBuffer.slice(0, 208 * 20),
        conductivity: fullBuffer.slice(208 * 20),
        complete: true,
      }));
      frameChunkBuffer.current = [];
    }
  }, []);

  // High-level commands
  const startScan = useCallback((freqIndex = 0) => {
    const payload = new Uint8Array([freqIndex]);
    return sendCommand(BLE_CMD_START_SCAN, payload);
  }, [sendCommand]);

  const stopScan = useCallback(() => {
    return sendCommand(BLE_CMD_STOP_SCAN);
  }, [sendCommand]);

  const requestFrame = useCallback(() => {
    return sendCommand(BLE_CMD_GET_FRAME);
  }, [sendCommand]);

  const requestStatus = useCallback(() => {
    return sendCommand(BLE_CMD_GET_STATUS);
  }, [sendCommand]);

  const requestEnv = useCallback(() => {
    return sendCommand(BLE_CMD_GET_ENV);
  }, [sendCommand]);

  const requestBattery = useCallback(() => {
    return sendCommand(BLE_CMD_GET_BATT);
  }, [sendCommand]);

  const setFrequency = useCallback((freqIndex) => {
    const payload = new Uint8Array([freqIndex]);
    return sendCommand(BLE_CMD_SET_FREQ, payload);
  }, [sendCommand]);

  const setCurrent = useCallback((currentUa) => {
    const payload = new Uint8Array([currentUa & 0xFF, (currentUa >> 8) & 0xFF]);
    return sendCommand(BLE_CMD_SET_CURRENT, payload);
  }, [sendCommand]);

  const runSelfTest = useCallback(() => {
    return sendCommand(BLE_CMD_SELF_TEST);
  }, [sendCommand]);

  const getFileList = useCallback(() => {
    return sendCommand(BLE_CMD_GET_FILE_LIST);
  }, [sendCommand]);

  const eraseLogs = useCallback(() => {
    return sendCommand(BLE_CMD_ERASE_LOGS);
  }, [sendCommand]);

  const enterDfu = useCallback(() => {
    return sendCommand(BLE_CMD_ENTER_DFU);
  }, [sendCommand]);

  // Utility: base64 encode/decode for BLE
  function bytesToBase64(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return btoa(binary);
  }

  function base64ToBytes(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
  }

  const value = {
    connected,
    scanning,
    device,
    status,
    envData,
    frameData,
    scanForDevices,
    connectDevice,
    disconnect,
    startScan,
    stopScan,
    requestFrame,
    requestStatus,
    requestEnv,
    requestBattery,
    setFrequency,
    setCurrent,
    runSelfTest,
    getFileList,
    eraseLogs,
    enterDfu,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) {
    throw new Error('useBle must be used within BleProvider');
  }
  return ctx;
}