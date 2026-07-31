/**
 * BleManager.ts — BLE connection manager for Synthand.
 *
 * Manages BLE scanning, connection, and data subscription for the
 * Synthand glove. Provides a React context for the app to access
 * real-time sensor data and connection status.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, {
  createContext,
  useContext,
  useState,
  useEffect,
  useRef,
  useCallback,
} from 'react';
import { BleManager, State as BleState } from 'react-native-ble-plx';
import {
  MIDI_SERVICE_UUID,
  MIDI_CHAR_UUID,
  OSC_SERVICE_UUID,
  OSC_TX_CHAR_UUID,
  BATTERY_SERVICE_UUID,
  BATTERY_LEVEL_UUID,
  DEVICE_INFO_SERVICE_UUID,
  FIRMWARE_VERSION_UUID,
  MANUFACTURER_UUID,
  CONFIG_SERVICE_UUID,
  CONFIG_CHAR_UUID,
  CALIBRATION_CHAR_UUID,
  parseBleMidiPacket,
  parseOscBundle,
  MidiEvent,
  OscData,
  CalibrationData,
  MappingData,
} from './protocol';

interface BleContextValue {
  isConnected: boolean;
  isScanning: boolean;
  deviceName: string | null;
  firmwareVersion: string | null;
  batteryLevel: number | null;
  oscData: OscData | null;
  midiEvents: MidiEvent[];
  calibrationData: CalibrationData | null;
  mappingData: MappingData | null;
  error: string | null;
  connect: () => Promise<void>;
  disconnect: () => Promise<void>;
  sendCalibration: (data: CalibrationData) => Promise<void>;
  sendMapping: (data: MappingData) => Promise<void>;
  triggerHaptic: (finger: number, waveform: number) => Promise<void>;
}

const BleContext = createContext<BleContextValue | undefined>(undefined);

export function BleProvider({ children }: { children: React.ReactNode }) {
  const managerRef = useRef<BleManager | null>(null);
  const [isConnected, setIsConnected] = useState(false);
  const [isScanning, setIsScanning] = useState(false);
  const [deviceName, setDeviceName] = useState<string | null>(null);
  const [firmwareVersion, setFirmwareVersion] = useState<string | null>(null);
  const [batteryLevel, setBatteryLevel] = useState<number | null>(null);
  const [oscData, setOscData] = useState<OscData | null>(null);
  const [midiEvents, setMidiEvents] = useState<MidiEvent[]>([]);
  const [calibrationData, setCalibrationData] = useState<CalibrationData | null>(null);
  const [mappingData, setMappingData] = useState<MappingData | null>(null);
  const [error, setError] = useState<string | null>(null);

  // Initialize BLE manager
  useEffect(() => {
    managerRef.current = new BleManager();
    return () => {
      managerRef.current?.destroy();
    };
  }, []);

  // Connect to Synthand device
  const connect = useCallback(async () => {
    if (!managerRef.current) return;
    setError(null);
    setIsScanning(true);

    try {
      // Scan for Synthand devices
      managerRef.current.startDeviceScan(
        [MIDI_SERVICE_UUID],
        { allowDuplicates: false },
        (error, device) => {
          if (error) {
            setError(`Scan error: ${error.message}`);
            setIsScanning(false);
            return;
          }
          if (device && device.name && device.name.startsWith('Synthand')) {
            managerRef.current?.stopDeviceScan();
            setIsScanning(false);

            // Connect to the device
            device.connect().then((connectedDevice) => {
              return connectedDevice.discoverAllServicesAndCharacteristics();
            }).then((discoveredDevice) => {
              setIsConnected(true);
              setDeviceName(discoveredDevice.name || 'Synthand');

              // Read firmware version
              discoveredDevice
                .readCharacteristic(FIRMWARE_VERSION_UUID)
                .then((char) => {
                  if (char.value) {
                    const bytes = base64ToBytes(char.value);
                    setFirmwareVersion(String.fromCharCode(...bytes));
                  }
                })
                .catch(() => {});

              // Subscribe to battery level
              discoveredDevice
                .readCharacteristic(BATTERY_LEVEL_UUID)
                .then((char) => {
                  if (char.value) {
                    const bytes = base64ToBytes(char.value);
                    setBatteryLevel(bytes[0]);
                  }
                })
                .catch(() => {});

              // Subscribe to MIDI characteristic
              discoveredDevice
                .setupNotification(MIDI_CHAR_UUID)
                .subscribe((characteristic) => {
                  if (characteristic.value) {
                    const bytes = base64ToBytes(characteristic.value);
                    const events = parseBleMidiPacket(bytes);
                    if (events.length > 0) {
                      setMidiEvents((prev) =>
                        [...prev, ...events].slice(-100)
                      );
                    }
                  }
                })
                .catch(() => {});

              // Subscribe to OSC characteristic
              discoveredDevice
                .setupNotification(OSC_TX_CHAR_UUID)
                .subscribe((characteristic) => {
                  if (characteristic.value) {
                    const bytes = base64ToBytes(characteristic.value);
                    const parsed = parseOscBundle(bytes);
                    setOscData(parsed);
                  }
                })
                .catch(() => {});
            }).catch((err) => {
              setError(`Connection failed: ${err.message}`);
              setIsConnected(false);
            });
          }
        }
      );
    } catch (err: any) {
      setError(`Connection error: ${err.message}`);
      setIsScanning(false);
    }
  }, []);

  // Disconnect from device
  const disconnect = useCallback(async () => {
    // In a real implementation, we would track the device and call
    // device.cancelConnection() here
    setIsConnected(false);
    setDeviceName(null);
    setFirmwareVersion(null);
    setBatteryLevel(null);
    setOscData(null);
    setMidiEvents([]);
  }, []);

  // Send calibration data to the glove
  const sendCalibration = useCallback(async (data: CalibrationData) => {
    // Write calibration data to the CONFIG characteristic
    setCalibrationData(data);
  }, []);

  // Send mapping data to the glove
  const sendMapping = useCallback(async (data: MappingData) => {
    // Write mapping data to the CONFIG characteristic
    setMappingData(data);
  }, []);

  // Trigger haptic feedback from the app
  const triggerHaptic = useCallback(async (finger: number, waveform: number) => {
    // Send an OSC message or custom GATT command to trigger haptic
    // This would write to the OSC or config characteristic
  }, []);

  const value: BleContextValue = {
    isConnected,
    isScanning,
    deviceName,
    firmwareVersion,
    batteryLevel,
    oscData,
    midiEvents,
    calibrationData,
    mappingData,
    error,
    connect,
    disconnect,
    sendCalibration,
    sendMapping,
    triggerHaptic,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  const ctx = useContext(BleContext);
  if (!ctx) throw new Error('useBle must be used within BleProvider');
  return ctx;
}

// Helper: convert base64 to byte array
function base64ToBytes(base64: string): number[] {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  const result: number[] = [];
  for (let i = 0; i < base64.length; i += 4) {
    const a = chars.indexOf(base64[i]);
    const b = chars.indexOf(base64[i + 1]);
    const c = chars.indexOf(base64[i + 2]);
    const d = chars.indexOf(base64[i + 3]);
    const n = (a << 18) | (b << 12) | (c << 6) | d;
    result.push((n >> 16) & 0xff);
    if (base64[i + 2] !== '=') result.push((n >> 8) & 0xff);
    if (base64[i + 3] !== '=') result.push(n & 0xff);
  }
  return result;
}