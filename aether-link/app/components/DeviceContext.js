// DeviceContext.js — Aether-Link Device Connection Context
// Provides global device connection state, TCP socket management,
// and command/response handling for all screens in the app.

import React, { createContext, useContext, useState, useRef, useCallback, useEffect } from 'react';
import { Alert, Platform } from 'react-native';
import TcpSocket from 'react-native-tcp-socket';
import {
  buildFrame, parseFrame, buildResponse, parseTelemetry, parseStats,
  parseEvent, parseConnectionList, buildAddConnectionPayload,
  FrameAccumulator, CMD, FLAGS, STATUS, DEFAULT_PORT, getCommandName, getStatusName,
} from '../utils/protocol';

const DeviceContext = createContext(null);

// ============================================================================
// Pending Request Tracking
// ============================================================================

class PendingRequest {
  constructor(cmdId, seqNum, resolve, reject, timeoutMs = 5000) {
    this.cmdId = cmdId;
    this.seqNum = seqNum;
    this.resolve = resolve;
    this.reject = reject;
    this.timestamp = Date.now();
    this.timeoutMs = timeoutMs;
    this.timeoutHandle = setTimeout(() => {
      this.reject(new Error(`Request timeout: ${getCommandName(cmdId)}`));
    }, timeoutMs);
  }

  cancel() {
    clearTimeout(this.timeoutHandle);
  }
}

// ============================================================================
// Device Provider Component
// ============================================================================

export function DeviceProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [deviceHost, setDeviceHost] = useState('192.168.1.10');
  const [devicePort, setDevicePort] = useState(DEFAULT_PORT);
  const [telemetry, setTelemetry] = useState(null);
  const [stats, setStats] = useState(null);
  const [connections, setConnections] = useState([]);
  const [errorLog, setErrorLog] = useState([]);
  const [events, setEvents] = useState([]);
  const [firmwareVersion, setFirmwareVersion] = useState(null);
  const [deviceStatus, setDeviceStatus] = useState(null);
  const [deviceFeatures, setDeviceFeatures] = useState(null);

  const socketRef = useRef(null);
  const accumulatorRef = useRef(new FrameAccumulator());
  const pendingRequestsRef = useRef(new Map());
  const telemetryIntervalRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);

  // ==========================================================================
  // Socket Event Handlers
  // ==========================================================================

  const handleSocketData = useCallback((data) => {
    const frames = accumulatorRef.current.addData(Buffer.from(data));

    for (const frame of frames) {
      if (!frame.valid) {
        console.warn('Invalid frame received:', frame.error);
        continue;
      }

      // Check if this is a response to a pending request
      const pendingKey = `${frame.cmdId}:${frame.seqNum}`;
      const pending = pendingRequestsRef.current.get(pendingKey);

      if (pending) {
        pending.cancel();
        pendingRequestsRef.current.delete(pendingKey);

        if (frame.payload && frame.payload.length >= 2) {
          const statusCode = frame.payload.readUInt16BE(0);
          if (statusCode === STATUS.OK) {
            pending.resolve(frame);
          } else {
            pending.reject(new Error(
              `Command failed: ${getStatusName(statusCode)}`
            ));
          }
        } else {
          pending.resolve(frame);
        }
        continue;
      }

      // Check if this is an event notification
      if (frame.cmdId >= 0x8000) {
        const event = parseEvent(frame);
        if (event) {
          setEvents((prev) => [event, ...prev].slice(0, 100));
          handleEvent(event);
        }
        continue;
      }

      // Unsolicited response — could be from a previous session
      console.warn('Unsolicited response:', getCommandName(frame.cmdId));
    }
  }, []);

  const handleSocketError = useCallback((error) => {
    console.error('Socket error:', error);
    setConnected(false);
    cleanupConnection();
  }, []);

  const handleSocketClose = useCallback(() => {
    setConnected(false);
    cleanupConnection();
  }, []);

  // ==========================================================================
  // Connection Management
  // ==========================================================================

  const cleanupConnection = useCallback(() => {
    if (telemetryIntervalRef.current) {
      clearInterval(telemetryIntervalRef.current);
      telemetryIntervalRef.current = null;
    }

    // Cancel all pending requests
    for (const [, pending] of pendingRequestsRef.current) {
      pending.cancel();
      pending.reject(new Error('Connection lost'));
    }
    pendingRequestsRef.current.clear();

    if (socketRef.current) {
      socketRef.current.destroy();
      socketRef.current = null;
    }

    accumulatorRef.current.reset();
  }, []);

  const connect = useCallback(async () => {
    if (connected || connecting) return;

    setConnecting(true);

    return new Promise((resolve, reject) => {
      try {
        const socket = TcpSocket.createConnection({
          host: deviceHost,
          port: devicePort,
        });

        socketRef.current = socket;

        socket.on('data', handleSocketData);
        socket.on('error', (err) => {
          handleSocketError(err);
          reject(err);
        });
        socket.on('close', handleSocketClose);

        socket.on('connect', () => {
          setConnected(true);
          setConnecting(false);

          // Start telemetry polling
          telemetryIntervalRef.current = setInterval(() => {
            sendCommand(CMD.GET_TELEMETRY).then((frame) => {
              if (frame.payload) {
                const t = parseTelemetry(frame.payload);
                if (t) setTelemetry(t);
              }
            }).catch(() => {});

            sendCommand(CMD.GET_STATS).then((frame) => {
              if (frame.payload) {
                const s = parseStats(frame.payload);
                if (s) setStats(s);
              }
            }).catch(() => {});
          }, 2000);  // Poll every 2 seconds

          // Fetch initial data
          Promise.all([
            sendCommand(CMD.GET_VERSION),
            sendCommand(CMD.GET_STATUS),
            sendCommand(CMD.GET_FEATURES),
            sendCommand(CMD.GET_CONN_LIST),
          ]).then(([verFrame, statusFrame, featFrame, connFrame]) => {
            if (verFrame.payload) {
              setFirmwareVersion(verFrame.payload.readUInt32BE(4));
            }
            if (statusFrame.payload) {
              setDeviceStatus(statusFrame.payload.readUInt32BE(4));
            }
            if (featFrame.payload) {
              setDeviceFeatures(featFrame.payload.readUInt32BE(4));
            }
            if (connFrame.payload) {
              const conns = parseConnectionList(connFrame.payload);
              setConnections(conns);
            }
          }).catch(() => {});

          resolve();
        });
      } catch (err) {
        setConnecting(false);
        reject(err);
      }
    });
  }, [connected, connecting, deviceHost, devicePort, handleSocketData, handleSocketError, handleSocketClose]);

  const disconnect = useCallback(() => {
    cleanupConnection();
    if (socketRef.current) {
      socketRef.current.destroy();
      socketRef.current = null;
    }
    setConnected(false);
  }, [cleanupConnection]);

  // ==========================================================================
  // Command Sending
  // ==========================================================================

  const sendCommand = useCallback((cmdId, payload = null, flags = FLAGS.RESPONSE_EXPECTED, timeoutMs = 5000) => {
    return new Promise((resolve, reject) => {
      if (!socketRef.current || !connected) {
        reject(new Error('Not connected'));
        return;
      }

      try {
        const frame = buildFrame(cmdId, payload, flags);
        const seqNum = frame.readUInt32BE(10);  // SEQ_NUM offset

        // Register pending request
        const pendingKey = `${cmdId}:${seqNum}`;
        const pending = new PendingRequest(cmdId, seqNum, resolve, reject, timeoutMs);
        pendingRequestsRef.current.set(pendingKey, pending);

        // Send frame
        socketRef.current.write(frame.toString('base64'), 'base64');
      } catch (err) {
        reject(err);
      }
    });
  }, [connected]);

  // ==========================================================================
  // Event Handling
  // ==========================================================================

  const handleEvent = useCallback((event) => {
    switch (event.type) {
      case CMD.EVENT_TEMP_WARNING:
        Alert.alert(
          'Temperature Warning',
          `FPGA temperature reached ${event.fpgaTemp?.toFixed(1)}°C. ` +
          'Performance may be reduced.'
        );
        break;

      case CMD.EVENT_TEMP_CRITICAL:
        Alert.alert(
          '⚠️ Critical Temperature',
          `FPGA temperature at ${event.fpgaTemp?.toFixed(1)}°C! ` +
          'Device may shut down to prevent damage.'
        );
        break;

      case CMD.EVENT_LINK_CHANGE:
        const linkMsg = event.linkUp ? 'Link UP' : 'Link DOWN';
        Alert.alert(
          'Network Link Change',
          `Port ${event.portId}: ${linkMsg} at ${event.speed} Mbps`
        );
        // Refresh connections
        sendCommand(CMD.GET_CONN_LIST).then((frame) => {
          if (frame.payload) {
            setConnections(parseConnectionList(frame.payload));
          }
        }).catch(() => {});
        break;

      case CMD.EVENT_CONN_CHANGE:
        // Refresh connection list
        sendCommand(CMD.GET_CONN_LIST).then((frame) => {
          if (frame.payload) {
            setConnections(parseConnectionList(frame.payload));
          }
        }).catch(() => {});
        break;

      case CMD.EVENT_POWER_WARNING:
        Alert.alert(
          'Power Warning',
          `Board power consumption: ${(event.powerMW / 1000).toFixed(1)}W. ` +
          'Approaching PCIe slot limit.'
        );
        break;

      case CMD.EVENT_ERROR:
        Alert.alert(
          'Device Error',
          `Error code: 0x${event.errorCode?.toString(16).toUpperCase()} ` +
          `Severity: ${event.severity} Source: ${event.source}`
        );
        break;

      default:
        break;
    }
  }, [sendCommand]);

  // ==========================================================================
  // High-Level Command Functions
  // ==========================================================================

  const addConnection = useCallback(async (targetIP, targetPort, portId) => {
    const payload = buildAddConnectionPayload(targetIP, targetPort, portId);
    const frame = await sendCommand(CMD.ADD_CONNECTION, payload);
    if (frame.payload) {
      const statusCode = frame.payload.readUInt16BE(0);
      if (statusCode === STATUS.OK) {
        // Refresh connection list
        const connFrame = await sendCommand(CMD.GET_CONN_LIST);
        if (connFrame.payload) {
          setConnections(parseConnectionList(connFrame.payload));
        }
        return true;
      }
    }
    return false;
  }, [sendCommand]);

  const deleteConnection = useCallback(async (connId) => {
    const payload = Buffer.alloc(2);
    payload.writeUInt16BE(connId, 0);
    const frame = await sendCommand(CMD.DELETE_CONNECTION, payload);
    if (frame.payload) {
      const statusCode = frame.payload.readUInt16BE(0);
      if (statusCode === STATUS.OK) {
        setConnections((prev) => prev.filter((c) => c.connId !== connId));
        return true;
      }
    }
    return false;
  }, [sendCommand]);

  const setFanSpeed = useCallback(async (pwmDuty) => {
    const payload = Buffer.alloc(1);
    payload.writeUInt8(Math.min(255, Math.max(0, pwmDuty)), 0);
    await sendCommand(CMD.SET_FAN_PWM, payload);
  }, [sendCommand]);

  const getErrorLog = useCallback(async (count = 20) => {
    const payload = Buffer.alloc(2);
    payload.writeUInt16BE(count, 0);
    const frame = await sendCommand(CMD.GET_ERROR_LOG, payload);
    if (frame.payload) {
      // Parse error log entries
      const entries = [];
      let offset = 4;  // Skip status
      const entryCount = frame.payload.readUInt16BE(offset);
      offset += 2;
      for (let i = 0; i < entryCount && offset + 16 <= frame.payload.length; i++) {
        entries.push({
          timestamp: frame.payload.readUInt32BE(offset),
          errorCode: frame.payload.readUInt8(offset + 4),
          severity: frame.payload.readUInt8(offset + 5),
          source: frame.payload.readUInt8(offset + 6),
          data0: frame.payload.readUInt32BE(offset + 8),
          data1: frame.payload.readUInt32BE(offset + 12),
        });
        offset += 16;
      }
      setErrorLog(entries);
      return entries;
    }
    return [];
  }, [sendCommand]);

  // ==========================================================================
  // Context Value
  // ==========================================================================

  const contextValue = {
    // Connection state
    connected,
    connecting,
    deviceHost,
    devicePort,
    setDeviceHost,
    setDevicePort,
    connect,
    disconnect,

    // Device data
    telemetry,
    stats,
    connections,
    errorLog,
    events,
    firmwareVersion,
    deviceStatus,
    deviceFeatures,

    // Command functions
    sendCommand,
    addConnection,
    deleteConnection,
    setFanSpeed,
    getErrorLog,
  };

  return (
    <DeviceContext.Provider value={contextValue}>
      {children}
    </DeviceContext.Provider>
  );
}

// ============================================================================
// Hook for consuming the device context
// ============================================================================

export function useDevice() {
  const context = useContext(DeviceContext);
  if (!context) {
    throw new Error('useDevice must be used within a DeviceProvider');
  }
  return context;
}

export default DeviceContext;
