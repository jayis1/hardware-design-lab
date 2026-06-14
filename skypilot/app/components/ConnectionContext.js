/**
 * SkyPilot — Connection Context
 * Manages BLE/TCP connection state and message passing
 */

import React, { createContext, useState, useRef, useCallback } from 'react';
import { NativeModules, NativeEventEmitter } from 'react-native';
import { FrameBuilder, FrameParser, Commands } from '../utils/protocol';

const { SkypilotBle } = NativeModules;

export const ConnectionContext = createContext({
  connected: false,
  connecting: false,
  connectionType: null, // 'ble' | 'tcp' | 'lte'
  sendMessage: () => {},
  onMessage: () => () => {},
  connect: () => {},
  disconnect: () => {},
  deviceInfo: null,
});

export function ConnectionProvider({ children }) {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [connectionType, setConnectionType] = useState(null);
  const [deviceInfo, setDeviceInfo] = useState(null);
  const listenersRef = useRef(new Set());
  const frameBuilder = useRef(new FrameBuilder());
  const frameParser = useRef(new FrameParser());
  const socketRef = useRef(null);

  const onMessage = useCallback((callback) => {
    listenersRef.current.add(callback);
    return () => listenersRef.current.delete(callback);
  }, []);

  const notifyListeners = useCallback((message) => {
    listenersRef.current.forEach((cb) => cb(message));
  }, []);

  const handleIncomingData = useCallback((rawData) => {
    const parsed = frameParser.current.parse(rawData);
    if (parsed) {
      notifyListeners(parsed);
    }
  }, [notifyListeners]);

  const sendMessage = useCallback((message) => {
    if (!connected) return;
    const frame = frameBuilder.current.build(
      message.cmd,
      message.payload
    );
    // Send via BLE or TCP
    if (connectionType === 'ble' && SkypilotBle) {
      SkypilotBle.write(frame);
    } else if (connectionType === 'tcp' && socketRef.current) {
      socketRef.current.write(frame);
    }
  }, [connected, connectionType]);

  const connect = useCallback(async (host, port) => {
    setConnecting(true);
    try {
      // Try BLE first, fall back to TCP
      if (SkypilotBle) {
        await SkypilotBle.connect('SkyPilot-FC');
        setConnectionType('ble');
      } else {
        // TCP connection via WebSocket (for LTE relay)
        const ws = new WebSocket(`ws://${host}:${port}`);
        ws.onopen = () => {
          setConnected(true);
          setConnectionType('tcp');
        };
        ws.onmessage = (event) => {
          handleIncomingData(event.data);
        };
        ws.onclose = () => {
          setConnected(false);
          setConnectionType(null);
        };
        ws.onerror = () => {
          setConnected(false);
        };
        socketRef.current = ws;
      }
      setConnected(true);
      setConnecting(false);

      // Request device info
      sendMessage({
        cmd: Commands.DEVICE_INFO_REQUEST,
        payload: {},
      });
    } catch (error) {
      setConnected(false);
      setConnecting(false);
      console.error('Connection failed:', error);
    }
  }, [handleIncomingData, sendMessage]);

  const disconnect = useCallback(() => {
    if (connectionType === 'ble' && SkypilotBle) {
      SkypilotBle.disconnect();
    } else if (connectionType === 'tcp' && socketRef.current) {
      socketRef.current.close();
    }
    setConnected(false);
    setConnectionType(null);
    setDeviceInfo(null);
    socketRef.current = null;
  }, [connectionType]);

  return (
    <ConnectionContext.Provider
      value={{
        connected,
        connecting,
        connectionType,
        sendMessage,
        onMessage,
        connect,
        disconnect,
        deviceInfo,
      }}
    >
      {children}
    </ConnectionContext.Provider>
  );
}