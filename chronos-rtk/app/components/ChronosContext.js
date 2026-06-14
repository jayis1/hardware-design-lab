/**
 * ChronosContext.js — React context provider for Chronos-RTK device connection
 * Manages USB/Bluetooth serial connection, NMEA parsing, and state
 */

import React, { createContext, useState, useCallback, useRef } from 'react';
import { NativeEventEmitter, NativeModules, Platform } from 'react-native';
import { ChronosProtocol, COMMAND_IDS } from '../utils/protocol';

export const ChronosContext = createContext(null);

export function ChronosProvider({ children }) {
  const [isConnected, setIsConnected] = useState(false);
  const [position, setPosition] = useState({
    latitude: null,
    longitude: null,
    altitude: null,
    hAccuracy: null,
    vAccuracy: null,
    speed: null,
    heading: null,
  });
  const [satellites, setSatellites] = useState({
    inView: 0,
    used: 0,
    gps: 0,
    glo: 0,
    gal: 0,
    bds: 0,
    pdop: null,
  });
  const [rtkStatus, setRtkStatus] = useState('No Fix');
  const [logs, setLogs] = useState([]);
  const [config, setConfig] = useState({
    firmware: 'v1.0.0',
    battery: null,
    flashUsed: null,
  });

  const protocol = useRef(new ChronosProtocol()).current;
  const serialRef = useRef(null);

  const addLog = useCallback((type, message) => {
    const timestamp = new Date().toISOString().substr(11, 12);
    setLogs((prev) => [...prev.slice(-499), { type, message, timestamp }]);
  }, []);

  const parseNmea = useCallback((sentence) => {
    if (!sentence || sentence.length < 6) return;
    const fields = sentence.split(',');

    switch (fields[0]) {
      case '$GNGGA':
      case '$GPGGA': {
        const lat = parseFloat(fields[2]);
        const lon = parseFloat(fields[4]);
        const alt = parseFloat(fields[9]);
        const hAcc = parseFloat(fields[6]) || null;
        setPosition((prev) => ({
          ...prev,
          latitude: fields[3] === 'S' ? -lat / 100 : lat / 100,
          longitude: fields[5] === 'W' ? -lon / 100 : lon / 100,
          altitude: alt,
          hAccuracy: hAcc,
        }));
        // Fix quality: 0=none, 1=GPS, 2=DGPS, 4=RTK Fixed, 5=RTK Float
        const quality = parseInt(fields[6]);
        switch (quality) {
          case 4: setRtkStatus('RTK Fixed'); break;
          case 5: setRtkStatus('RTK Float'); break;
          case 2: setRtkStatus('DGPS'); break;
          case 1: setRtkStatus('3D Fix'); break;
          default: setRtkStatus('No Fix'); break;
        }
        addLog('NMEA', sentence);
        break;
      }
      case '$GNGSA':
      case '$GPGSA': {
        // PDOP is in field 15
        const pdop = parseFloat(fields[15]) || null;
        setSatellites((prev) => ({ ...prev, pdop }));
        break;
      }
      case '$GNGSV':
      case '$GPGSV': {
        const total = parseInt(fields[3]) || 0;
        setSatellites((prev) => ({ ...prev, inView: total }));
        break;
      }
      default:
        addLog('NMEA', sentence);
    }
  }, [addLog]);

  const connect = useCallback(async () => {
    try {
      // In a real implementation, this would use react-native-usb-serial
      // or react-native-ble-plx to connect to the Chronos-RTK board
      addLog('SYS', 'Connecting to Chronos-RTK...');

      // Simulated connection for now
      // Real implementation would:
      // 1. Enumerate USB serial devices
      // 2. Open serial port at 115200 baud
      // 3. Send handshake command via ChronosProtocol
      // 4. Start receiving NMEA data

      const handshake = protocol.buildCommand(COMMAND_IDS.HANDSHAKE, []);
      addLog('SYS', `Sent handshake: ${handshake.map((b) => b.toString(16)).join(' ')}`);

      setIsConnected(true);
      addLog('SYS', 'Connected to Chronos-RTK');

      // Request config
      const configCmd = protocol.buildCommand(COMMAND_IDS.GET_CONFIG, []);
      addLog('SYS', 'Requested device configuration');
    } catch (error) {
      addLog('ERR', `Connection failed: ${error.message}`);
    }
  }, [addLog, protocol]);

  const disconnect = useCallback(() => {
    setIsConnected(false);
    setRtkStatus('No Fix');
    addLog('SYS', 'Disconnected');
  }, [addLog]);

  const sendCommand = useCallback((command, value) => {
    if (!isConnected) return;

    addLog('SYS', `Sending: ${command} = ${value}`);

    // Map commands to protocol command IDs
    let cmdId;
    switch (command) {
      case 'SET_LORA_FREQ': cmdId = COMMAND_IDS.SET_LORA_FREQ; break;
      case 'SET_LORA_SF': cmdId = COMMAND_IDS.SET_LORA_SF; break;
      case 'SET_RTK_MODE': cmdId = COMMAND_IDS.SET_RTK_MODE; break;
      case 'SET_NMEA_RATE': cmdId = COMMAND_IDS.SET_NMEA_RATE; break;
      case 'SET_OUTPUT': cmdId = COMMAND_IDS.SET_OUTPUT; break;
      case 'SET_LOGGING': cmdId = COMMAND_IDS.SET_LOGGING; break;
      case 'ERASE_FLASH': cmdId = COMMAND_IDS.ERASE_FLASH; break;
      case 'DOWNLOAD_LOG': cmdId = COMMAND_IDS.DOWNLOAD_LOG; break;
      case 'CLEAR_LOG': cmdId = COMMAND_IDS.CLEAR_LOG; break;
      default: cmdId = COMMAND_IDS.CUSTOM;
    }

    const frame = protocol.buildCommand(cmdId, [parseInt(value) || 0]);
    addLog('SYS', `Frame: ${frame.map((b) => b.toString(16).padStart(2, '0')).join(' ')}`);

    // In a real implementation, this would write to the serial port
    // serialRef.current.write(frame);
  }, [isConnected, addLog, protocol]);

  return (
    <ChronosContext.Provider
      value={{
        isConnected,
        position,
        satellites,
        rtkStatus,
        logs,
        config,
        connect,
        disconnect,
        sendCommand,
        parseNmea,
      }}
    >
      {children}
    </ChronosContext.Provider>
  );
}