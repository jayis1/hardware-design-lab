/**
 * NeuroLink BLE Context — Manages Bluetooth connection state
 * Provides connection, data streaming, and command interface
 */

import React, { createContext, useContext, useState, useEffect, useRef } from 'react';
import { BleManager } from 'react-native-ble-plx';
import { NativeModules, Platform } from 'react-native';
import { parseFrame, buildCommand, COMMAND_IDS } from '../utils/protocol';

const BleContext = createContext(null);

// NeuroLink BLE Service & Characteristic UUIDs
const NEUROLINK_SERVICE_UUID = '6E4C5F00-2E4D-4A3B-8C5D-1F2E3A4B5C6D';
const NEUROLINK_DATA_CHAR_UUID = '6E4C5F01-2E4D-4A3B-8C5D-1F2E3A4B5C6D';
const NEUROLINK_CMD_CHAR_UUID = '6E4C5F02-2E4D-4A3B-8C5D-1F2E3A4B5C6D';
const NEUROLINK_STATUS_CHAR_UUID = '6E4C5F03-2E4D-4A3B-8C5D-1F2E3A4B5C6D';

export function BleProvider({ children }) {
  const [device, setDevice] = useState(null);
  const [connected, setConnected] = useState(false);
  const [scanning, setScanning] = useState(false);
  const [streaming, setStreaming] = useState(false);
  const [channelData, setChannelData] = useState(new Float32Array(32));
  const [impedanceData, setImpedanceData] = useState(new Float32Array(32));
  const [batteryLevel, setBatteryLevel] = useState(100);
  const [signalQuality, setSignalQuality] = useState(new Array(32).fill(100));
  const [error, setError] = useState(null);

  const bleManager = useRef(null);
  const subscription = useRef(null);

  useEffect(() => {
    bleManager.current = new BleManager();
    return () => {
      if (bleManager.current) {
        bleManager.current.destroy();
      }
    };
  }, []);

  const startScan = useCallback(async () => {
    if (!bleManager.current) return;
    setScanning(true);
    setError(null);

    try {
      bleManager.current.startDeviceScan(
        [NEUROLINK_SERVICE_UUID],
        { allowDuplicates: false },
        (error, scannedDevice) => {
          if (error) {
            setError(error.message);
            setScanning(false);
            return;
          }
          if (scannedDevice && scannedDevice.name?.startsWith('NeuroLink')) {
            bleManager.current.stopDeviceScan();
            setScanning(false);
            setDevice(scannedDevice);
          }
        }
      );

      // Auto-stop scan after 10 seconds
      setTimeout(() => {
        bleManager.current.stopDeviceScan();
        setScanning(false);
      }, 10000);
    } catch (err) {
      setError(err.message);
      setScanning(false);
    }
  }, []);

  const connectToDevice = useCallback(async (deviceToConnect) => {
    if (!bleManager.current || !deviceToConnect) return;

    try {
      const connectedDevice = await deviceToConnect.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();
      setDevice(connectedDevice);
      setConnected(true);
      setError(null);

      // Subscribe to status characteristic for battery/impedance
      subscription.current = await connectedDevice.monitorCharacteristicForService(
        NEUROLINK_SERVICE_UUID,
        NEUROLINK_STATUS_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            setError(error.message);
            return;
          }
          if (characteristic?.value) {
            const statusData = parseFrame(
              Buffer.from(characteristic.value, 'base64')
            );
            if (statusData.type === 'status') {
              setBatteryLevel(statusData.battery);
            }
          }
        }
      );
    } catch (err) {
      setError(err.message);
      setConnected(false);
    }
  }, []);

  const disconnect = useCallback(async () => {
    if (!device) return;
    try {
      if (subscription.current) {
        subscription.current.remove();
        subscription.current = null;
      }
      await device.cancelConnection();
      setConnected(false);
      setStreaming(false);
      setDevice(null);
    } catch (err) {
      setError(err.message);
    }
  }, [device]);

  const startStreaming = useCallback(async () => {
    if (!device || !connected) return;

    try {
      // Send start command
      const cmd = buildCommand(COMMAND_IDS.START_STREAM, []);
      await device.writeCharacteristicWithResponseForService(
        NEUROLINK_SERVICE_UUID,
        NEUROLINK_CMD_CHAR_UUID,
        cmd.toString('base64')
      );

      // Subscribe to data characteristic
      subscription.current = await device.monitorCharacteristicForService(
        NEUROLINK_SERVICE_UUID,
        NEUROLINK_DATA_CHAR_UUID,
        (error, characteristic) => {
          if (error) {
            setError(error.message);
            setStreaming(false);
            return;
          }
          if (characteristic?.value) {
            const rawData = Buffer.from(characteristic.value, 'base64');
            const parsed = parseFrame(rawData);
            if (parsed.type === 'samples') {
              setChannelData(parsed.channels);
            }
          }
        }
      );

      setStreaming(true);
    } catch (err) {
      setError(err.message);
      setStreaming(false);
    }
  }, [device, connected]);

  const stopStreaming = useCallback(async () => {
    if (!device || !connected) return;

    try {
      const cmd = buildCommand(COMMAND_IDS.STOP_STREAM, []);
      await device.writeCharacteristicWithResponseForService(
        NEUROLINK_SERVICE_UUID,
        NEUROLINK_CMD_CHAR_UUID,
        cmd.toString('base64')
      );

      if (subscription.current) {
        subscription.current.remove();
        subscription.current = null;
      }
      setStreaming(false);
    } catch (err) {
      setError(err.message);
    }
  }, [device, connected]);

  const sendCommand = useCallback(async (commandId, payload) => {
    if (!device || !connected) return;
    try {
      const cmd = buildCommand(commandId, payload || []);
      await device.writeCharacteristicWithResponseForService(
        NEUROLINK_SERVICE_UUID,
        NEUROLINK_CMD_CHAR_UUID,
        cmd.toString('base64')
      );
    } catch (err) {
      setError(err.message);
    }
  }, [device, connected]);

  const checkImpedance = useCallback(async () => {
    await sendCommand(COMMAND_IDS.IMPEDANCE_CHECK, []);
  }, [sendCommand]);

  const value = {
    device,
    connected,
    scanning,
    streaming,
    channelData,
    impedanceData,
    batteryLevel,
    signalQuality,
    error,
    startScan,
    connectToDevice,
    disconnect,
    startStreaming,
    stopStreaming,
    sendCommand,
    checkImpedance,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}

export function useBle() {
  return useContext(BleContext);
}