/**
 * @file    GrainGuardContext.js
 * @brief   Global state provider — manages probe data, alerts, WebSocket.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';

const GrainGuardContext = createContext(null);

export function GrainGuardProvider({ children }) {
  const [probes, setProbes] = useState({});
  const [alerts, setAlerts] = useState([]);
  const [connected, setConnected] = useState(false);
  const [selectedSiloId, setSelectedSiloId] = useState(null);
  const [ws, setWs] = useState(null);

  // Connect to gateway WebSocket
  useEffect(() => {
    const gatewayUrl = 'ws://grainguard-gateway.local:8080/ws';
    let socket;
    let reconnectTimer;

    const connect = () => {
      socket = new WebSocket(gatewayUrl);
      socket.onopen = () => {
        setConnected(true);
        console.log('[GrainGuard] Connected to gateway');
      };
      socket.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          handleMessage(msg);
        } catch (e) {
          console.warn('[GrainGuard] Bad message:', e);
        }
      };
      socket.onclose = () => {
        setConnected(false);
        console.log('[GrainGuard] Disconnected, retrying in 5s...');
        reconnectTimer = setTimeout(connect, 5000);
      };
      socket.onerror = (err) => {
        console.error('[GrainGuard] WS error:', err);
      };
      setWs(socket);
    };

    connect();
    return () => {
      clearTimeout(reconnectTimer);
      if (socket) socket.close();
    };
  }, []);

  const handleMessage = useCallback((msg) => {
    switch (msg.type) {
      case 'probe_update':
        setProbes(prev => ({
          ...prev,
          [msg.payload.serial]: {
            ...prev[msg.payload.serial],
            ...msg.payload,
            lastUpdate: Date.now(),
          },
        }));
        // Check for new alerts
        if (msg.payload.sri >= (msg.payload.criticalThreshold || 70)) {
          setAlerts(prev => [{
            id: `alert-${msg.payload.serial}-${Date.now()}`,
            serial: msg.payload.serial,
            siloName: msg.payload.siloName || `Silo ${msg.payload.serial}`,
            level: 'critical',
            sri: msg.payload.sri,
            message: `Critical: SRI ${msg.payload.sri} — immediate action required`,
            timestamp: Date.now(),
          }, ...prev].slice(0, 50));
        } else if (msg.payload.sri >= (msg.payload.cautionThreshold || 40)) {
          setAlerts(prev => [{
            id: `alert-${msg.payload.serial}-${Date.now()}`,
            serial: msg.payload.serial,
            siloName: msg.payload.siloName || `Silo ${msg.payload.serial}`,
            level: 'caution',
            sri: msg.payload.sri,
            message: `Caution: SRI ${msg.payload.sri} — monitor closely`,
            timestamp: Date.now(),
          }, ...prev].slice(0, 50));
        }
        break;

      case 'full_sync':
        setProbes(msg.payload.probes || {});
        break;

      case 'alert_ack':
        setAlerts(prev => prev.filter(a => a.id !== msg.payload.alertId));
        break;

      default:
        break;
    }
  }, []);

  const dismissAlert = useCallback((alertId) => {
    setAlerts(prev => prev.filter(a => a.id !== alertId));
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'ack_alert', payload: { alertId } }));
    }
  }, [ws]);

  const selectSilo = useCallback((siloId) => {
    setSelectedSiloId(siloId);
  }, []);

  const configureProbe = useCallback((serial, config) => {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({
        type: 'configure_probe',
        payload: { serial, ...config },
      }));
    }
  }, [ws]);

  const value = {
    probes,
    alerts,
    connected,
    selectedSiloId,
    selectSilo,
    dismissAlert,
    configureProbe,
    probeList: Object.values(probes),
  };

  return (
    <GrainGuardContext.Provider value={value}>
      {children}
    </GrainGuardContext.Provider>
  );
}

export function useGrainGuard() {
  const ctx = useContext(GrainGuardContext);
  if (!ctx) {
    throw new Error('useGrainGuard must be used within GrainGuardProvider');
  }
  return ctx;
}