// SettingsScreen.js — Aether-Link Device Settings and Configuration
// Provides device configuration, firmware update, fan control,
// network settings, and error log viewing.

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Switch,
  Modal,
  Alert,
  ActivityIndicator,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { useDevice } from '../components/DeviceContext';
import { CMD, STATUS } from '../utils/protocol';

// ============================================================================
// Settings Screen Component
// ============================================================================

export default function SettingsScreen() {
  const {
    connected,
    deviceHost,
    devicePort,
    setDeviceHost,
    setDevicePort,
    telemetry,
    firmwareVersion,
    deviceStatus,
    deviceFeatures,
    errorLog,
    sendCommand,
    setFanSpeed,
    getErrorLog,
    disconnect,
    connect,
  } = useDevice();

  const [hostInput, setHostInput] = useState(deviceHost);
  const [portInput, setPortInput] = useState(String(devicePort));
  const [fanPWM, setFanPWM] = useState(80);
  const [fwUpdateModal, setFwUpdateModal] = useState(false);
  const [errorLogModal, setErrorLogModal] = useState(false);
  const [loading, setLoading] = useState(false);

  // ==========================================================================
  // Connection Settings
  // ==========================================================================

  const handleSaveConnection = useCallback(() => {
    const port = parseInt(portInput, 10);
    if (isNaN(port) || port < 1 || port > 65535) {
      Alert.alert('Invalid Port', 'Port must be between 1 and 65535.');
      return;
    }
    setDeviceHost(hostInput);
    setDevicePort(port);
    Alert.alert('Saved', 'Connection settings updated. Reconnect to apply.');
  }, [hostInput, portInput, setDeviceHost, setDevicePort]);

  const handleReconnect = useCallback(async () => {
    disconnect();
    await new Promise((r) => setTimeout(r, 500));
    try {
      await connect();
    } catch (err) {
      Alert.alert('Connection Failed', err.message);
    }
  }, [disconnect, connect]);

  // ==========================================================================
  // Fan Control
  // ==========================================================================

  const handleFanSpeedChange = useCallback(async (value) => {
    const pwm = parseInt(value, 10);
    if (isNaN(pwm) || pwm < 0 || pwm > 255) return;
    setFanPWM(pwm);
    try {
      await setFanSpeed(pwm);
    } catch (err) {
      Alert.alert('Error', 'Failed to set fan speed.');
    }
  }, [setFanSpeed]);

  const fanPresets = [
    { label: 'Quiet', value: 30, icon: 'volume-low' },
    { label: 'Normal', value: 80, icon: 'fan' },
    { label: 'Performance', value: 150, icon: 'fan-speed-1' },
    { label: 'Maximum', value: 255, icon: 'fan-speed-3' },
  ];

  // ==========================================================================
  // Firmware Update
  // ==========================================================================

  const handleFWUpdateCheck = useCallback(async () => {
    setLoading(true);
    try {
      const frame = await sendCommand(CMD.GET_VERSION);
      if (frame.payload) {
        const version = frame.payload.readUInt32BE(4);
        const major = (version >> 24) & 0xFF;
        const minor = (version >> 16) & 0xFF;
        const patch = (version >> 8) & 0xFF;
        const build = version & 0xFF;
        Alert.alert(
          'Firmware Version',
          `Current: v${major}.${minor}.${patch} (build ${build})\n\n` +
          'Firmware updates are performed via the host management driver.\n' +
          'Use aether_mgmt.ko to flash new bitstreams.'
        );
      }
    } catch (err) {
      Alert.alert('Error', 'Failed to get firmware version.');
    } finally {
      setLoading(false);
    }
  }, [sendCommand]);

  // ==========================================================================
  // Error Log
  // ==========================================================================

  const handleViewErrorLog = useCallback(async () => {
    setLoading(true);
    try {
      const entries = await getErrorLog(50);
      setErrorLogModal(true);
    } catch (err) {
      Alert.alert('Error', 'Failed to retrieve error log.');
    } finally {
      setLoading(false);
    }
  }, [getErrorLog]);

  // ==========================================================================
  // Device Reset
  // ==========================================================================

  const handleResetDevice = useCallback(() => {
    Alert.alert(
      'Reset Device',
      'This will soft-reset the Aether-Link device. All connections will be dropped. Continue?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Reset',
          style: 'destructive',
          onPress: async () => {
            try {
              await sendCommand(CMD.RESET_DEVICE);
              Alert.alert('Reset', 'Device reset command sent. The device will restart.');
            } catch (err) {
              Alert.alert('Error', 'Failed to send reset command.');
            }
          },
        },
      ]
    );
  }, [sendCommand]);

  // ==========================================================================
  // Feature Flags Display
  // ==========================================================================

  const featureList = [
    { bit: 0, name: 'NVMe-oF/TCP Offload', desc: 'Hardware NVMe-oF/TCP PDU processing' },
    { bit: 1, name: 'Dual Port', desc: 'Two 100GbE QSFP28 ports' },
    { bit: 2, name: 'RDMA Zero-Copy', desc: 'Direct DMA to host memory' },
    { bit: 3, name: 'TSO', desc: 'TCP Segmentation Offload' },
    { bit: 4, name: 'LRO', desc: 'Large Receive Offload' },
    { bit: 5, name: 'DCTCP', desc: 'Data Center TCP congestion control' },
    { bit: 6, name: 'ECN Marking', desc: 'Explicit Congestion Notification' },
    { bit: 7, name: 'DH-HMAC-CHAP', desc: 'NVMe-oF authentication' },
    { bit: 8, name: 'CRC32C Offload', desc: 'Hardware CRC32C computation' },
    { bit: 9, name: 'FW Update', desc: 'In-field firmware updates' },
    { bit: 10, name: 'Secure Boot', desc: 'AES-GCM encrypted bitstream' },
    { bit: 11, name: 'DDR4 ECC', desc: 'Single-error correction, double-error detection' },
    { bit: 12, name: 'Thermal Throttling', desc: 'Temperature-based performance management' },
    { bit: 13, name: 'Telemetry', desc: 'Temperature, power, and statistics monitoring' },
    { bit: 14, name: 'Debug UART', desc: 'Serial console for diagnostics' },
  ];

  const isFeatureEnabled = (bit) => {
    return deviceFeatures ? (deviceFeatures & (1 << bit)) !== 0 : false;
  };

  // ==========================================================================
  // Not Connected State
  // ==========================================================================

  if (!connected) {
    return (
      <View style={styles.centeredContainer}>
        <Icon name="cog-off" size={64} color="#6B7280" />
        <Text style={styles.disconnectedTitle}>Not Connected</Text>
        <Text style={styles.disconnectedSubtitle}>
          Connect to configure device settings
        </Text>

        {/* Connection Settings (available offline) */}
        <View style={styles.offlineSettings}>
          <Text style={styles.sectionTitle}>Connection Settings</Text>
          <Text style={styles.inputLabel}>Device Host</Text>
          <TextInput
            style={styles.input}
            value={hostInput}
            onChangeText={setHostInput}
            placeholder="192.168.1.10"
            placeholderTextColor="#4B5563"
            autoCapitalize="none"
          />
          <Text style={styles.inputLabel}>Port</Text>
          <TextInput
            style={styles.input}
            value={portInput}
            onChangeText={setPortInput}
            placeholder="4420"
            placeholderTextColor="#4B5563"
            keyboardType="numeric"
          />
          <TouchableOpacity style={styles.saveButton} onPress={handleSaveConnection}>
            <Text style={styles.saveButtonText}>Save & Connect</Text>
          </TouchableOpacity>
        </View>
      </View>
    );
  }

  // ==========================================================================
  // Render
  // ==========================================================================

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.contentContainer}>
      {/* Connection Settings */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Connection</Text>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Host</Text>
          <TextInput
            style={styles.settingInput}
            value={hostInput}
            onChangeText={setHostInput}
            autoCapitalize="none"
          />
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Port</Text>
          <TextInput
            style={styles.settingInput}
            value={portInput}
            onChangeText={setPortInput}
            keyboardType="numeric"
          />
        </View>
        <View style={styles.settingActions}>
          <TouchableOpacity style={styles.actionButton} onPress={handleSaveConnection}>
            <Text style={styles.actionButtonText}>Save</Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.reconnectButton} onPress={handleReconnect}>
            <Text style={styles.reconnectButtonText}>Reconnect</Text>
          </TouchableOpacity>
        </View>
      </View>

      {/* Fan Control */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Fan Control</Text>
        <View style={styles.fanStatus}>
          <Icon name="fan" size={20} color="#3B82F6" />
          <Text style={styles.fanStatusText}>
            Current: {telemetry?.fan0RPM ?? '--'} RPM (PWM: {telemetry?.fanPWM ?? '--'}/255)
          </Text>
        </View>
        <View style={styles.fanPresets}>
          {fanPresets.map((preset) => (
            <TouchableOpacity
              key={preset.value}
              style={[styles.fanPreset, fanPWM === preset.value && styles.fanPresetActive]}
              onPress={() => handleFanSpeedChange(String(preset.value))}
            >
              <Icon name={preset.icon} size={18} color={fanPWM === preset.value ? '#3B82F6' : '#9CA3AF'} />
              <Text style={[styles.fanPresetLabel, fanPWM === preset.value && styles.fanPresetLabelActive]}>
                {preset.label}
              </Text>
              <Text style={[styles.fanPresetValue, fanPWM === preset.value && styles.fanPresetValueActive]}>
                {preset.value}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
        <View style={styles.settingRow}>
          <Text style={styles.settingLabel}>Custom PWM</Text>
          <TextInput
            style={styles.settingInput}
            value={String(fanPWM)}
            onChangeText={handleFanSpeedChange}
            keyboardType="numeric"
          />
        </View>
      </View>

      {/* Firmware */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Firmware</Text>
        <View style={styles.fwInfo}>
          <Icon name="chip" size={20} color="#8B5CF6" />
          <Text style={styles.fwVersionText}>
            {firmwareVersion
              ? `v${(firmwareVersion >> 24) & 0xFF}.${(firmwareVersion >> 16) & 0xFF}.${(firmwareVersion >> 8) & 0xFF}.${firmwareVersion & 0xFF}`
              : 'Unknown'}
          </Text>
        </View>
        <TouchableOpacity style={styles.actionButton} onPress={handleFWUpdateCheck}>
          {loading ? (
            <ActivityIndicator size="small" color="#FFFFFF" />
          ) : (
            <Text style={styles.actionButtonText}>Check Version</Text>
          )}
        </TouchableOpacity>
      </View>

      {/* Features */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Hardware Features</Text>
        {featureList.map((feature) => (
          <View key={feature.bit} style={styles.featureRow}>
            <Icon
              name={isFeatureEnabled(feature.bit) ? 'check-circle' : 'close-circle'}
              size={18}
              color={isFeatureEnabled(feature.bit) ? '#10B981' : '#374151'}
            />
            <View style={styles.featureText}>
              <Text style={[
                styles.featureName,
                { color: isFeatureEnabled(feature.bit) ? '#E5E7EB' : '#6B7280' }
              ]}>
                {feature.name}
              </Text>
              <Text style={styles.featureDesc}>{feature.desc}</Text>
            </View>
          </View>
        ))}
      </View>

      {/* Diagnostics */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Diagnostics</Text>
        <TouchableOpacity style={styles.diagButton} onPress={handleViewErrorLog}>
          <Icon name="alert-circle-outline" size={20} color="#F59E0B" />
          <Text style={styles.diagButtonText}>View Error Log</Text>
          <Icon name="chevron-right" size={20} color="#6B7280" />
        </TouchableOpacity>
        <TouchableOpacity style={styles.diagButton} onPress={handleResetDevice}>
          <Icon name="restart" size={20} color="#EF4444" />
          <Text style={[styles.diagButtonText, { color: '#EF4444' }]}>Reset Device</Text>
          <Icon name="chevron-right" size={20} color="#6B7280" />
        </TouchableOpacity>
      </View>

      {/* Error Log Modal */}
      <Modal
        visible={errorLogModal}
        animationType="slide"
        transparent={true}
        onRequestClose={() => setErrorLogModal(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Error Log</Text>
            <ScrollView style={styles.errorLogScroll}>
              {errorLog.length === 0 ? (
                <Text style={styles.noErrors}>No errors recorded</Text>
              ) : (
                errorLog.map((entry, idx) => (
                  <View key={idx} style={styles.errorEntry}>
                    <Text style={styles.errorTimestamp}>
                      T+{entry.timestamp}ms
                    </Text>
                    <Text style={[
                      styles.errorCode,
                      { color: entry.severity >= 2 ? '#EF4444' : '#F59E0B' }
                    ]}>
                      Code: 0x{entry.errorCode?.toString(16).toUpperCase().padStart(2, '0')}
                    </Text>
                    <Text style={styles.errorDetail}>
                      Sev: {entry.severity} | Src: {entry.source}
                    </Text>
                  </View>
                ))
              )}
            </ScrollView>
            <TouchableOpacity
              style={styles.closeButton}
              onPress={() => setErrorLogModal(false)}
            >
              <Text style={styles.closeButtonText}>Close</Text>
            </TouchableOpacity>
          </View>
        </View>
      </Modal>

      <View style={{ height: 40 }} />
    </ScrollView>
  );
}

// ============================================================================
// Styles
// ============================================================================

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0E1A',
  },
  contentContainer: {
    padding: 16,
  },
  centeredContainer: {
    flex: 1,
    backgroundColor: '#0A0E1A',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 32,
  },
  disconnectedTitle: {
    fontSize: 22,
    fontWeight: '700',
    color: '#E5E7EB',
    marginTop: 16,
  },
  disconnectedSubtitle: {
    fontSize: 14,
    color: '#9CA3AF',
    textAlign: 'center',
    marginTop: 8,
  },
  offlineSettings: {
    width: '100%',
    marginTop: 24,
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 16,
  },
  section: {
    marginBottom: 24,
    backgroundColor: '#111827',
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#1F2937',
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: '#E5E7EB',
    marginBottom: 12,
  },
  settingRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  settingLabel: {
    width: 60,
    fontSize: 14,
    color: '#9CA3AF',
  },
  settingInput: {
    flex: 1,
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 10,
    fontSize: 14,
    color: '#E5E7EB',
    borderWidth: 1,
    borderColor: '#374151',
  },
  settingActions: {
    flexDirection: 'row',
    gap: 10,
    marginTop: 8,
  },
  actionButton: {
    backgroundColor: '#3B82F6',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  actionButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
  reconnectButton: {
    backgroundColor: '#1F2937',
    paddingHorizontal: 20,
    paddingVertical: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#374151',
  },
  reconnectButtonText: {
    color: '#E5E7EB',
    fontSize: 14,
    fontWeight: '600',
  },
  fanStatus: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  fanStatusText: {
    fontSize: 14,
    color: '#E5E7EB',
    marginLeft: 8,
  },
  fanPresets: {
    flexDirection: 'row',
    gap: 8,
    marginBottom: 12,
  },
  fanPreset: {
    flex: 1,
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 10,
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#374151',
  },
  fanPresetActive: {
    borderColor: '#3B82F6',
    backgroundColor: '#1E3A5F',
  },
  fanPresetLabel: {
    fontSize: 11,
    color: '#9CA3AF',
    marginTop: 4,
  },
  fanPresetLabelActive: {
    color: '#3B82F6',
  },
  fanPresetValue: {
    fontSize: 12,
    color: '#6B7280',
    marginTop: 2,
  },
  fanPresetValueActive: {
    color: '#3B82F6',
  },
  fwInfo: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  fwVersionText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#E5E7EB',
    marginLeft: 8,
  },
  featureRow: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#1F2937',
  },
  featureText: {
    flex: 1,
    marginLeft: 10,
  },
  featureName: {
    fontSize: 14,
    fontWeight: '600',
  },
  featureDesc: {
    fontSize: 12,
    color: '#6B7280',
    marginTop: 2,
  },
  diagButton: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#1F2937',
  },
  diagButtonText: {
    flex: 1,
    fontSize: 14,
    color: '#E5E7EB',
    marginLeft: 10,
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  modalContent: {
    backgroundColor: '#111827',
    borderRadius: 16,
    padding: 24,
    width: '100%',
    maxWidth: 400,
    maxHeight: '80%',
    borderWidth: 1,
    borderColor: '#1F2937',
  },
  modalTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: '#E5E7EB',
    marginBottom: 16,
  },
  errorLogScroll: {
    maxHeight: 400,
  },
  noErrors: {
    fontSize: 14,
    color: '#6B7280',
    textAlign: 'center',
    padding: 20,
  },
  errorEntry: {
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 10,
    marginBottom: 8,
  },
  errorTimestamp: {
    fontSize: 12,
    color: '#9CA3AF',
  },
  errorCode: {
    fontSize: 14,
    fontWeight: '600',
    marginTop: 2,
  },
  errorDetail: {
    fontSize: 12,
    color: '#6B7280',
    marginTop: 2,
  },
  closeButton: {
    backgroundColor: '#374151',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 16,
  },
  closeButtonText: {
    color: '#E5E7EB',
    fontSize: 14,
    fontWeight: '600',
  },
  inputLabel: {
    fontSize: 13,
    color: '#9CA3AF',
    marginBottom: 6,
    marginTop: 12,
  },
  input: {
    backgroundColor: '#1F2937',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    color: '#E5E7EB',
    borderWidth: 1,
    borderColor: '#374151',
  },
  saveButton: {
    backgroundColor: '#3B82F6',
    paddingVertical: 12,
    borderRadius: 8,
    alignItems: 'center',
    marginTop: 16,
  },
  saveButtonText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '600',
  },
});
