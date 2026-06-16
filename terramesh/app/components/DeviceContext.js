/**
 * Terramesh — Device Context
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * React context provider for managing Terramesh node state, including
 * sensor data, classification status, and mesh connectivity.
 */

import React, { createContext, useContext, useState, useCallback, useEffect } from 'react';
import { Platform } from 'react-native';
import { protocol } from '../utils/protocol';

const DeviceContext = createContext();

const DEFAULT_NODE = {
  id: 'TM-0000',
  name: 'Gateway',
  status: 'offline',
  battery: 0,
  rssi: 0,
  snr: 0,
  lastSeen: null,
  classification: 0,
  sensors: {
    porePressureShallow: 0,
    porePressureDeep: 0,
    moisture: 0,
    tiltX: 0,
    tiltY: 0,
    accelX: 0,
    accelY: 0,
    accelZ: 0,
    temperature: 0,
    pressure: 0,
  },
  history: [],
};

export const DeviceProvider = ({ children }) => {
  const [nodes, setNodes] = useState({});
  const [gatewayConnected, setGatewayConnected] = useState(false);
  const [alerts, setAlerts] = useState([]);
  const [isScanning, setIsScanning] = useState(false);

  const addAlert = useCallback((alert) => {
    setAlerts(prev => {
      const updated = [alert, ...prev];
      return updated.slice(0, 100); // Keep last 100 alerts
    });
  }, []);

  const updateNode = useCallback((nodeId, data) => {
    setNodes(prev => {
      const existing = prev[nodeId] || { ...DEFAULT_NODE, id: nodeId };
      const updated = {
        ...existing,
        ...data,
        lastSeen: new Date().toISOString(),
        status: 'online',
      };

      // Add to history (keep last 500 samples)
      if (data.sensors) {
        const historyEntry = {
          timestamp: new Date().toISOString(),
          ...data.sensors,
          classification: data.classification || 0,
        };
        updated.history = [...(existing.history || []), historyEntry].slice(-500);
      }

      return { ...prev, [nodeId]: updated };
    });
  }, []);

  const processTelemetry = useCallback((packet) => {
    const nodeId = `TM-${packet.src_addr.toString(16).padStart(4, '0').toUpperCase()}`;

    const sensorData = protocol.parseSensorData(packet.payload);
    if (!sensorData) return;

    const classification = sensorData.classification;
    const nodeData = {
      sensors: {
        porePressureShallow: sensorData.pore_press_shallow * 0.01,
        porePressureDeep: sensorData.pore_press_deep * 0.01,
        moisture: sensorData.moisture * 0.01,
        tiltX: sensorData.tilt_x * 0.001,
        tiltY: sensorData.tilt_y * 0.001,
        accelX: sensorData.accel_x,
        accelY: sensorData.accel_y,
        accelZ: sensorData.accel_z,
        temperature: sensorData.temperature * 0.01,
        pressure: sensorData.pressure,
      },
      classification,
      battery: sensorData.battery_mv * 20,
      rssi: packet.rssi || 0,
      snr: packet.snr || 0,
    };

    updateNode(nodeId, nodeData);

    // Generate alert for anomalies
    if (classification === 2) {
      addAlert({
        id: `${nodeId}-${Date.now()}`,
        nodeId,
        type: 'CRITICAL',
        message: `CRITICAL: ${nodeId} — Rapid ground movement detected!`,
        timestamp: new Date().toISOString(),
        acknowledged: false,
      });
    } else if (classification === 1) {
      addAlert({
        id: `${nodeId}-${Date.now()}`,
        nodeId,
        type: 'WARNING',
        message: `WARNING: ${nodeId} — Elevated pore pressure / tilt detected`,
        timestamp: new Date().toISOString(),
        acknowledged: false,
      });
    }
  }, [updateNode, addAlert]);

  const acknowledgeAlert = useCallback((alertId) => {
    setAlerts(prev =>
      prev.map(a => a.id === alertId ? { ...a, acknowledged: true } : a)
    );
  }, []);

  const clearAlerts = useCallback(() => {
    setAlerts([]);
  }, []);

  const getNodeCount = useCallback(() => {
    return Object.keys(nodes).length;
  }, [nodes]);

  const getOnlineCount = useCallback(() => {
    const fiveMinutesAgo = Date.now() - 5 * 60 * 1000;
    return Object.values(nodes).filter(n => {
      const lastSeen = new Date(n.lastSeen).getTime();
      return lastSeen > fiveMinutesAgo;
    }).length;
  }, [nodes]);

  const getCriticalCount = useCallback(() => {
    return Object.values(nodes).filter(n => n.classification === 2).length;
  }, [nodes]);

  const getWarningCount = useCallback(() => {
    return Object.values(nodes).filter(n => n.classification === 1).length;
  }, [nodes]);

  const value = {
    nodes,
    alerts,
    gatewayConnected,
    isScanning,
    setGatewayConnected,
    setIsScanning,
    updateNode,
    processTelemetry,
    addAlert,
    acknowledgeAlert,
    clearAlerts,
    getNodeCount,
    getOnlineCount,
    getCriticalCount,
    getWarningCount,
  };

  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
};

export const useDevices = () => {
  const context = useContext(DeviceContext);
  if (!context) {
    throw new Error('useDevices must be used within a DeviceProvider');
  }
  return context;
};

export default DeviceContext;
