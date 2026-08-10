/**
 * @file    TideBandContext.js
 * @brief   React Context providing BLE connection state and device data
 *          to all screens in the TideBand companion app.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { BleManager } from 'react-native-ble-plx';
import {
  TIDEBAND_SERVICE_UUID,
  TIDEBAND_TX_CHAR_UUID,
  TIDEBAND_RX_CHAR_UUID,
  parsePacket,
  parseStatusPayload,
  parseProfilePayload,
  OP,
} from '../utils/protocol';

const TideBandContext = createContext(null);

export function useTideBand() {
  const ctx = useContext(TideBandContext);
  if (!ctx) {
    throw new Error('useTideBand must be used within TideBandProvider');
  }
  return ctx;
}

export function TideBandProvider({ children }) {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [status, setStatus] = useState(null);
  const [profileData, setProfileData] = useState([]);
  const [diveActive, setDiveActive] = useState(false);
  const [diveCount, setDiveCount] = useState(0);
  const [error, setError] = useState(null);
  const [units, setUnits] = useState('metric'); // 'metric' or 'imperial'
  const [sampleRate, setSampleRate] = useState(2);
  const [hapticThreshold, setHapticThreshold] = useState(0.5);

  // ---- Start scanning for TideBand devices ----
  const startScan = useCallback(() => {
    setScanning(true);
    setError(null);

    manager.startDeviceScan(null, null, (err, dev) => {
      if (err) {
        setError(err.message);
        setScanning(false);
        return;
      }

      if (dev.name && dev.name.includes('TideBand')) {
        manager.stopDeviceScan();
        setScanning(false);
        connectToDevice(dev);
      }
    });

    // Stop scan after 10 seconds
    setTimeout(() => {
      if (scanning) {
        manager.stopDeviceScan();
        setScanning(false);
      }
    }, 10000);
  }, [manager, scanning]);

  // ---- Connect to a discovered device ----
  const connectToDevice = useCallback(async (dev) => {
    try {
      const connectedDev = await dev.connect();
      await connectedDev.discoverAllServicesAndCharacteristics();
      setDevice(connectedDev);
      setConnected(true);
      setError(null);

      // Subscribe to TX characteristic (device → app)
      const txChar = await connectedDev.readCharacteristic(
        TIDEBAND_SERVICE_UUID,
        TIDEBAND_TX_CHAR_UUID
      );

      // Set up notification listener
      const subscription = txChar.monitor((err, characteristic) => {
        if (err) {
          setError(err.message);
          return;
        }

        if (characteristic && characteristic.value) {
          const pkt = parsePacket(characteristic.value);
          if (!pkt) return;

          handlePacket(pkt);
        }
      });

      // Request initial status
      sendCommand(OP.STATUS_REQ);
    } catch (err) {
      setError(err.message);
      setConnected(false);
    }
  }, [manager]);

  // ---- Handle incoming packets ----
  const handlePacket = useCallback((pkt) => {
    switch (pkt.opcode) {
      case OP.STATUS_RSP:
        const st = parseStatusPayload(pkt.payload);
        if (st) {
          setStatus(st);
          setDiveActive(st.diveActive);
          setDiveCount(st.diveCount);
        }
        break;

      case OP.PROFILE_DATA:
        const pd = parseProfilePayload(pkt.payload);
        if (pd) {
          setProfileData((prev) => {
            const next = [...prev, pd];
            // Keep last 500 samples to limit memory
            if (next.length > 500) {
              return next.slice(-500);
            }
            return next;
          });
        }
        break;

      case OP.DIVE_START:
        setDiveActive(true);
        break;

      case OP.DIVE_END:
        setDiveActive(false);
        break;

      case OP.INFO_RSP:
        // Device info response
        break;

      default:
        break;
    }
  }, []);

  // ---- Send a command to the device ----
  const sendCommand = useCallback(async (opcode, payload = []) => {
    if (!device || !connected) return;

    try {
      const { buildPacket } = require('../utils/protocol');
      const base64Pkt = buildPacket(opcode, payload);
      await device.writeCharacteristicWithResponseForService(
        TIDEBAND_SERVICE_UUID,
        TIDEBAND_RX_CHAR_UUID,
        base64Pkt
      );
    } catch (err) {
      setError(err.message);
    }
  }, [device, connected]);

  // ---- Set sample rate ----
  const setRate = useCallback((rate) => {
    setSampleRate(rate);
    sendCommand(OP.SET_RATE, [rate]);
  }, [sendCommand]);

  // ---- Set haptic threshold ----
  const setThreshold = useCallback((threshold) => {
    setHapticThreshold(threshold);
    const payload = new ArrayBuffer(4);
    const view = new DataView(payload);
    view.setFloat32(0, threshold, true);
    sendCommand(OP.SET_THRESHOLD, new Uint8Array(payload));
  }, [sendCommand]);

  // ---- Erase all dive data ----
  const eraseDives = useCallback(() => {
    sendCommand(OP.ERASE_DIVES);
    setDiveCount(0);
    setProfileData([]);
  }, [sendCommand]);

  // ---- Disconnect ----
  const disconnect = useCallback(async () => {
    if (device) {
      await device.cancelConnection();
      setDevice(null);
      setConnected(false);
      setStatus(null);
      setProfileData([]);
    }
  }, [device]);

  // ---- Cleanup on unmount ----
  useEffect(() => {
    return () => {
      if (device) {
        device.cancelConnection();
      }
      manager.destroy();
    };
  }, [device, manager]);

  // ---- Periodic status requests (every 5 seconds) ----
  useEffect(() => {
    if (!connected) return;
    const interval = setInterval(() => {
      sendCommand(OP.STATUS_REQ);
    }, 5000);
    return () => clearInterval(interval);
  }, [connected, sendCommand]);

  const value = {
    manager,
    device,
    connected,
    scanning,
    status,
    profileData,
    diveActive,
    diveCount,
    error,
    units,
    setUnits,
    sampleRate,
    hapticThreshold,
    startScan,
    disconnect,
    sendCommand,
    setRate,
    setThreshold,
    eraseDives,
  };

  return (
    <TideBandContext.Provider value={value}>
      {children}
    </TideBandContext.Provider>
  );
}