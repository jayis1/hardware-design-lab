// ============================================================
// LignoScan App — BLE Communication Context
// Manages BLE connection state and data streaming from scanner
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT
// ============================================================

import React, { createContext, useState, useContext, useEffect } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { decode } from 'base-64';
import { LIGNOSCAN_SERVICE, parseStatusPacket, parseTofMatrix, parseTomogram, parseGpsData } from './protocol';

const BleContext = createContext(null);

export const BleProvider = ({ children }) => {
  const [manager] = useState(() => new BleManager());
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [scanStatus, setScanStatus] = useState({ state: 0, progress: 0 });
  const [tofMatrix, setTofMatrix] = useState(null);
  const [tomogram, setTomogram] = useState(null);
  const [gpsData, setGpsData] = useState(null);
  const [batteryLevel, setBatteryLevel] = useState(100);
  const [sensorCount, setSensorCount] = useState(12);
  const [error, setError] = useState(null);

  // Start scanning for LignoScan devices
  const startScan = useCallback(() => {
    if (scanning) return;
    setScanning(true);
    setError(null);

    manager.startDeviceScan(null, null, (scanError, scannedDevice) => {
      if (scanError) {
        setError(scanError.message);
        setScanning(false);
        return;
      }

      // Look for devices advertising the LignoScan service
      if (scannedDevice.name && scannedDevice.name.includes('LignoScan')) {
        manager.stopDeviceScan();
        setScanning(false);
        setDevice(scannedDevice);

        // Connect to the device
        scannedDevice
          .connect()
          .then((d) => d.discoverAllServicesAndCharacteristics())
          .then((d) => {
            setConnected(true);
            subscribeToCharacteristics(d);
          })
          .catch((e) => {
            setError(`Connection failed: ${e.message}`);
            setConnected(false);
          });
      }
    });
  }, [manager, scanning]);

  // Subscribe to BLE characteristic notifications
  const subscribeToCharacteristics = useCallback(async (d) => {
    const services = await d.services();

    for (const service of services) {
      if (service.uuid.toLowerCase().includes('lign')) {
        const characteristics = await service.characteristics();

        for (const char of characteristics) {
          if (char.isNotifiable) {
            char.monitor((err, characteristic) => {
              if (err) {
                setError(`Monitor error: ${err.message}`);
                return;
              }
              if (characteristic && characteristic.value) {
                handleNotification(characteristic.uuid, characteristic.value);
              }
            });
          }
        }
        break;
      }
    }
  }, []);

  // Handle incoming BLE notifications
  const handleNotification = useCallback((uuid, base64Value) => {
    const bytes = new Uint8Array(decode(base64Value).split('').map(c => c.charCodeAt(0)));

    // Determine packet type from first byte
    if (bytes.length < 4) return;

    const packetType = bytes[0];

    switch (packetType) {
      case 0x01: // Status
        const status = parseStatusPacket(bytes);
        setScanStatus(status);
        break;
      case 0x02: // ToF Matrix
        const tof = parseTofMatrix(bytes);
        setTofMatrix(tof);
        break;
      case 0x03: // Tomogram
        const tomo = parseTomogram(bytes);
        setTomogram(tomo);
        break;
      case 0x04: // GPS
        const gps = parseGpsData(bytes);
        setGpsData(gps);
        break;
      case 0x05: // Device info
        if (bytes.length >= 16) {
          setBatteryLevel(bytes[15]);
        }
        break;
      default:
        break;
    }
  }, []);

  // Start a scan on the device
  const startDeviceScan = useCallback(() => {
    if (!device || !connected) return;
    // Write to SCAN_TRIGGER characteristic
    const service = device.services().then(services => {
      for (const s of services) {
        if (s.uuid.toLowerCase().includes('lign')) {
          s.characteristics().then(chars => {
            for (const c of chars) {
              if (c.isWritableWithResponse) {
                const data = new Uint8Array([sensorCount]);
                const base64 = btoa(String.fromCharCode(...data));
                c.writeWithResponse(base64);
                break;
              }
            }
          });
          break;
        }
      }
    });
  }, [device, connected, sensorCount]);

  // Disconnect from device
  const disconnect = useCallback(() => {
    if (device && connected) {
      device.cancelConnection();
      setConnected(false);
      setDevice(null);
    }
  }, [device, connected]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      if (manager) {
        manager.destroy();
      }
    };
  }, [manager]);

  const value = {
    manager,
    device,
    connected,
    scanning,
    scanStatus,
    tofMatrix,
    tomogram,
    gpsData,
    batteryLevel,
    sensorCount,
    error,
    setSensorCount,
    startScan,
    startDeviceScan,
    disconnect,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
};

export const useBle = () => {
  const context = useContext(BleContext);
  if (!context) {
    throw new Error('useBle must be used within BleProvider');
  }
  return context;
};

export { BleContext };

// EOF — BleContext.js
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT