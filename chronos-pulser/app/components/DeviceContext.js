// components/DeviceContext.js — React Context for device state sharing
// Provides device connection state, protocol engine, and action methods
// to all screens in the Chronos Pulser companion app
// Also includes helper hooks for common device operations

import React, { createContext, useContext, useState, useCallback, useMemo } from 'react';

// Default context value (used when no provider is mounted)
const defaultDeviceContext = {
  connected: false,
  deviceInfo: null,
  temperature: 25.0,
  pulseConfig: {
    frequency: 1000,
    amplitude: 250,
    enabled: false,
  },
  vgaGain: 10.0,
  calibrationValid: false,
  lastError: null,
  protocolEngine: null,
  connectDevice: async () => {},
  disconnectDevice: async () => {},
  updatePulseConfig: async (config) => {},
  updateVgaGain: async (gain) => {},
  startAcquisition: async (params) => null,
  runTdcCalibration: async () => null,
};

const DeviceContext = createContext(defaultDeviceContext);

// Provider component
export function DeviceProvider({ value, children }) {
  return (
    <DeviceContext.Provider value={value}>
      {children}
    </DeviceContext.Provider>
  );
}

// Hook for consuming device context
export function useDevice() {
  const context = useContext(DeviceContext);
  if (!context) {
    console.warn('useDevice must be used within a DeviceProvider');
    return defaultDeviceContext;
  }
  return context;
}

// Specialized hook: pulse generator control
export function usePulseControl() {
  const device = useDevice();
  const [localConfig, setLocalConfig] = useState(device.pulseConfig);

  const setFrequency = useCallback(async (freqHz) => {
    const newConfig = { ...localConfig, frequency: freqHz };
    setLocalConfig(newConfig);
    await device.updatePulseConfig(newConfig);
  }, [device, localConfig]);

  const setAmplitude = useCallback(async (amplitudeDac) => {
    const newConfig = { ...localConfig, amplitude: amplitudeDac };
    setLocalConfig(newConfig);
    await device.updatePulseConfig(newConfig);
  }, [device, localConfig]);

  const toggleEnabled = useCallback(async () => {
    const newConfig = { ...localConfig, enabled: !localConfig.enabled };
    setLocalConfig(newConfig);
    await device.updatePulseConfig(newConfig);
  }, [device, localConfig]);

  const fireSingle = useCallback(async () => {
    if (device.protocolEngine) {
      await device.protocolEngine.fireSinglePulse();
    }
  }, [device.protocolEngine]);

  return {
    config: localConfig,
    setFrequency,
    setAmplitude,
    toggleEnabled,
    fireSingle,
  };
}

// Specialized hook: temperature monitoring
export function useTemperature() {
  const device = useDevice();
  const [tempHistory, setTempHistory] = useState([]);
  const maxHistory = 100;

  // Track temperature changes
  const currentTemp = device.temperature;
  useMemo(() => {
    if (currentTemp !== undefined && currentTemp !== null) {
      setTempHistory(prev => {
        const updated = [...prev, { time: Date.now(), value: currentTemp }];
        if (updated.length > maxHistory) {
          return updated.slice(updated.length - maxHistory);
        }
        return updated;
      });
    }
  }, [currentTemp]);

  const getMinTemp = useCallback(() => {
    if (tempHistory.length === 0) return currentTemp;
    return Math.min(...tempHistory.map(t => t.value));
  }, [tempHistory, currentTemp]);

  const getMaxTemp = useCallback(() => {
    if (tempHistory.length === 0) return currentTemp;
    return Math.max(...tempHistory.map(t => t.value));
  }, [tempHistory, currentTemp]);

  const getAvgTemp = useCallback(() => {
    if (tempHistory.length === 0) return currentTemp;
    const sum = tempHistory.reduce((acc, t) => acc + t.value, 0);
    return sum / tempHistory.length;
  }, [tempHistory, currentTemp]);

  return {
    current: currentTemp,
    history: tempHistory,
    min: getMinTemp(),
    max: getMaxTemp(),
    avg: getAvgTemp(),
  };
}

// Specialized hook: acquisition control
export function useAcquisition() {
  const device = useDevice();
  const [acqState, setAcqState] = useState({
    running: false,
    complete: false,
    error: false,
    repCount: 0,
  });

  const start = useCallback(async (params) => {
    setAcqState({ running: true, complete: false, error: false, repCount: 0 });
    try {
      const result = await device.startAcquisition(params);
      // Poll for completion
      const pollInterval = setInterval(async () => {
        try {
          const status = await device.protocolEngine.getAcqStatus();
          setAcqState(status);
          if (status.complete || status.error) {
            clearInterval(pollInterval);
          }
        } catch (err) {
          clearInterval(pollInterval);
          setAcqState(prev => ({ ...prev, error: true }));
        }
      }, 500);
      return result;
    } catch (err) {
      setAcqState({ running: false, complete: false, error: true, repCount: 0 });
      throw err;
    }
  }, [device]);

  const stop = useCallback(async () => {
    if (device.protocolEngine) {
      await device.protocolEngine.stopAcquisition();
      setAcqState({ running: false, complete: true, error: false, repCount: 0 });
    }
  }, [device.protocolEngine]);

  return {
    state: acqState,
    start,
    stop,
  };
}

// Higher-order component for class components
export function withDevice(Component) {
  return function DeviceWrappedComponent(props) {
    const device = useDevice();
    return <Component {...props} device={device} />;
  };
}

export default DeviceContext;
