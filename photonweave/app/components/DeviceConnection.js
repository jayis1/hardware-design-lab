/**
 * DeviceConnection.js — Connection Status & Management Component
 *
 * Reusable header component that displays device connection status,
 * provides connect/disconnect controls, and shows device info.
 * Used in the navigation header of the PhotonWeave app.
 */

import React, { useState, useCallback } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Modal,
  TextInput, Alert, ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { THEME } from '../App';

//=============================================================================
// DeviceConnection Component
//=============================================================================
export default function DeviceConnection({
  isConnected,
  deviceInfo,
  onConnect,
  onDisconnect,
}) {
  const [modalVisible, setModalVisible] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [connectionType, setConnectionType] = useState('usb');
  const [tcpHost, setTcpHost] = useState('192.168.1.100');
  const [tcpPort, setTcpPort] = useState('9999');

  //===========================================================================
  // Connect Handler
  //===========================================================================
  const handleConnectPress = useCallback(async () => {
    setConnecting(true);
    try {
      const params = connectionType === 'usb'
        ? { type: 'usb' }
        : { type: 'tcp', host: tcpHost, port: parseInt(tcpPort) };

      await onConnect(params);
      setModalVisible(false);
    } catch (error) {
      Alert.alert('Connection Error', error.message);
    } finally {
      setConnecting(false);
    }
  }, [connectionType, tcpHost, tcpPort, onConnect]);

  //===========================================================================
  // Disconnect Handler
  //===========================================================================
  const handleDisconnectPress = useCallback(async () => {
    try {
      await onDisconnect();
    } catch (error) {
      console.warn('Disconnect error:', error);
    }
  }, [onDisconnect]);

  //===========================================================================
  // Render
  //===========================================================================
  return (
    <View style={styles.container}>
      {/* ── Connection Status Button ── */}
      <TouchableOpacity
        style={[
          styles.statusButton,
          isConnected ? styles.statusConnected : styles.statusDisconnected,
        ]}
        onPress={() => {
          if (isConnected) {
            handleDisconnectPress();
          } else {
            setModalVisible(true);
          }
        }}
      >
        <View style={[
          styles.statusDot,
          { backgroundColor: isConnected ? THEME.success : THEME.error },
        ]} />
        <Text style={styles.statusText}>
          {isConnected ? 'Connected' : 'Disconnected'}
        </Text>
        <Icon
          name={isConnected ? 'close-circle' : 'plus-circle'}
          size={18}
          color={isConnected ? THEME.error : THEME.primary}
        />
      </TouchableOpacity>

      {/* ── Device Info Tooltip (when connected) ── */}
      {isConnected && deviceInfo && (
        <View style={styles.infoTooltip}>
          <Text style={styles.infoText} numberOfLines={1}>
            {deviceInfo.productName || 'PhotonWeave'} — SN: {deviceInfo.serialNumber || 'N/A'}
          </Text>
        </View>
      )}

      {/* ── Connection Modal ── */}
      <Modal
        visible={modalVisible}
        transparent
        animationType="slide"
        onRequestClose={() => setModalVisible(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Connect to PhotonWeave</Text>

            {/* Connection Type Selector */}
            <Text style={styles.modalLabel}>Connection Type</Text>
            <View style={styles.typeRow}>
              <TouchableOpacity
                style={[
                  styles.typeButton,
                  connectionType === 'usb' && styles.typeButtonActive,
                ]}
                onPress={() => setConnectionType('usb')}
              >
                <Icon
                  name="usb-port"
                  size={20}
                  color={connectionType === 'usb' ? THEME.primary : THEME.textSecondary}
                />
                <Text style={[
                  styles.typeText,
                  connectionType === 'usb' && styles.typeTextActive,
                ]}>
                  USB
                </Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[
                  styles.typeButton,
                  connectionType === 'tcp' && styles.typeButtonActive,
                ]}
                onPress={() => setConnectionType('tcp')}
              >
                <Icon
                  name="lan-connect"
                  size={20}
                  color={connectionType === 'tcp' ? THEME.primary : THEME.textSecondary}
                />
                <Text style={[
                  styles.typeText,
                  connectionType === 'tcp' && styles.typeTextActive,
                ]}>
                  TCP/IP
                </Text>
              </TouchableOpacity>
            </View>

            {/* TCP Configuration */}
            {connectionType === 'tcp' && (
              <View style={styles.tcpConfig}>
                <View style={styles.tcpRow}>
                  <Text style={styles.modalLabel}>Host</Text>
                  <TextInput
                    style={styles.tcpInput}
                    value={tcpHost}
                    onChangeText={setTcpHost}
                    placeholder="192.168.1.100"
                    autoCapitalize="none"
                    keyboardType="url"
                  />
                </View>
                <View style={styles.tcpRow}>
                  <Text style={styles.modalLabel}>Port</Text>
                  <TextInput
                    style={styles.tcpInput}
                    value={tcpPort}
                    onChangeText={setTcpPort}
                    placeholder="9999"
                    keyboardType="numeric"
                  />
                </View>
              </View>
            )}

            {/* USB Info */}
            {connectionType === 'usb' && (
              <View style={styles.usbInfo}>
                <Icon name="information" size={16} color={THEME.textSecondary} />
                <Text style={styles.usbInfoText}>
                  Connect via USB 3.0 cable. The PhotonWeave device will appear
                  as a vendor-specific SuperSpeed device (VID: 0x4E4F, PID: 0x5057).
                </Text>
              </View>
            )}

            {/* Action Buttons */}
            <View style={styles.modalActions}>
              <TouchableOpacity
                style={styles.cancelButton}
                onPress={() => setModalVisible(false)}
              >
                <Text style={styles.cancelText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={styles.connectButton}
                onPress={handleConnectPress}
                disabled={connecting}
              >
                {connecting ? (
                  <ActivityIndicator color="#FFF" size="small" />
                ) : (
                  <>
                    <Icon name="connection" size={18} color="#FFF" />
                    <Text style={styles.connectText}>Connect</Text>
                  </>
                )}
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </View>
  );
}

//=============================================================================
// Styles
//=============================================================================
const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    alignItems: 'center',
    marginRight: 12,
  },
  statusButton: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 16,
    borderWidth: 1,
  },
  statusConnected: {
    backgroundColor: THEME.success + '15',
    borderColor: THEME.success + '40',
  },
  statusDisconnected: {
    backgroundColor: THEME.error + '15',
    borderColor: THEME.error + '40',
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 6,
  },
  statusText: {
    color: THEME.text,
    fontSize: 12,
    fontWeight: '600',
    marginRight: 6,
  },
  infoTooltip: {
    position: 'absolute',
    top: 36,
    right: 0,
    backgroundColor: THEME.surface,
    borderRadius: 8,
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderWidth: 1,
    borderColor: THEME.border,
    maxWidth: 250,
    zIndex: 100,
  },
  infoText: {
    color: THEME.textSecondary,
    fontSize: 11,
    fontWeight: '500',
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContent: {
    backgroundColor: THEME.surface,
    borderRadius: 16,
    padding: 24,
    width: '88%',
    borderWidth: 1,
    borderColor: THEME.border,
  },
  modalTitle: {
    color: THEME.text,
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 20,
    textAlign: 'center',
  },
  modalLabel: {
    color: THEME.textSecondary,
    fontSize: 13,
    fontWeight: '600',
    marginBottom: 6,
    textTransform: 'uppercase',
    letterSpacing: 0.5,
  },
  typeRow: {
    flexDirection: 'row',
    marginBottom: 16,
  },
  typeButton: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 12,
    borderRadius: 10,
    borderWidth: 1,
    borderColor: THEME.border,
    marginHorizontal: 4,
    backgroundColor: THEME.background,
  },
  typeButtonActive: {
    borderColor: THEME.primary,
    backgroundColor: THEME.primary + '15',
  },
  typeText: {
    color: THEME.textSecondary,
    fontSize: 14,
    fontWeight: '600',
    marginLeft: 8,
  },
  typeTextActive: {
    color: THEME.primary,
  },
  tcpConfig: {
    marginBottom: 12,
  },
  tcpRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  tcpInput: {
    flex: 1,
    backgroundColor: THEME.background,
    color: THEME.text,
    fontSize: 14,
    fontWeight: '600',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    marginLeft: 8,
    fontFamily: 'monospace',
  },
  usbInfo: {
    flexDirection: 'row',
    backgroundColor: THEME.background,
    borderRadius: 8,
    padding: 12,
    marginBottom: 16,
  },
  usbInfoText: {
    color: THEME.textSecondary,
    fontSize: 12,
    lineHeight: 16,
    marginLeft: 8,
    flex: 1,
  },
  modalActions: {
    flexDirection: 'row',
    justifyContent: 'flex-end',
    marginTop: 8,
  },
  cancelButton: {
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 8,
    marginRight: 10,
  },
  cancelText: {
    color: THEME.textSecondary,
    fontSize: 14,
    fontWeight: '600',
  },
  connectButton: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: THEME.primary,
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 8,
  },
  connectText: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: '700',
    marginLeft: 6,
  },
});
