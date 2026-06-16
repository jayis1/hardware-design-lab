/**
 * DeviceConnection.js — Nebula Matrix Device Connection Component
 *
 * Reusable header component for connecting to a Nebula Matrix device.
 * Displays connection status, device info, and provides connect/disconnect UI.
 *
 * Used in the headerRight of all navigation screens.
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  Modal,
  TextInput,
  ActivityIndicator,
  Platform,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

/* =========================================================================
 * Constants
 * ========================================================================= */

const DEFAULT_HOST = '192.168.1.100';
const DEFAULT_PORT = '6454';
const RECENT_HOSTS_KEY = 'nebula_recent_hosts';

/* =========================================================================
 * DeviceConnection Component
 * ========================================================================= */

const DeviceConnection = ({
  connected,
  deviceInfo,
  isScanning,
  error,
  onConnect,
  onDisconnect,
}) => {
  const [modalVisible, setModalVisible] = useState(false);
  const [host, setHost] = useState(DEFAULT_HOST);
  const [port, setPort] = useState(DEFAULT_PORT);
  const [recentHosts, setRecentHosts] = useState([
    { host: '192.168.1.100', port: '6454', label: 'Default (Art-Net)' },
    { host: '192.168.1.100', port: '5568', label: 'sACN Port' },
    { host: '192.168.2.50', port: '6454', label: 'Stage Left' },
  ]);

  /* =====================================================================
   * Connect handler
   * ===================================================================== */

  const handleConnect = useCallback(() => {
    const portNum = parseInt(port, 10);
    if (isNaN(portNum) || portNum < 1 || portNum > 65535) {
      return;
    }

    /* Add to recent hosts */
    setRecentHosts(prev => {
      const exists = prev.find(h => h.host === host && h.port === port);
      if (exists) return prev;
      return [{ host, port, label: `${host}:${port}` }, ...prev].slice(0, 10);
    });

    setModalVisible(false);
    onConnect(host, portNum);
  }, [host, port, onConnect]);

  /* =====================================================================
   * Disconnect handler
   * ===================================================================== */

  const handleDisconnect = useCallback(() => {
    onDisconnect();
  }, [onDisconnect]);

  /* =====================================================================
   * Quick connect to recent host
   * ===================================================================== */

  const quickConnect = useCallback((recentHost, recentPort) => {
    setHost(recentHost);
    setPort(recentPort);
    setModalVisible(false);
    onConnect(recentHost, parseInt(recentPort, 10));
  }, [onConnect]);

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <View style={styles.container}>
      {/* Connection status button */}
      <TouchableOpacity
        style={[
          styles.statusButton,
          connected ? styles.statusConnected : styles.statusDisconnected,
        ]}
        onPress={() => setModalVisible(true)}
      >
        <Icon
          name={connected ? 'lan-connect' : 'lan-disconnect'}
          size={16}
          color={connected ? '#4CAF50' : '#FF6B6B'}
        />
        <Text style={[
          styles.statusText,
          connected ? styles.statusTextConnected : styles.statusTextDisconnected,
        ]}>
          {connected ? 'Connected' : isScanning ? 'Scanning...' : 'Connect'}
        </Text>
      </TouchableOpacity>

      {/* Device info tooltip (when connected) */}
      {connected && deviceInfo && (
        <View style={styles.infoTooltip}>
          <Text style={styles.infoText} numberOfLines={1}>
            {deviceInfo.deviceName || 'Nebula Matrix'}
          </Text>
          <Text style={styles.infoSubtext} numberOfLines={1}>
            v{deviceInfo.version || '?.?'} | {deviceInfo.matrixWidth || 256}×{deviceInfo.matrixHeight || 256}
          </Text>
        </View>
      )}

      {/* Error indicator */}
      {error && !connected && (
        <View style={styles.errorIndicator}>
          <Icon name="alert-circle" size={12} color="#FF6B6B" />
        </View>
      )}

      {/* Connection Modal */}
      <Modal
        visible={modalVisible}
        animationType="slide"
        transparent={true}
        onRequestClose={() => setModalVisible(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            {/* Header */}
            <View style={styles.modalHeader}>
              <Text style={styles.modalTitle}>Connect to Device</Text>
              <TouchableOpacity onPress={() => setModalVisible(false)}>
                <Icon name="close" size={24} color="#888" />
              </TouchableOpacity>
            </View>

            {/* Current status */}
            {connected && (
              <View style={styles.connectedBanner}>
                <Icon name="lan-connect" size={16} color="#4CAF50" />
                <Text style={styles.connectedText}>
                  Currently connected to {deviceInfo?.deviceName || 'device'}
                </Text>
                <TouchableOpacity
                  style={styles.disconnectButton}
                  onPress={handleDisconnect}
                >
                  <Text style={styles.disconnectText}>Disconnect</Text>
                </TouchableOpacity>
              </View>
            )}

            {/* Manual entry */}
            <Text style={styles.fieldLabel}>Host / IP Address</Text>
            <TextInput
              style={styles.textInput}
              value={host}
              onChangeText={setHost}
              placeholder="192.168.1.100"
              placeholderTextColor="#555"
              autoCapitalize="none"
              autoCorrect={false}
              keyboardType="decimal-pad"
            />

            <Text style={styles.fieldLabel}>Port</Text>
            <TextInput
              style={styles.textInput}
              value={port}
              onChangeText={setPort}
              placeholder="6454"
              placeholderTextColor="#555"
              keyboardType="number-pad"
              maxLength={5}
            />

            {/* Connect button */}
            <TouchableOpacity
              style={styles.connectButton}
              onPress={handleConnect}
              disabled={isScanning}
            >
              {isScanning ? (
                <ActivityIndicator size="small" color="#000" />
              ) : (
                <Icon name="lan-connect" size={18} color="#000" />
              )}
              <Text style={styles.connectButtonText}>
                {isScanning ? 'Connecting...' : 'Connect'}
              </Text>
            </TouchableOpacity>

            {/* Recent hosts */}
            {recentHosts.length > 0 && (
              <View style={styles.recentSection}>
                <Text style={styles.recentTitle}>Recent Devices</Text>
                {recentHosts.map((item, index) => (
                  <TouchableOpacity
                    key={index}
                    style={styles.recentItem}
                    onPress={() => quickConnect(item.host, item.port)}
                  >
                    <Icon name="history" size={16} color="#888" />
                    <View style={styles.recentTextCol}>
                      <Text style={styles.recentHost}>{item.host}:{item.port}</Text>
                      <Text style={styles.recentLabel}>{item.label}</Text>
                    </View>
                    <Icon name="chevron-right" size={16} color="#666" />
                  </TouchableOpacity>
                ))}
              </View>
            )}

            {/* Connection tips */}
            <View style={styles.tipsSection}>
              <Icon name="information" size={16} color="#888" />
              <Text style={styles.tipsText}>
                Connect via Ethernet (Art-Net port 6454, sACN port 5568) or USB RNDIS (port 6454).
                Default IP: 192.168.1.100
              </Text>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
};

/* =========================================================================
 * Styles
 * ========================================================================= */

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 8,
  },
  statusButton: {
    flexDirection: 'row',
    alignItems: 'center',
    borderRadius: 16,
    paddingVertical: 5,
    paddingHorizontal: 10,
    gap: 5,
  },
  statusConnected: {
    backgroundColor: '#1A3A1A',
    borderWidth: 1,
    borderColor: '#4CAF5044',
  },
  statusDisconnected: {
    backgroundColor: '#3A1A1A',
    borderWidth: 1,
    borderColor: '#FF6B6B44',
  },
  statusText: {
    fontSize: 11,
    fontWeight: '700',
  },
  statusTextConnected: {
    color: '#4CAF50',
  },
  statusTextDisconnected: {
    color: '#FF6B6B',
  },
  infoTooltip: {
    marginLeft: 6,
    maxWidth: 120,
  },
  infoText: {
    color: '#E0E0E0',
    fontSize: 10,
    fontWeight: '600',
  },
  infoSubtext: {
    color: '#888',
    fontSize: 9,
  },
  errorIndicator: {
    marginLeft: 4,
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'flex-end',
  },
  modalContent: {
    backgroundColor: '#16213E',
    borderTopLeftRadius: 20,
    borderTopRightRadius: 20,
    padding: 20,
    paddingBottom: Platform.OS === 'ios' ? 40 : 20,
    maxHeight: '80%',
  },
  modalHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 16,
  },
  modalTitle: {
    color: '#E0E0E0',
    fontSize: 18,
    fontWeight: '700',
  },
  connectedBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1A3A1A',
    borderRadius: 8,
    padding: 10,
    marginBottom: 16,
    gap: 8,
  },
  connectedText: {
    color: '#4CAF50',
    fontSize: 12,
    flex: 1,
  },
  disconnectButton: {
    backgroundColor: '#FF6B6B',
    borderRadius: 6,
    paddingVertical: 4,
    paddingHorizontal: 10,
  },
  disconnectText: {
    color: '#fff',
    fontSize: 11,
    fontWeight: '700',
  },
  fieldLabel: {
    color: '#AAA',
    fontSize: 12,
    fontWeight: '600',
    textTransform: 'uppercase',
    marginTop: 12,
    marginBottom: 6,
  },
  textInput: {
    backgroundColor: '#0F3460',
    borderRadius: 8,
    padding: 12,
    color: '#E0E0E0',
    fontSize: 15,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    borderWidth: 1,
    borderColor: '#333',
  },
  connectButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#00E5FF',
    borderRadius: 10,
    padding: 14,
    marginTop: 16,
    gap: 8,
  },
  connectButtonText: {
    color: '#000',
    fontSize: 15,
    fontWeight: '700',
  },
  recentSection: {
    marginTop: 20,
  },
  recentTitle: {
    color: '#888',
    fontSize: 12,
    fontWeight: '600',
    textTransform: 'uppercase',
    marginBottom: 8,
  },
  recentItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#0F3460',
    borderRadius: 8,
    padding: 10,
    marginBottom: 6,
    gap: 8,
  },
  recentTextCol: {
    flex: 1,
  },
  recentHost: {
    color: '#E0E0E0',
    fontSize: 13,
    fontWeight: '600',
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  recentLabel: {
    color: '#888',
    fontSize: 11,
    marginTop: 1,
  },
  tipsSection: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    marginTop: 16,
    gap: 8,
  },
  tipsText: {
    color: '#888',
    fontSize: 11,
    flex: 1,
    lineHeight: 16,
  },
});

export default DeviceConnection;
