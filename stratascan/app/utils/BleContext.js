// BleContext.js — BLE Connection Manager for StrataScan
// StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
//
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// SPDX-License-Identifier: MIT
//
// Provides a React context that manages the BLE connection to the StrataScan
// device.  Handles scanning, connection, command sending, and incoming data
// notification parsing.  Exposes the connection state, device status, and
// B-scan data buffer to all screens via a useBle() hook.
//

import React, { createContext, useContext, useState, useRef, useCallback, useEffect } from 'react';
import { PermissionsAndroid, Platform, NativeModules } from 'react-native';

// Protocol helpers
import { CMD, buildFrame, parseFrame, parseStatus, parseBscanRow, BAND_PRESETS } from './protocol';

// StrataScan BLE service UUIDs
const SERVICE_UUID = '0000fff0-0000-1000-8000-00805f9b34fb';
const CMD_CHAR_UUID = '0000fff1-0000-1000-8000-00805f9b34fb';
const DATA_CHAR_UUID = '0000fff2-0000-1000-8000-00805f9b34fb';
const STATUS_CHAR_UUID = '0000fff3-0000-1000-8000-00805f9b34fb';

const BleContext = createContext(null);

export function BleProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [deviceName, setDeviceName] = useState('');
  const [status, setStatus] = useState(null);
  const [bscanBuffer, setBscanBuffer] = useState({ traces: [], depths: [], maxTraces: 4096 });
  const [error, setError] = useState('');
  const [commandQueue, setCommandQueue] = useState([]);

  // Refs for BLE manager and characteristics
  const bleManagerRef = useRef(null);
  const cmdCharRef = useRef(null);
  const dataCharRef = useRef(null);
  const rxBufferRef = useRef([]);

  // Request Android BLE permissions
  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      const granted = await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      ]);
      return Object.values(granted).every(r => r === 'granted');
    }
    return true;
  };

  // Connect to the StrataScan device
  const connect = useCallback(async () => {
    setError('');
    setConnecting(true);
    try {
      await requestPermissions();
      // In a real implementation, this uses react-native-ble-plx:
      //   const { BleManager } = NativeModules;
      //   bleManagerRef.current = new BleManager();
      //   const device = await bleManagerRef.current.connectToDevice(SERVICE_UUID);
      //   await device.discoverAllServicesAndCharacteristics();
      //   cmdCharRef.current = device.writeCharacteristicWithResponseForService(...);
      //   dataCharRef.current = device.monitorCharacteristicForService(...);
      //
      // For this standalone design, we simulate the connection.
      setConnected(true);
      setDeviceName('StrataScan-001');
      setConnecting(false);
    } catch (err) {
      setError(`Connection failed: ${err.message}`);
      setConnecting(false);
    }
  }, []);

  // Disconnect
  const disconnect = useCallback(async () => {
    // if (bleManagerRef.current) { await bleManagerRef.current.cancelDeviceConnection(); }
    setConnected(false);
    setStatus(null);
    setBscanBuffer({ traces: [], depths: [], maxTraces: 4096 });
  }, []);

  // Send a command to the device
  const sendCommand = useCallback((opcode, payload = []) => {
    const frame = buildFrame(opcode, payload);
    // In real impl: cmdCharRef.current.writeWithResponse(frame);
    // For simulation, we update status based on command
    if (opcode === CMD.GET_STATUS) {
      // Simulate a status response
      const mockStatus = {
        state: 1, bandIdx: 2, tracesAcquired: 0, batteryPct: 85,
        calibrated: true, bscanTraces: 0, lat: 37.7749, lon: -122.4194,
        stateName: 'IDLE', bandName: 'MED',
      };
      setStatus(mockStatus);
    }
  }, [connected]);

  // Process incoming BLE data (called from notification handler)
  const handleIncomingData = useCallback((data) => {
    // Append to RX buffer
    rxBufferRef.current = rxBufferRef.current.concat(Array.from(data));

    // Try to parse complete frames
    let parsed;
    while ((parsed = parseFrame(rxBufferRef.current))) {
      const { opcode, payload } = parsed;
      const frameLen = 5 + payload.length + 2;
      rxBufferRef.current = rxBufferRef.current.slice(frameLen);

      if (opcode === CMD.STATUS_RESP) {
        const st = parseStatus(Array.from(payload));
        if (st) setStatus(st);
      } else if (opcode === CMD.BSCAN_ROW) {
        const row = parseBscanRow(Array.from(payload));
        if (row) {
          setBscanBuffer(prev => {
            const traces = [...prev.traces];
            traces[row.row] = row.trace;
            const depths = row.depth.length > 0 ? row.depth : prev.depths;
            return { ...prev, traces, depths };
          });
        }
      } else if (opcode === CMD.TRACE_READY) {
        // New trace available — request it
        sendCommand(CMD.GET_BSCAN_ROW);
      }
    }
  }, [sendCommand]);

  // Auto-request status every 2 seconds when connected
  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(() => {
      sendCommand(CMD.GET_STATUS);
    }, 2000);
    return () => clearInterval(interval);
  }, [connected, sendCommand]);

  // Start survey
  const startSurvey = useCallback(() => sendCommand(CMD.START_SURVEY), [sendCommand]);
  const pauseSurvey = useCallback(() => sendCommand(CMD.PAUSE_SURVEY), [sendCommand]);
  const stopSurvey = useCallback(() => sendCommand(CMD.STOP_SURVEY), [sendCommand]);
  const recalibrate = useCallback(() => sendCommand(CMD.RECALIBRATE), [sendCommand]);
  const setBand = useCallback((bandId) => sendCommand(CMD.SET_BAND, [bandId]), [sendCommand]);
  const setEpsR = useCallback((epsR) => {
    const raw = Math.round(epsR * 100);
    const payload = [raw & 0xFF, (raw >> 8) & 0xFF, (raw >> 16) & 0xFF, (raw >> 24) & 0xFF];
    sendCommand(CMD.SET_EPS_R, payload);
  }, [sendCommand]);
  const setSpacing = useCallback((mm) => {
    const raw = Math.round(mm * 10);
    const payload = [raw & 0xFF, (raw >> 8) & 0xFF, (raw >> 16) & 0xFF, (raw >> 24) & 0xFF];
    sendCommand(CMD.SET_SPACING, payload);
  }, [sendCommand]);

  const value = {
    connected,
    connecting,
    deviceName,
    status,
    error,
    bscanBuffer,
    connect,
    disconnect,
    sendCommand,
    startSurvey,
    pauseSurvey,
    stopSurvey,
    recalibrate,
    setBand,
    setEpsR,
    setSpacing,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
}