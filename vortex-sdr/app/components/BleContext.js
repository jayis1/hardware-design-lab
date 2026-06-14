/**
 * BleContext.js — BLE connection state management for Vortex-SDR
 * Provides context for BLE scanning, connection, and data transfer
 */

import React, { createContext, useContext, useState, useCallback, useRef } from 'react';
import { NativeEventEmitter, NativeModules, Platform, PermissionsAndroid } from 'react-native';
import BleManager from 'react-native-ble-manager';
import { VORTEX_SDR_SERVICE_UUID, VORTEX_SDR_CMD_CHAR_UUID, VORTEX_SDR_DATA_CHAR_UUID, VORTEX_SDR_STATUS_CHAR_UUID } from '../utils/protocol';

const BleContext = createContext(null);

export function useBle() {
  const context = useContext(BleContext);
  if (!context) {
    throw new Error('useBle must be used within a BleProvider');
  }
  return context;
}

export function BleProvider({ children }) {
  const [isScanning, setIsScanning] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState(null);
  const [deviceList, setDeviceList] = useState([]);
  const [isConnected, setIsConnected] = useState(false);
  const [spectrumData, setSpectrumData] = useState(null);
  const [deviceStatus, setDeviceStatus] = useState({
    state: 'disconnected',
    startFreq: 88000000,
    stopFreq: 108000000,
    fftSize: 1024,
    sweepCount: 0,
    batteryMv: 0,
    pllLocked: false,
    temperature: 0,
  });

  const dataBuffer = useRef(new Uint8Array(0));
  const scanTimeout = useRef(null);

  // Request BLE permissions (Android)
  const requestPermissions = useCallback(async () => {
    if (Platform.OS === 'android') {
      try {
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        ]);
        return Object.values(granted).every(
          (val) => val === PermissionsAndroid.RESULTS.GRANTED
        );
      } catch (err) {
        console.error('BLE permission error:', err);
        return false;
      }
    }
    return true;
  }, []);

  // Start BLE scan
  const startScan = useCallback(async () => {
    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      console.warn('BLE permissions not granted');
      return;
    }

    try {
      await BleManager.start({ showAlert: false });
    } catch (e) {
      // Already started
    }

    setIsScanning(true);
    setDeviceList([]);

    try {
      await BleManager.scan([VORTEX_SDR_SERVICE_UUID], 10, true);
      if (scanTimeout.current) clearTimeout(scanTimeout.current);
      scanTimeout.current = setTimeout(() => {
        setIsScanning(false);
      }, 10000);
    } catch (err) {
      console.error('BLE scan error:', err);
      setIsScanning(false);
    }
  }, [requestPermissions]);

  // Stop BLE scan
  const stopScan = useCallback(async () => {
    try {
      await BleManager.stopScan();
      setIsScanning(false);
      if (scanTimeout.current) {
        clearTimeout(scanTimeout.current);
        scanTimeout.current = null;
      }
    } catch (err) {
      console.error('BLE stop scan error:', err);
    }
  }, []);

  // Connect to device
  const connectToDevice = useCallback(async (deviceId) => {
    try {
      await BleManager.connect(deviceId);
      setConnectedDevice(deviceId);
      setIsConnected(true);

      // Discover services and characteristics
      const peripheralInfo = await BleManager.retrieveServices(deviceId, [
        VORTEX_SDR_SERVICE_UUID,
      ]);

      // Start notification listeners for data and status
      await BleManager.startNotification(
        deviceId,
        VORTEX_SDR_SERVICE_UUID,
        VORTEX_SDR_DATA_CHAR_UUID
      );
      await BleManager.startNotification(
        deviceId,
        VORTEX_SDR_SERVICE_UUID,
        VORTEX_SDR_STATUS_CHAR_UUID
      );

      console.log('Connected to Vortex-SDR:', deviceId);
    } catch (err) {
      console.error('BLE connect error:', err);
      setIsConnected(false);
      setConnectedDevice(null);
    }
  }, []);

  // Disconnect from device
  const disconnectDevice = useCallback(async () => {
    if (!connectedDevice) return;
    try {
      await BleManager.stopNotification(
        connectedDevice,
        VORTEX_SDR_SERVICE_UUID,
        VORTEX_SDR_DATA_CHAR_UUID
      );
      await BleManager.stopNotification(
        connectedDevice,
        VORTEX_SDR_SERVICE_UUID,
        VORTEX_SDR_STATUS_CHAR_UUID
      );
      await BleManager.disconnect(connectedDevice);
    } catch (err) {
      console.error('BLE disconnect error:', err);
    } finally {
      setConnectedDevice(null);
      setIsConnected(false);
      setSpectrumData(null);
      setDeviceStatus({
        state: 'disconnected',
        startFreq: 0,
        stopFreq: 0,
        fftSize: 1024,
        sweepCount: 0,
        batteryMv: 0,
        pllLocked: false,
        temperature: 0,
      });
    }
  }, [connectedDevice]);

  // Send command to device
  const sendCommand = useCallback(
    async (cmd, payload = null) => {
      if (!connectedDevice || !isConnected) return;

      const packet = buildCommandPacket(cmd, payload);
      try {
        await BleManager.write(
          connectedDevice,
          VORTEX_SDR_SERVICE_UUID,
          VORTEX_SDR_CMD_CHAR_UUID,
          Array.from(packet)
        );
      } catch (err) {
        console.error('BLE write error:', err);
      }
    },
    [connectedDevice, isConnected]
  );

  // Handle incoming BLE data
  const handleDataNotification = useCallback((data) => {
    // data is a Uint8Array from BLE notification
    // First 4 bytes: command ID + sequence number
    if (data.length < 6) return;

    const msgType = data[0];
    const msgLen = (data[1] << 8) | data[2];

    switch (msgType) {
      case 0x10: // Spectrum data
        parseSpectrumData(data.slice(3));
        break;
      case 0x11: // Status update
        parseStatusData(data.slice(3));
        break;
      case 0x12: // Peak data
        parsePeakData(data.slice(3));
        break;
      default:
        console.warn('Unknown BLE message type:', msgType);
    }
  }, []);

  // Parse spectrum data from device
  const parseSpectrumData = useCallback((data) => {
    if (data.length < 8) return;

    const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
    const numBins = view.getUint16(0, true);
    const centerFreq = Number(view.getBigUint64(2, true)); // Hz
    const binBw = view.getUint32(10, true); // Hz per bin

    const bins = new Int16Array(numBins);
    for (let i = 0; i < numBins && i + 14 < data.length; i++) {
      bins[i] = view.getInt16(12 + i * 2, true); // dBm * 10
    }

    setSpectrumData({
      bins,
      numBins,
      centerFreq,
      binBw,
      timestamp: Date.now(),
    });
  }, []);

  // Parse status data from device
  const parseStatusData = useCallback((data) => {
    if (data.length < 20) return;

    const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
    setDeviceStatus({
      state: view.getUint8(0),
      startFreq: Number(view.getBigUint64(1, true)),
      stopFreq: Number(view.getBigUint64(9, true)),
      fftSize: view.getUint16(17, true),
      sweepCount: view.getUint32(19, true),
      batteryMv: view.getUint16(23, true),
      pllLocked: view.getUint8(27) !== 0,
      temperature: view.getInt8(28),
    });
  }, []);

  // Parse peak data from device
  const parsePeakData = useCallback((data) => {
    if (data.length < 2) return;

    const numPeaks = data[0];
    const peaks = [];
    const view = new DataView(data.buffer, data.byteOffset + 1, data.byteLength - 1);

    for (let i = 0; i < numPeaks && i * 10 + 10 <= data.length - 1; i++) {
      peaks.push({
        freq: Number(view.getBigUint64(i * 10, true)),
        power: view.getInt16(i * 10 + 8, true) / 10.0, // dBm
      });
    }

    return peaks;
  }, []);

  // Build command packet
  const buildCommandPacket = useCallback((cmd, payload) => {
    const header = new Uint8Array(4);
    header[0] = cmd;
    header[1] = (payload ? payload.length + 1 : 1) >> 8;
    header[2] = (payload ? payload.length + 1 : 1) & 0xff;
    header[3] = 0; // sequence number (will be filled by protocol layer)

    if (payload) {
      const packet = new Uint8Array(4 + payload.length);
      packet.set(header, 0);
      packet.set(payload, 4);
      return packet;
    }
    return header;
  }, []);

  // Set up event listeners
  React.useEffect(() => {
    const bleManagerEmitter = new NativeEventEmitter(NativeModules.BleManager);

    const discoverHandler = bleManagerEmitter.addListener(
      'BleManagerDiscoverPeripheral',
      (peripheral) => {
        if (peripheral.advertising) {
          const serviceUUIDs = peripheral.advertising.serviceUUIDs || [];
          if (serviceUUIDs.includes(VORTEX_SDR_SERVICE_UUID)) {
            setDeviceList((prev) => {
              const exists = prev.find((d) => d.id === peripheral.id);
              if (!exists) {
                return [...prev, {
                  id: peripheral.id,
                  name: peripheral.name || 'Vortex-SDR',
                  rssi: peripheral.rssi,
                }];
              }
              return prev;
            });
          }
        }
      }
    );

    const disconnectHandler = bleManagerEmitter.addListener(
      'BleManagerDisconnectPeripheral',
      () => {
        setIsConnected(false);
        setConnectedDevice(null);
        setSpectrumData(null);
      }
    );

    const notificationHandler = bleManagerEmitter.addListener(
      'BleManagerDidUpdateValueForCharacteristic',
      (info) => {
        if (info.characteristic === VORTEX_SDR_DATA_CHAR_UUID) {
          handleDataNotification(new Uint8Array(info.value));
        } else if (info.characteristic === VORTEX_SDR_STATUS_CHAR_UUID) {
          handleDataNotification(new Uint8Array(info.value));
        }
      }
    );

    return () => {
      discoverHandler.remove();
      disconnectHandler.remove();
      notificationHandler.remove();
    };
  }, [handleDataNotification]);

  const value = {
    isScanning,
    isConnected,
    connectedDevice,
    deviceList,
    spectrumData,
    deviceStatus,
    startScan,
    stopScan,
    connectToDevice,
    disconnectDevice,
    sendCommand,
  };

  return <BleContext.Provider value={value}>{children}</BleContext.Provider>;
}