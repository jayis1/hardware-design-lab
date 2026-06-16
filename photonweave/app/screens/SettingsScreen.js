/**
 * SettingsScreen.js — Device Configuration & Calibration
 *
 * Settings and configuration interface for the PhotonWeave CGH Engine.
 * Provides device information display, firmware update, QSFP+ module
 * management, clock configuration, and advanced debug controls.
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View, Text, StyleSheet, ScrollView, TouchableOpacity,
  Switch, TextInput, Alert, ActivityIndicator, Modal, FlatList,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { THEME } from '../App';

//=============================================================================
// SettingsScreen Component
//=============================================================================
export default function SettingsScreen({ protocol, isConnected, deviceInfo }) {
  // ── Device Info ──
  const [localDeviceInfo, setLocalDeviceInfo] = useState(deviceInfo);

  // ── QSFP+ State ──
  const [qsfpEnabled, setQsfpEnabled] = useState(false);
  const [qsfpStatus, setQsfpStatus] = useState(null);
  const [qsfpLoading, setQsfpLoading] = useState(false);

  // ── Firmware Update ──
  const [fwUpdateModal, setFwUpdateModal] = useState(false);
  const [fwUpdateProgress, setFwUpdateProgress] = useState(0);
  const [fwUpdating, setFwUpdating] = useState(false);

  // ── Clock Configuration ──
  const [clockThrottle, setClockThrottle] = useState(false);
  const [fftEngineMask, setFftEngineMask] = useState(0xFF);

  // ── Register Debug ──
  const [regOffset, setRegOffset] = useState('0x0000');
  const [regValue, setRegValue] = useState('');
  const [regReadResult, setRegReadResult] = useState(null);
  const [regWriteStatus, setRegWriteStatus] = useState(null);

  // ── System Control ──
  const [watchdogEnabled, setWatchdogEnabled] = useState(true);
  const [eccEnabled, setEccEnabled] = useState(true);
  const [thermalThrottle, setThermalThrottle] = useState(true);

  // Refresh device info on mount and when deviceInfo prop changes
  useEffect(() => {
    if (deviceInfo) {
      setLocalDeviceInfo(deviceInfo);
    }
  }, [deviceInfo]);

  // Refresh QSFP+ status periodically
  useEffect(() => {
    if (isConnected && qsfpEnabled) {
      const interval = setInterval(refreshQsfpStatus, 2000);
      return () => clearInterval(interval);
    }
  }, [isConnected, qsfpEnabled]);

  //===========================================================================
  // QSFP+ Management
  //===========================================================================
  const refreshQsfpStatus = useCallback(async () => {
    try {
      const status = await protocol.getQsfpStatus();
      setQsfpStatus(status);
    } catch (error) {
      console.warn('QSFP status error:', error.message);
    }
  }, [isConnected]);

  const handleQsfpToggle = useCallback(async (value) => {
    if (!isConnected) return;
    setQsfpLoading(true);
    try {
      if (value) {
        // Enable QSFP+ — this would use a specific command
        await protocol.writeRegister(0x00B0, 0x01); // QSFP_CTRL enable
        setQsfpEnabled(true);
        await refreshQsfpStatus();
      } else {
        await protocol.writeRegister(0x00B0, 0x00); // QSFP_CTRL disable
        setQsfpEnabled(false);
        setQsfpStatus(null);
      }
    } catch (error) {
      Alert.alert('Error', `QSFP+ control failed: ${error.message}`);
    } finally {
      setQsfpLoading(false);
    }
  }, [isConnected]);

  //===========================================================================
  // Firmware Update
  //===========================================================================
  const handleFwUpdate = useCallback(async () => {
    if (!isConnected) return;
    setFwUpdateModal(true);
    setFwUpdateProgress(0);
    setFwUpdating(true);

    try {
      // Simulated firmware update progress
      for (let i = 0; i <= 100; i += 5) {
        await new Promise(resolve => setTimeout(resolve, 200));
        setFwUpdateProgress(i);
      }
      Alert.alert('Success', 'Firmware update completed. Device will reboot.');
    } catch (error) {
      Alert.alert('Error', `Firmware update failed: ${error.message}`);
    } finally {
      setFwUpdating(false);
      setFwUpdateModal(false);
    }
  }, [isConnected]);

  //===========================================================================
  // Register Debug
  //===========================================================================
  const handleRegRead = useCallback(async () => {
    if (!isConnected) return;
    try {
      const offset = parseInt(regOffset, 16);
      const value = await protocol.readRegister(offset);
      setRegReadResult({ offset: regOffset, value });
    } catch (error) {
      Alert.alert('Error', `Register read failed: ${error.message}`);
    }
  }, [isConnected, regOffset]);

  const handleRegWrite = useCallback(async () => {
    if (!isConnected) return;
    try {
      const offset = parseInt(regOffset, 16);
      const value = parseInt(regValue, 16);
      await protocol.writeRegister(offset, value);
      setRegWriteStatus('OK');
      setTimeout(() => setRegWriteStatus(null), 2000);
    } catch (error) {
      setRegWriteStatus('FAIL');
      setTimeout(() => setRegWriteStatus(null), 2000);
    }
  }, [isConnected, regOffset, regValue]);

  //===========================================================================
  // System Control
  //===========================================================================
  const handleSystemControl = useCallback(async (register, bit, value) => {
    if (!isConnected) return;
    try {
      const current = await protocol.readRegister(register);
      const updated = value ? (current | bit) : (current & ~bit);
      await protocol.writeRegister(register, updated);
    } catch (error) {
      Alert.alert('Error', `System control failed: ${error.message}`);
    }
  }, [isConnected]);

  //===========================================================================
  // Render
  //===========================================================================
  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* ── Device Information ── */}
      <Text style={styles.sectionTitle}>Device Information</Text>
      <View style={styles.infoCard}>
        {localDeviceInfo ? (
          <>
            <InfoRow label="Product" value={localDeviceInfo.productName || 'PhotonWeave CGH Engine'} />
            <InfoRow label="Manufacturer" value={localDeviceInfo.manufacturer || 'Nous Research'} />
            <InfoRow label="Hardware Rev" value={`${localDeviceInfo.hardwareVersion?.major || 1}.${localDeviceInfo.hardwareVersion?.minor || 0}`} />
            <InfoRow label="Board Rev" value={localDeviceInfo.boardRevision || '1.0'} />
            <InfoRow label="Firmware" value={localDeviceInfo.firmwareVersion || '1.0.0'} />
            <InfoRow label="Serial" value={localDeviceInfo.serialNumber || 'Unknown'} />
            <InfoRow label="MAC Address" value={localDeviceInfo.macAddress || 'Unknown'} />
            <InfoRow label="FPGA" value="XC7K325T-2FFG900C" />
            <InfoRow label="DDR4" value="4× 8Gb MT40A512M16JY-075E" />
            <InfoRow label="Clock Gen" value="Si5345A-B-GM" />
            <InfoRow label="PMIC" value="TPS65218D0RSLR" />
          </>
        ) : (
          <Text style={styles.noInfoText}>
            Connect to device to view information
          </Text>
        )}
      </View>

      {/* ── QSFP+ Module ── */}
      <Text style={styles.sectionTitle}>QSFP+ Camera Ingest</Text>
      <View style={styles.qsfpCard}>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Enable QSFP+ Receiver</Text>
          <Switch
            value={qsfpEnabled}
            onValueChange={handleQsfpToggle}
            trackColor={{ false: THEME.border, true: THEME.primary + '60' }}
            thumbColor={qsfpEnabled ? THEME.primary : THEME.textSecondary}
            disabled={!isConnected || qsfpLoading}
          />
        </View>
        {qsfpLoading && <ActivityIndicator color={THEME.primary} style={{ marginVertical: 8 }} />}
        {qsfpStatus && (
          <View style={styles.qsfpStatus}>
            <StatusDot label="Module Present" ok={qsfpStatus.modulePresent} />
            <StatusDot label="Link Up" ok={qsfpStatus.linkUp} />
            <StatusDot label="RX Active" ok={qsfpStatus.rxActive} />
            <InfoRow label="Link Speed" value={`${qsfpStatus.linkSpeedMbps || 0} Mbps`} />
            <InfoRow label="RX Frames" value={qsfpStatus.rxFrameCount?.toLocaleString() || '0'} />
            <InfoRow label="RX Errors" value={qsfpStatus.rxErrorCount?.toLocaleString() || '0'} />
          </View>
        )}
      </View>

      {/* ── Clock Configuration ── */}
      <Text style={styles.sectionTitle}>Clock Configuration</Text>
      <View style={styles.settingsCard}>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Thermal Clock Throttle</Text>
          <Switch
            value={clockThrottle}
            onValueChange={(v) => {
              setClockThrottle(v);
              handleSystemControl(0x0128, 0x01, v);
            }}
            trackColor={{ false: THEME.border, true: THEME.accent + '60' }}
            thumbColor={clockThrottle ? THEME.accent : THEME.textSecondary}
            disabled={!isConnected}
          />
        </View>
        <Text style={styles.settingDesc}>
          Automatically reduce clock frequencies when temperature exceeds 80°C
        </Text>

        <Text style={styles.settingLabel}>Active FFT Engines</Text>
        <View style={styles.engineGrid}>
          {[0, 1, 2, 3, 4, 5, 6, 7].map((engine) => {
            const active = (fftEngineMask & (1 << engine)) !== 0;
            return (
              <TouchableOpacity
                key={engine}
                style={[styles.engineChip, active && styles.engineChipActive]}
                onPress={() => {
                  const newMask = fftEngineMask ^ (1 << engine);
                  setFftEngineMask(newMask);
                  handleSystemControl(0x004C, newMask << 20, true);
                }}
                disabled={!isConnected}
              >
                <Text style={[styles.engineText, active && styles.engineTextActive]}>
                  E{engine}
                </Text>
              </TouchableOpacity>
            );
          })}
        </View>
        <Text style={styles.settingDesc}>
          Disable individual FFT engines for power savings or fault isolation
        </Text>
      </View>

      {/* ── System Control ── */}
      <Text style={styles.sectionTitle}>System Control</Text>
      <View style={styles.settingsCard}>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Watchdog Timer</Text>
          <Switch
            value={watchdogEnabled}
            onValueChange={(v) => {
              setWatchdogEnabled(v);
              handleSystemControl(0x000C, 0x0100, v);
            }}
            trackColor={{ false: THEME.border, true: THEME.success + '60' }}
            thumbColor={watchdogEnabled ? THEME.success : THEME.textSecondary}
            disabled={!isConnected}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>DDR4 ECC</Text>
          <Switch
            value={eccEnabled}
            onValueChange={(v) => {
              setEccEnabled(v);
              handleSystemControl(0x000C, 0x0080, v);
            }}
            trackColor={{ false: THEME.border, true: THEME.success + '60' }}
            thumbColor={eccEnabled ? THEME.success : THEME.textSecondary}
            disabled={!isConnected}
          />
        </View>
        <View style={styles.switchRow}>
          <Text style={styles.switchLabel}>Auto Thermal Throttle</Text>
          <Switch
            value={thermalThrottle}
            onValueChange={(v) => {
              setThermalThrottle(v);
              handleSystemControl(0x000C, 0x0040, v);
            }}
            trackColor={{ false: THEME.border, true: THEME.success + '60' }}
            thumbColor={thermalThrottle ? THEME.success : THEME.textSecondary}
            disabled={!isConnected}
          />
        </View>
      </View>

      {/* ── Register Debug ── */}
      <Text style={styles.sectionTitle}>Register Debug</Text>
      <View style={styles.settingsCard}>
        <View style={styles.regRow}>
          <Text style={styles.settingLabel}>Offset (hex)</Text>
          <TextInput
            style={styles.regInput}
            value={regOffset}
            onChangeText={setRegOffset}
            autoCapitalize="characters"
            placeholder="0x0000"
          />
        </View>
        <View style={styles.regRow}>
          <Text style={styles.settingLabel}>Value (hex, write)</Text>
          <TextInput
            style={styles.regInput}
            value={regValue}
            onChangeText={setRegValue}
            autoCapitalize="characters"
            placeholder="0x00000000"
          />
        </View>
        <View style={styles.regActions}>
          <TouchableOpacity
            style={styles.regButton}
            onPress={handleRegRead}
            disabled={!isConnected}
          >
            <Icon name="arrow-down-bold" size={16} color="#FFF" />
            <Text style={styles.regButtonText}>Read</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={[styles.regButton, styles.regWriteButton]}
            onPress={handleRegWrite}
            disabled={!isConnected}
          >
            <Icon name="arrow-up-bold" size={16} color="#FFF" />
            <Text style={styles.regButtonText}>Write</Text>
          </TouchableOpacity>
        </View>
        {regReadResult && (
          <View style={styles.regResult}>
            <Text style={styles.regResultText}>
              [{regReadResult.offset}] = 0x{regReadResult.value.toString(16).toUpperCase().padStart(8, '0')}
            </Text>
          </View>
        )}
        {regWriteStatus && (
          <Text style={[
            styles.regWriteStatus,
            { color: regWriteStatus === 'OK' ? THEME.success : THEME.error },
          ]}>
            Write: {regWriteStatus}
          </Text>
        )}
      </View>

      {/* ── Firmware Update ── */}
      <Text style={styles.sectionTitle}>Firmware Update</Text>
      <TouchableOpacity
        style={styles.fwUpdateButton}
        onPress={handleFwUpdate}
        disabled={!isConnected}
      >
        <Icon name="cellphone-arrow-down" size={20} color="#FFF" />
        <Text style={styles.fwUpdateText}>Update FPGA Firmware</Text>
      </TouchableOpacity>
      <Text style={styles.fwWarning}>
        Warning: Firmware update will interrupt hologram processing.
        Ensure no critical operations are in progress.
      </Text>

      {/* ── Firmware Update Modal ── */}
      <Modal visible={fwUpdateModal} transparent animationType="fade">
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Firmware Update</Text>
            <Text style={styles.modalSubtitle}>
              Updating FPGA bitstream. Do not power off the device.
            </Text>
            <View style={styles.progressBar}>
              <View style={[styles.progressFill, { width: `${fwUpdateProgress}%` }]} />
            </View>
            <Text style={styles.progressText}>{fwUpdateProgress}%</Text>
            {fwUpdateProgress >= 100 && (
              <Text style={styles.modalComplete}>Update complete! Rebooting...</Text>
            )}
          </View>
        </View>
      </Modal>

      {/* ── Connection Warning ── */}
      {!isConnected && (
        <View style={styles.warningBanner}>
          <Icon name="alert-circle" size={20} color={THEME.warning} />
          <Text style={styles.warningText}>
            Not connected. Settings require an active device connection.
          </Text>
        </View>
      )}
    </ScrollView>
  );
}

//=============================================================================
// Sub-Components
//=============================================================================

function InfoRow({ label, value }) {
  return (
    <View style={infoStyles.row}>
      <Text style={infoStyles.label}>{label}</Text>
      <Text style={infoStyles.value} selectable>{value}</Text>
    </View>
  );
}

const infoStyles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: THEME.border,
  },
  label: {
    color: THEME.textSecondary,
    fontSize: 13,
    fontWeight: '500',
  },
  value: {
    color: THEME.text,
    fontSize: 13,
    fontWeight: '600',
    fontVariant: ['tabular-nums'],
    maxWidth: '55%',
    textAlign: 'right',
  },
});

function StatusDot({ label, ok }) {
  return (
    <View style={statusStyles.row}>
      <View style={[statusStyles.dot, { backgroundColor: ok ? THEME.success : THEME.error }]} />
      <Text style={statusStyles.label}>{label}</Text>
      <Text style={[statusStyles.value, { color: ok ? THEME.success : THEME.error }]}>
        {ok ? 'YES' : 'NO'}
      </Text>
    </View>
  );
}

const statusStyles = StyleSheet.create({
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  label: {
    color: THEME.text,
    fontSize: 13,
    fontWeight: '500',
    flex: 1,
  },
  value: {
    fontSize: 13,
    fontWeight: '700',
  },
});

//=============================================================================
// Styles
//=============================================================================
const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: THEME.background,
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  sectionTitle: {
    color: THEME.primary,
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 12,
  },
  infoCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  noInfoText: {
    color: THEME.textSecondary,
    fontSize: 14,
    fontStyle: 'italic',
    textAlign: 'center',
    paddingVertical: 12,
  },
  qsfpCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 8,
  },
  switchLabel: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '500',
    flex: 1,
  },
  qsfpStatus: {
    marginTop: 12,
    borderTopWidth: 1,
    borderTopColor: THEME.border,
    paddingTop: 12,
  },
  settingsCard: {
    backgroundColor: THEME.surface,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  settingLabel: {
    color: THEME.text,
    fontSize: 14,
    fontWeight: '500',
    marginTop: 12,
    marginBottom: 8,
  },
  settingDesc: {
    color: THEME.textSecondary,
    fontSize: 12,
    marginTop: 4,
    marginBottom: 8,
    lineHeight: 16,
  },
  engineGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 4,
  },
  engineChip: {
    paddingHorizontal: 14,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    marginRight: 8,
    marginBottom: 8,
    backgroundColor: THEME.background,
  },
  engineChipActive: {
    borderColor: THEME.primary,
    backgroundColor: THEME.primary + '20',
  },
  engineText: {
    color: THEME.textSecondary,
    fontSize: 13,
    fontWeight: '600',
  },
  engineTextActive: {
    color: THEME.primary,
  },
  regRow: {
    marginBottom: 10,
  },
  regInput: {
    backgroundColor: THEME.background,
    color: THEME.text,
    fontSize: 14,
    fontWeight: '600',
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: THEME.border,
    fontFamily: 'monospace',
    marginTop: 4,
  },
  regActions: {
    flexDirection: 'row',
    marginTop: 8,
  },
  regButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: THEME.primary,
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 8,
    marginRight: 10,
    flex: 1,
  },
  regWriteButton: {
    backgroundColor: THEME.accent,
  },
  regButtonText: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: '600',
    marginLeft: 6,
  },
  regResult: {
    backgroundColor: THEME.background,
    borderRadius: 8,
    padding: 10,
    marginTop: 10,
    borderWidth: 1,
    borderColor: THEME.border,
  },
  regResultText: {
    color: THEME.success,
    fontSize: 14,
    fontFamily: 'monospace',
    fontWeight: '600',
  },
  regWriteStatus: {
    fontSize: 13,
    fontWeight: '600',
    marginTop: 6,
    textAlign: 'center',
  },
  fwUpdateButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: THEME.secondary,
    paddingVertical: 14,
    borderRadius: 12,
    marginBottom: 8,
  },
  fwUpdateText: {
    color: '#FFF',
    fontSize: 15,
    fontWeight: '700',
    marginLeft: 8,
  },
  fwWarning: {
    color: THEME.warning,
    fontSize: 12,
    lineHeight: 16,
    marginBottom: 8,
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.8)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContent: {
    backgroundColor: THEME.surface,
    borderRadius: 16,
    padding: 24,
    width: '85%',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: THEME.border,
  },
  modalTitle: {
    color: THEME.text,
    fontSize: 20,
    fontWeight: '700',
    marginBottom: 8,
  },
  modalSubtitle: {
    color: THEME.textSecondary,
    fontSize: 14,
    textAlign: 'center',
    marginBottom: 20,
  },
  progressBar: {
    width: '100%',
    height: 8,
    backgroundColor: THEME.border,
    borderRadius: 4,
    overflow: 'hidden',
    marginBottom: 8,
  },
  progressFill: {
    height: '100%',
    backgroundColor: THEME.primary,
    borderRadius: 4,
  },
  progressText: {
    color: THEME.primary,
    fontSize: 24,
    fontWeight: '800',
    marginBottom: 8,
  },
  modalComplete: {
    color: THEME.success,
    fontSize: 14,
    fontWeight: '600',
    marginTop: 8,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: THEME.warning + '15',
    borderRadius: 8,
    padding: 12,
    marginTop: 8,
    borderWidth: 1,
    borderColor: THEME.warning + '40',
  },
  warningText: {
    color: THEME.warning,
    fontSize: 13,
    fontWeight: '500',
    marginLeft: 8,
    flex: 1,
  },
});
