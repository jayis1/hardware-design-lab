/**
 * BleContext.js — BLE connection manager for WattLens
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Manages BLE scanning, connection, and notification subscriptions.
 * Provides React context for all screens to access device data.
 */

import React, {
  createContext,
  useContext,
  useState,
  useEffect,
  useRef,
  useCallback,
} from 'react';
import {
  BleManager,
  State as BleState,
} from 'react-native-ble-plx';

import {
  WATTLENS_SERVICE_UUID,
  CHAR_METRICS_UUID,
  CHAR_NILM_UUID,
  CHAR_EVENT_UUID,
  CHAR_STATUS_UUID,
  decodeMetrics,
  decodeNilm,
  decodeEvent,
  decodeStatus,
} from './protocol';

const BleContext = createContext(null);

export const useBle = () => useContext(BleContext);

// ========================================================================
// Error type names
// ========================================================================
const ERROR_NAMES = {
  1: 'ADC SPI Error',
  2: 'ADC Timeout',
  4: 'SD Card Mount Failed',
  8: 'SD Write Error',
  16: 'BLE Init Failed',
  32: 'LoRa Init Failed',
  64: 'Calibration Invalid',
  128: 'NILM Model Missing',
  256: 'Battery Low',
  512: 'Over-temperature',
};

function decodeErrorFlags(flags) {
  const errors = [];
  for (const [bit, name] of Object.entries(ERROR_NAMES)) {
    if (flags & parseInt(bit)) errors.push(name);
  }
  return errors;
}

// ========================================================================
// BleProvider component
// ========================================================================
export function BleProvider({ children }) {
  const manager = useRef(null);
  const deviceRef = useRef(null);

  const [connectionState, setConnectionState] = useState('disconnected');
  const [metrics, setMetrics] = useState(null);
  const [nilm, setNilm] = useState(null);
  const [events, setEvents] = useState([]);
  const [status, setStatus] = useState(null);
  const [errors, setErrors] = useState([]);
  const [scanning, setScanning] = useState(false);

  // ========================================================================
  // Initialize BLE manager
  // ========================================================================
  useEffect(() => {
    manager.current = new BleManager();

    const subscription = manager.current.onStateChange((state) => {
      if (state === BleState.PoweredOn) {
        // Auto-scan when Bluetooth is enabled
        scanAndConnect();
      }
    }, true);

    return () => {
      subscription.remove();
      if (deviceRef.current) {
        deviceRef.current.cancelConnection();
      }
      if (manager.current) {
        manager.current.destroy();
      }
    };
  }, []);

  // ========================================================================
  // Scan for WattLens device and connect
  // ========================================================================
  const scanAndConnect = useCallback(() => {
    if (!manager.current || scanning) return;
    setScanning(true);
    setConnectionState('scanning');

    manager.current.startDeviceScan(
      [WATTLENS_SERVICE_UUID],
      { allowDuplicates: false },
      (error, device) => {
        if (error) {
          console.error('[WattLens] Scan error:', error);
          setScanning(false);
          setConnectionState('error');
          return;
        }

        if (device && device.name && device.name.includes('WattLens')) {
          manager.current.stopDeviceScan();
          setScanning(false);

          device
            .connect()
            .then((d) => d.discoverAllServicesAndCharacteristics())
            .then((d) => {
              deviceRef.current = d;
              setConnectionState('connected');
              subscribeToCharacteristics(d);
            })
            .catch((err) => {
              console.error('[WattLens] Connection error:', err);
              setConnectionState('error');
            });
        }
      }
    );

    // Stop scanning after 10 seconds
    setTimeout(() => {
      if (scanning) {
        manager.current.stopDeviceScan();
        setScanning(false);
        setConnectionState('disconnected');
      }
    }, 10000);
  }, [scanning]);

  // ========================================================================
  // Subscribe to BLE notification characteristics
  // ========================================================================
  const subscribeToCharacteristics = useCallback((device) => {
    // Metrics (1 Hz)
    device.monitorCharacteristicForService(
      WATTLENS_SERVICE_UUID,
      CHAR_METRICS_UUID,
      (error, char) => {
        if (error || !char?.value) return;
        const bytes = base64ToUint8Array(char.value);
        const decoded = decodeMetrics(bytes);
        if (decoded) setMetrics(decoded);
      }
    );

    // NILM state (0.5 Hz)
    device.monitorCharacteristicForService(
      WATTLENS_SERVICE_UUID,
      CHAR_NILM_UUID,
      (error, char) => {
        if (error || !char?.value) return;
        const bytes = base64ToUint8Array(char.value);
        const decoded = decodeNilm(bytes);
        if (decoded) setNilm(decoded);
      }
    );

    // Events
    device.monitorCharacteristicForService(
      WATTLENS_SERVICE_UUID,
      CHAR_EVENT_UUID,
      (error, char) => {
        if (error || !char?.value) return;
        const bytes = base64ToUint8Array(char.value);
        const decoded = decodeEvent(bytes);
        if (decoded) {
          setEvents((prev) => [decoded, ...prev].slice(0, 100));
        }
      }
    );

    // Status
    device.monitorCharacteristicForService(
      WATTLENS_SERVICE_UUID,
      CHAR_STATUS_UUID,
      (error, char) => {
        if (error || !char?.value) return;
        const bytes = base64ToUint8Array(char.value);
        const decoded = decodeStatus(bytes);
        if (decoded) {
          setStatus(decoded);
          setErrors(decodeErrorFlags(decoded.errorFlags));
        }
      }
    );
  }, []);

  // ========================================================================
  // Send command to device
  // ========================================================================
  const sendCommand = useCallback(async (encodedFrame) => {
    if (!deviceRef.current || connectionState !== 'connected') return;
    try {
      const base64 = uint8ArrayToBase64(encodedFrame);
      await deviceRef.current.writeCharacteristicWithResponseForService(
        WATTLENS_SERVICE_UUID,
        '0000ff15-1212-ef12-1234-56789abcdef0',
        base64
      );
    } catch (e) {
      console.error('[WattLens] Command send error:', e);
    }
  }, [connectionState]);

  // ========================================================================
  // Reconnect
  // ========================================================================
  const reconnect = useCallback(() => {
    if (deviceRef.current) {
      deviceRef.current.cancelConnection().finally(() => {
        deviceRef.current = null;
        scanAndConnect();
      });
    } else {
      scanAndConnect();
    }
  }, [scanAndConnect]);

  const value = {
    connectionState,
    scanning,
    metrics,
    nilm,
    events,
    status,
    errors,
    sendCommand,
    reconnect,
    scanAndConnect,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

// ========================================================================
// Utility: base64 <-> Uint8Array
// ========================================================================
function base64ToUint8Array(base64) {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes;
}

function uint8ArrayToBase64(bytes) {
  let binary = '';
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }
  return btoa(binary);
}

// atob/btoa polyfill for React Native (uses base64-js if available)
if (typeof atob === 'undefined') {
  global.atob = (input) => {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let output = '';
    let str = input.replace(/=+$/, '');
    for (let bc = 0, bs = 0, buffer, i = 0;
         (buffer = str.charAt(i++));
         ~buffer && (bs = bc % 4 ? bs * 64 + buffer : buffer, bc++ % 4)
        ) {
      output += String.fromCharCode(255 & (bs >> ((-2 * bc) & 6)));
    }
    return output;
  };
}

if (typeof btoa === 'undefined') {
  global.btoa = (input) => {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let output = '';
    for (let block = 0, charCode, i = 0, map = chars;
         input.charAt(i | 0) || (map = '=', i % 1);
         output += map.charAt(63 & (block >> (8 - (i % 1) * 8)))
        ) {
      charCode = input.charCodeAt(i += 3 / 4);
      block = block << 8 | charCode;
    }
    return output;
  };
}

export { BleContext };