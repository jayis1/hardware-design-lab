/**
 * store.ts — global app state (React Context + useReducer)
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Holds the live node list, recent alerts, and the WebSocket subscription.
 * The Dashboard and Alert screens consume this context. Offline-first:
 * if the gateway is unreachable, the store serves the last cached state
 * from AsyncStorage.
 */

import React, { createContext, useContext, useReducer, useEffect, useCallback } from 'react';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { NodeSummary, AeEventLog, fetchNodes, fetchAeEvents, subscribeLive } from './api';
import { DecodedUplink, FLAG_URGENT, FLAG_AE_ALARM, aeLabel } from './proto';

interface State {
  nodes: NodeSummary[];
  alerts: AlertEntry[];
  lastSync: number;
  liveConnected: boolean;
}

export interface AlertEntry {
  id: string;
  nodeId: string;
  unixTime: number;
  type: 'wire-break' | 'dt-alarm' | 'low-battery' | 'cal-lost' | 'node-silent';
  message: string;
  severity: 'info' | 'warning' | 'critical';
}

type Action =
  | { type: 'SET_NODES'; nodes: NodeSummary[] }
  | { type: 'ADD_ALERT'; alert: AlertEntry }
  | { type: 'SET_LIVE'; connected: boolean }
  | { type: 'SET_SYNC'; time: number }
  | { type: 'CLEAR_ALERTS' };

const initialState: State = {
  nodes: [],
  alerts: [],
  lastSync: 0,
  liveConnected: false,
};

function reducer(state: State, action: Action): State {
  switch (action.type) {
    case 'SET_NODES':
      return { ...state, nodes: action.nodes, lastSync: Date.now() };
    case 'ADD_ALERT': {
      const alerts = [action.alert, ...state.alerts].slice(0, 200);
      return { ...state, alerts };
    }
    case 'SET_LIVE':
      return { ...state, liveConnected: action.connected };
    case 'SET_SYNC':
      return { ...state, lastSync: action.time };
    case 'CLEAR_ALERTS':
      return { ...state, alerts: [] };
    default:
      return state;
  }
}

interface StoreContextValue {
  state: State;
  refreshNodes: () => Promise<void>;
  clearAlerts: () => void;
}

const StoreContext = createContext<StoreContextValue | null>(null);

export function StoreProvider({ children }: { children: React.ReactNode }) {
  const [state, dispatch] = useReducer(reducer, initialState);

  const refreshNodes = useCallback(async () => {
    const nodes = await fetchNodes();
    if (nodes.length > 0) {
      dispatch({ type: 'SET_NODES', nodes });
      // persist to AsyncStorage for offline use
      try {
        await AsyncStorage.setItem('@tg_nodes', JSON.stringify(nodes));
      } catch { /* ignore */ }
      // generate alerts for nodes in alarm state
      for (const n of nodes) {
        if (n.flags & FLAG_URGENT || n.flags & FLAG_AE_ALARM) {
          dispatch({
            type: 'ADD_ALERT',
            alert: {
              id: `${n.nodeId}-${n.lastSeen}`,
              nodeId: n.nodeId,
              unixTime: n.lastSeen,
              type: 'wire-break',
              message: `Node ${n.label}: ${n.aeCount} AE event(s), possible wire break`,
              severity: 'critical',
            },
          });
        }
        if (n.batteryPct < 15) {
          dispatch({
            type: 'ADD_ALERT',
            alert: {
              id: `${n.nodeId}-batt-${n.lastSeen}`,
              nodeId: n.nodeId,
              unixTime: n.lastSeen,
              type: 'low-battery',
              message: `Node ${n.label}: battery at ${n.batteryPct.toFixed(0)}%`,
              severity: 'warning',
            },
          });
        }
      }
    } else {
      // try to load cached nodes
      try {
        const cached = await AsyncStorage.getItem('@tg_nodes');
        if (cached) {
          dispatch({ type: 'SET_NODES', nodes: JSON.parse(cached) as NodeSummary[] });
        }
      } catch { /* ignore */ }
    }
  }, []);

  useEffect(() => {
    // initial fetch
    refreshNodes();
    // periodic refresh every 60 s
    const interval = setInterval(refreshNodes, 60000);
    // live WebSocket
    const unsub = subscribeLive((uplink: DecodedUplink) => {
      dispatch({ type: 'SET_LIVE', connected: true });
      // update the matching node in the list
      if (uplink.header.nodeId) {
        dispatch({
          type: 'ADD_ALERT',
          alert: {
            id: `${uplink.header.nodeId}-${uplink.header.seq}`,
            nodeId: uplink.header.nodeId,
            unixTime: uplink.header.unixTime,
            type: uplink.packet.urgent ? 'wire-break' : 'info',
            message: `Uplink from ${uplink.header.nodeId}: T_mag=${uplink.packet.tMagKn.toFixed(1)} kN, T_vib=${uplink.packet.tVibKn.toFixed(1)} kN`,
            severity: uplink.packet.urgent ? 'critical' : 'info',
          },
        });
      }
      if (uplink.ae && uplink.ae.classification === 3) {
        dispatch({
          type: 'ADD_ALERT',
          alert: {
            id: `ae-${uplink.header.nodeId}-${uplink.ae.unixTime}`,
            nodeId: uplink.header.nodeId,
            unixTime: uplink.ae.unixTime,
            type: 'wire-break',
            message: `Wire break detected on ${uplink.header.nodeId}: score ${uplink.ae.score}, ${aeLabel(uplink.ae.classification)}`,
            severity: 'critical',
          },
        });
      }
    });
    return () => { clearInterval(interval); unsub(); };
  }, [refreshNodes]);

  const clearAlerts = useCallback(() => dispatch({ type: 'CLEAR_ALERTS' }), []);

  return (
    <StoreContext.Provider value={{ state, refreshNodes, clearAlerts }}>
      {children}
    </StoreContext.Provider>
  );
}

export function useStore(): StoreContextValue {
  const ctx = useContext(StoreContext);
  if (!ctx) throw new Error('useStore must be used within StoreProvider');
  return ctx;
}