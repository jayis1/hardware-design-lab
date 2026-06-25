/**
 * store.js — Redux store with persist
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import { createStore, combineReducers } from 'redux';
import { persistStore, persistReducer } from 'redux-persist';
import AsyncStorage from '@react-native-async-storage/async-storage';

// ---- Pods reducer ----
const podsInitialState = { pods: [], lastSync: null };

function podsReducer(state = podsInitialState, action) {
  switch (action.type) {
    case 'PODS_SET_ALL':
      return { ...state, pods: action.payload, lastSync: Date.now() };
    case 'PODS_UPDATE_ONE': {
      const idx = state.pods.findIndex(p => p.id === action.payload.id);
      if (idx < 0) return { ...state, pods: [...state.pods, action.payload] };
      const pods = [...state.pods];
      pods[idx] = { ...pods[idx], ...action.payload };
      return { ...state, pods, lastSync: Date.now() };
    }
    case 'PODS_UPDATE_SVI': {
      const idx = state.pods.findIndex(p => p.id === action.payload.podId);
      if (idx < 0) return state;
      const pods = [...state.pods];
      const sviHistory = [...(pods[idx].sviHistory || []), { ts: Date.now(), svi: action.payload.svi }];
      pods[idx] = { ...pods[idx], svi: action.payload.svi, sviHistory: sviHistory.slice(-1440) };
      return { ...state, pods };
    }
    default:
      return state;
  }
}

// ---- Events reducer ----
const eventsInitialState = { events: [], unreadCount: 0 };

function eventsReducer(state = eventsInitialState, action) {
  switch (action.type) {
    case 'EVENTS_ADD': {
      const events = [...state.events, ...action.payload].slice(-10000);
      return { ...state, events, unreadCount: state.unreadCount + action.payload.length };
    }
    case 'EVENTS_CLEAR':
      return { ...state, events: [] };
    case 'EVENTS_MARK_READ':
      return { ...state, unreadCount: 0 };
    case 'EVENTS_DELETE':
      return { ...state, events: state.events.filter(e => e.id !== action.payload) };
    default:
      return state;
  }
}

// ---- Alerts reducer ----
const alertsInitialState = { alerts: [] };

function alertsReducer(state = alertsInitialState, action) {
  switch (action.type) {
    case 'ALERTS_ADD':
      return { ...state, alerts: [...state.alerts, ...action.payload].slice(-500) };
    case 'ALERTS_DISMISS':
      return { ...state, alerts: state.alerts.filter(a => a.id !== action.payload) };
    case 'ALERTS_CLEAR':
      return { ...state, alerts: [] };
    default:
      return state;
  }
}

// ---- Settings reducer ----
const settingsInitialState = {
  cadence: 30,            // minutes
  listenWindow: 30,      // seconds
  sensitivity: 3.0,      // threshold factor (sigma)
  loraRegion: 'EU868',
  loraDevEUI: '',
  loraAppKey: '',
  units: 'metric',
  theme: 'light',
  author: 'jayis1',
};

function settingsReducer(state = settingsInitialState, action) {
  switch (action.type) {
    case 'SETTINGS_UPDATE':
      return { ...state, ...action.payload };
    case 'SETTINGS_RESET':
      return settingsInitialState;
    default:
      return state;
  }
}

// ---- Combine & persist ----
const rootReducer = combineReducers({
  pods: podsReducer,
  events: eventsReducer,
  alerts: alertsReducer,
  settings: settingsReducer,
});

const persistConfig = {
  key: 'soundloom-root',
  storage: AsyncStorage,
  whitelist: ['pods', 'events', 'settings'],
};

const persistedReducer = persistReducer(persistConfig, rootReducer);

export const store = createStore(persistedReducer);
export const persistor = persistStore(store);