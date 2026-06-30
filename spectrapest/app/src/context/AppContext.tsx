/**
 * AppContext.tsx — Global state management for SpectraPest Field app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { createContext, useContext, useState, useEffect, useCallback } from 'react';
import { DetectionEvent, SpectraPestNode, GatewayStatus, AlertConfig } from './types';
import { SpectraPestAPI } from './api';

interface AppContextValue {
  api: SpectraPestAPI;
  gatewayUrl: string;
  setGatewayUrl: (url: string) => void;
  gatewayStatus: GatewayStatus | null;
  nodes: SpectraPestNode[];
  detections: DetectionEvent[];
  alerts: AlertConfig[];
  loading: boolean;
  error: string | null;
  refreshData: () => Promise<void>;
  lastUpdate: Date | null;
}

const AppContext = createContext<AppContextValue | undefined>(undefined);

export function AppProvider({ children }: { children: React.ReactNode }) {
  const [gatewayUrl, setGatewayUrl] = useState('http://192.168.4.1');
  const [api, setApi] = useState(new SpectraPestAPI(gatewayUrl));
  const [gatewayStatus, setGatewayStatus] = useState<GatewayStatus | null>(null);
  const [nodes, setNodes] = useState<SpectraPestNode[]>([]);
  const [detections, setDetections] = useState<DetectionEvent[]>([]);
  const [alerts, setAlerts] = useState<AlertConfig[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);

  useEffect(() => {
    setApi(new SpectraPestAPI(gatewayUrl));
  }, [gatewayUrl]);

  const refreshData = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const [status, nodeList, detList, alertList] = await Promise.all([
        api.getGatewayStatus(),
        api.getNodes(),
        api.getDetections(),
        api.getAlerts(),
      ]);
      setGatewayStatus(status);
      setNodes(nodeList);
      setDetections(detList);
      setAlerts(alertList);
      setLastUpdate(new Date());
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
    } finally {
      setLoading(false);
    }
  }, [api]);

  useEffect(() => {
    refreshData();
    const interval = setInterval(refreshData, 30000); /* Refresh every 30s */
    return () => clearInterval(interval);
  }, [refreshData]);

  return (
    <AppContext.Provider
      value={{
        api,
        gatewayUrl,
        setGatewayUrl,
        gatewayStatus,
        nodes,
        detections,
        alerts,
        loading,
        error,
        refreshData,
        lastUpdate,
      }}
    >
      {children}
    </AppContext.Provider>
  );
}

export function useApp(): AppContextValue {
  const ctx = useContext(AppContext);
  if (!ctx) throw new Error('useApp must be used within AppProvider');
  return ctx;
}