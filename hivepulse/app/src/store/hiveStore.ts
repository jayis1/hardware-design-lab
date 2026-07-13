/**
 * HivePulse Redux store
 * Author: jayis1
 * License: MIT
 */

import { createStore, applyMiddleware } from 'redux';
import thunk from 'redux-thunk';
import { ColonyState, HiveData, AlertData } from '../types';

// ---- State ----
export interface AppState {
  hives: HiveData[];
  activeHiveId: string | null;
  alerts: AlertData[];
  connected: boolean;
  bleScanning: boolean;
  loading: boolean;
  error: string | null;
  lastSync: number;
}

const initialState: AppState = {
  hives: [],
  activeHiveId: null,
  alerts: [],
  connected: false,
  bleScanning: false,
  loading: false,
  error: null,
  lastSync: 0,
};

// ---- Actions ----
const SET_HIVES = 'SET_HIVES';
const UPDATE_HIVE = 'UPDATE_HIVE';
const SET_ACTIVE_HIVE = 'SET_ACTIVE_HIVE';
const ADD_ALERT = 'ADD_ALERT';
const CLEAR_ALERT = 'CLEAR_ALERT';
const SET_CONNECTED = 'SET_CONNECTED';
const SET_SCANNING = 'SET_SCANNING';
const SET_LOADING = 'SET_LOADING';
const SET_ERROR = 'SET_ERROR';
const SET_LAST_SYNC = 'SET_LAST_SYNC';

interface Action {
  type: string;
  payload?: any;
}

// ---- Reducer ----
function hiveReducer(state: AppState = initialState, action: Action): AppState {
  switch (action.type) {
  case SET_HIVES:
    return { ...state, hives: action.payload };

  case UPDATE_HIVE: {
    const updated: HiveData = action.payload;
    const existing = state.hives.findIndex(h => h.id === updated.id);
    if (existing >= 0) {
      const hives = [...state.hives];
      hives[existing] = updated;
      return { ...state, hives };
    }
    return { ...state, hives: [...state.hives, updated] };
  }

  case SET_ACTIVE_HIVE:
    return { ...state, activeHiveId: action.payload };

  case ADD_ALERT:
    return { ...state, alerts: [...state.alerts, action.payload] };

  case CLEAR_ALERT:
    return {
      ...state,
      alerts: state.alerts.filter(a => a.hiveId !== action.payload),
    };

  case SET_CONNECTED:
    return { ...state, connected: action.payload };

  case SET_SCANNING:
    return { ...state, bleScanning: action.payload };

  case SET_LOADING:
    return { ...state, loading: action.payload };

  case SET_ERROR:
    return { ...state, error: action.payload };

  case SET_LAST_SYNC:
    return { ...state, lastSync: action.payload };

  default:
    return state;
  }
}

// ---- Action Creators ----
export const setHives = (hives: HiveData[]) => ({
  type: SET_HIVES, payload: hives,
});

export const updateHive = (hive: HiveData) => ({
  type: UPDATE_HIVE, payload: hive,
});

export const setActiveHive = (id: string) => ({
  type: SET_ACTIVE_HIVE, payload: id,
});

export const addAlert = (alert: AlertData) => ({
  type: ADD_ALERT, payload: alert,
});

export const clearAlert = (hiveId: string) => ({
  type: CLEAR_ALERT, payload: hiveId,
});

export const setConnected = (connected: boolean) => ({
  type: SET_CONNECTED, payload: connected,
});

export const setScanning = (scanning: boolean) => ({
  type: SET_SCANNING, payload: scanning,
});

// ---- Thunk: Fetch hives from REST API ----
export const fetchHives = () => async (dispatch: any) => {
  dispatch({ type: SET_LOADING, payload: true });
  dispatch({ type: SET_ERROR, payload: null });

  try {
    const response = await fetch('https://api.hivepulse.io/v1/hives', {
      headers: { 'Authorization': 'Bearer ' + getAuthToken() },
    });
    if (!response.ok) throw new Error(`API error: ${response.status}`);
    const hives: HiveData[] = await response.json();
    dispatch(setHives(hives));
    dispatch({ type: SET_LAST_SYNC, payload: Date.now() });

    // Check for alerts
    for (const hive of hives) {
      if (hive.swarmAlert) {
        dispatch(addAlert({
          hiveId: hive.id,
          hiveName: hive.name,
          alertType: 'swarm',
          severity: 'critical',
          message: `${hive.name} is preparing to swarm. Action needed within 3 days.`,
          recommendedAction: 'Perform artificial split or add honey supers.',
          timestamp: hive.lastUpdated,
          confidence: hive.confidence,
        }));
      }
      if (hive.queenlessAlert) {
        dispatch(addAlert({
          hiveId: hive.id,
          hiveName: hive.name,
          alertType: 'queenless',
          severity: 'critical',
          message: `${hive.name} queenless roar detected. Queen may be lost.`,
          recommendedAction: 'Inspect within 48h. Introduce new queen or merge colony.',
          timestamp: hive.lastUpdated,
          confidence: hive.confidence,
        }));
      }
      if (hive.colonyState === ColonyState.VarroaHigh && hive.confidence > 0.8) {
        dispatch(addAlert({
          hiveId: hive.id,
          hiveName: hive.name,
          alertType: 'varroa',
          severity: 'warning',
          message: `${hive.name} high Varroa mite load detected.`,
          recommendedAction: 'Treat within 7 days. Confirm with sugar shake test.',
          timestamp: hive.lastUpdated,
          confidence: hive.confidence,
        }));
      }
    }
  } catch (err: any) {
    dispatch({ type: SET_ERROR, payload: err.message });
  } finally {
    dispatch({ type: SET_LOADING, payload: false });
  }
};

// ---- Auth token (stored in AsyncStorage in production) ----
function getAuthToken(): string {
  return 'demo-token-jayis1';
}

// ---- Create Store ----
export const store = createStore(hiveReducer, initialState, applyMiddleware(thunk));