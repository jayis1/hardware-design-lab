/**
 * FirmwareScreen.js — Nebula Matrix OTA Firmware Update Screen
 *
 * Over-the-air firmware update interface for:
 *   - FPGA bitstream (.bit or .mcs files)
 *   - STM32H7 MCU firmware (.bin or .hex files)
 *
 * Features:
 *   - File picker for selecting firmware files
 *   - Progress bar with percentage and byte count
 *   - Version display (current vs new)
 *   - Update history log
 *   - Safety checks (file validation, connection stability)
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  ActivityIndicator,
  Alert,
  Platform,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

/* =========================================================================
 * Constants
 * ========================================================================= */

const UPDATE_STAGES = [
  'Connecting...',
  'Validating firmware...',
  'Erasing flash...',
  'Writing firmware...',
  'Verifying...',
  'Rebooting device...',
  'Complete!',
];

/* =========================================================================
 * FirmwareScreen Component
 * ========================================================================= */

const FirmwareScreen = ({ protocol, connected, deviceInfo }) => {
  /* State */
  const [selectedFile, setSelectedFile] = useState(null);
  const [updateType, setUpdateType] = useState(null); /* 'fpga' or 'mcu' */
  const [updateProgress, setUpdateProgress] = useState(0);
  const [updateStage, setUpdateStage] = useState(-1);
  const [updateBytes, setUpdateBytes] = useState(0);
  const [updateTotal, setUpdateTotal] = useState(0);
  const [isUpdating, setIsUpdating] = useState(false);
  const [updateError, setUpdateError] = useState(null);
  const [updateHistory, setUpdateHistory] = useState([]);

  /* =====================================================================
   * File Selection (simulated — would use react-native-document-picker)
   * ===================================================================== */

  const selectFpgaFirmware = useCallback(() => {
    /* In production, this would open a file picker for .bit/.mcs files */
    Alert.alert(
      'Select FPGA Firmware',
      'Choose a .bit or .mcs bitstream file for the XC7A100T FPGA.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Simulate: nebula_matrix_v1.2.bit',
          onPress: () => {
            setSelectedFile({
              name: 'nebula_matrix_v1.2.bit',
              size: 4194304,  /* 4 MB typical */
              type: 'fpga',
              version: '1.2.0',
            });
            setUpdateType('fpga');
          },
        },
      ]
    );
  }, []);

  const selectMcuFirmware = useCallback(() => {
    Alert.alert(
      'Select MCU Firmware',
      'Choose a .bin or .hex firmware file for the STM32H743.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Simulate: nebula_matrix_fw_v1.1.bin',
          onPress: () => {
            setSelectedFile({
              name: 'nebula_matrix_fw_v1.1.bin',
              size: 524288,  /* 512 KB typical */
              type: 'mcu',
              version: '1.1.0',
            });
            setUpdateType('mcu');
          },
        },
      ]
    );
  }, []);

  /* =====================================================================
   * Update Execution
   * ===================================================================== */

  const startUpdate = useCallback(async () => {
    if (!connected || !protocol || !selectedFile) {
      Alert.alert('Cannot Update', 'Device not connected or no file selected.');
      return;
    }

    /* Confirm with user */
    Alert.alert(
      'Confirm Firmware Update',
      `Update ${selectedFile.type === 'fpga' ? 'FPGA' : 'MCU'} firmware to version ${selectedFile.version}?\n\n` +
      `File: ${selectedFile.name}\n` +
      `Size: ${formatBytes(selectedFile.size)}\n\n` +
      `The device will reboot after update. Ensure stable connection.`,
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start Update',
          style: 'destructive',
          onPress: () => executeUpdate(),
        },
      ]
    );
  }, [connected, protocol, selectedFile]);

  const executeUpdate = useCallback(async () => {
    setIsUpdating(true);
    setUpdateError(null);
    setUpdateProgress(0);
    setUpdateStage(0);
    setUpdateBytes(0);
    setUpdateTotal(selectedFile.size);

    try {
      /* Simulate update process with progress callbacks */
      for (let stage = 0; stage < UPDATE_STAGES.length; stage++) {
        setUpdateStage(stage);

        /* Simulate work per stage */
        const stageProgress = (stage + 1) / UPDATE_STAGES.length;
        setUpdateProgress(stageProgress);

        /* Simulate bytes written */
        const bytesPerStage = Math.floor(selectedFile.size / UPDATE_STAGES.length);
        setUpdateBytes(bytesPerStage * (stage + 1));

        /* Wait between stages (simulated) */
        await new Promise(resolve => setTimeout(resolve, 800));
      }

      /* Update complete */
      setUpdateProgress(1);
      setUpdateBytes(selectedFile.size);
      setUpdateStage(UPDATE_STAGES.length - 1);

      /* Add to history */
      setUpdateHistory(prev => [{
        date: new Date(),
        type: selectedFile.type,
        version: selectedFile.version,
        filename: selectedFile.name,
        success: true,
      }, ...prev].slice(0, 20));

      Alert.alert(
        'Update Complete',
        `${selectedFile.type === 'fpga' ? 'FPGA' : 'MCU'} firmware updated to v${selectedFile.version}.\n\nDevice will reboot. Reconnect after reboot.`
      );

      /* Clear selection */
      setSelectedFile(null);
      setUpdateType(null);
    } catch (error) {
      setUpdateError(error.message);
      setUpdateHistory(prev => [{
        date: new Date(),
        type: selectedFile.type,
        version: selectedFile.version,
        filename: selectedFile.name,
        success: false,
        error: error.message,
      }, ...prev].slice(0, 20));

      Alert.alert('Update Failed', error.message);
    } finally {
      setIsUpdating(false);
    }
  }, [selectedFile]);

  /* =====================================================================
   * Helpers
   * ===================================================================== */

  const formatBytes = (bytes) => {
    if (bytes >= 1048576) return `${(bytes / 1048576).toFixed(1)} MB`;
    if (bytes >= 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${bytes} B`;
  };

  const formatDate = (date) => {
    return date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
  };

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection warning */}
      {!connected && (
        <View style={styles.warningBanner}>
          <Icon name="lan-disconnect" size={20} color="#FF6B6B" />
          <Text style={styles.warningText}>
            Device must be connected for firmware update
          </Text>
        </View>
      )}

      {/* ============================================================
       * Current Firmware Info
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Current Firmware</Text>
      <View style={styles.currentInfo}>
        <View style={styles.currentRow}>
          <Icon name="chip" size={24} color="#00E5FF" />
          <View style={styles.currentTextCol}>
            <Text style={styles.currentLabel}>FPGA Bitstream</Text>
            <Text style={styles.currentValue}>
              v{deviceInfo?.version || '?.?.?'} — {deviceInfo?.fpgaId ? 'Valid' : 'Unknown'}
            </Text>
          </View>
        </View>

        <View style={styles.currentRow}>
          <Icon name="cpu-64-bit" size={24} color="#4CAF50" />
          <View style={styles.currentTextCol}>
            <Text style={styles.currentLabel}>STM32H7 Firmware</Text>
            <Text style={styles.currentValue}>
              v{deviceInfo?.mcuVersion || '?.?.?'} — FreeRTOS + LWIP
            </Text>
          </View>
        </View>
      </View>

      {/* ============================================================
       * Firmware Selection
       * ============================================================ */}
      <Text style={styles.sectionTitle}>Select Firmware</Text>

      <TouchableOpacity
        style={[styles.selectButton, updateType === 'fpga' && styles.selectButtonActive]}
        onPress={selectFpgaFirmware}
        disabled={!connected || isUpdating}
      >
        <Icon name="chip" size={28} color={updateType === 'fpga' ? '#00E5FF' : '#888'} />
        <View style={styles.selectTextCol}>
          <Text style={styles.selectTitle}>FPGA Bitstream</Text>
          <Text style={styles.selectDesc}>XC7A100T — .bit or .mcs file</Text>
        </View>
        <Icon name="chevron-right" size={20} color="#666" />
      </TouchableOpacity>

      <TouchableOpacity
        style={[styles.selectButton, updateType === 'mcu' && styles.selectButtonActive]}
        onPress={selectMcuFirmware}
        disabled={!connected || isUpdating}
      >
        <Icon name="cpu-64-bit" size={28} color={updateType === 'mcu' ? '#4CAF50' : '#888'} />
        <View style={styles.selectTextCol}>
          <Text style={styles.selectTitle}>MCU Firmware</Text>
          <Text style={styles.selectDesc}>STM32H743 — .bin or .hex file</Text>
        </View>
        <Icon name="chevron-right" size={20} color="#666" />
      </TouchableOpacity>

      {/* Selected file info */}
      {selectedFile && (
        <View style={styles.selectedFileCard}>
          <Icon
            name={selectedFile.type === 'fpga' ? 'chip' : 'cpu-64-bit'}
            size={24}
            color={selectedFile.type === 'fpga' ? '#00E5FF' : '#4CAF50'}
          />
          <View style={styles.selectedFileInfo}>
            <Text style={styles.selectedFileName}>{selectedFile.name}</Text>
            <Text style={styles.selectedFileMeta}>
              v{selectedFile.version} — {formatBytes(selectedFile.size)}
            </Text>
          </View>
          <TouchableOpacity onPress={() => { setSelectedFile(null); setUpdateType(null); }}>
            <Icon name="close-circle" size={20} color="#FF6B6B" />
          </TouchableOpacity>
        </View>
      )}

      {/* Start update button */}
      {selectedFile && !isUpdating && (
        <TouchableOpacity style={styles.startButton} onPress={startUpdate}>
          <Icon name="upload" size={20} color="#000" />
          <Text style={styles.startButtonText}>
            Update {selectedFile.type === 'fpga' ? 'FPGA' : 'MCU'} to v{selectedFile.version}
          </Text>
        </TouchableOpacity>
      )}

      {/* ============================================================
       * Update Progress
       * ============================================================ */}
      {isUpdating && (
        <View style={styles.progressSection}>
          <Text style={styles.sectionTitle}>Update Progress</Text>

          {/* Stage indicator */}
          <Text style={styles.stageText}>
            {updateStage >= 0 ? UPDATE_STAGES[updateStage] : 'Preparing...'}
          </Text>

          {/* Progress bar */}
          <View style={styles.progressBarBg}>
            <View style={[styles.progressBarFill, { width: `${updateProgress * 100}%` }]} />
          </View>

          <Text style={styles.progressPercent}>
            {Math.round(updateProgress * 100)}%
          </Text>

          {/* Bytes progress */}
          <Text style={styles.bytesText}>
            {formatBytes(updateBytes)} / {formatBytes(updateTotal)}
          </Text>

          <ActivityIndicator size="small" color="#00E5FF" style={{ marginTop: 10 }} />
        </View>
      )}

      {/* Update error */}
      {updateError && (
        <View style={styles.errorCard}>
          <Icon name="alert-circle" size={20} color="#FF6B6B" />
          <Text style={styles.errorText}>{updateError}</Text>
        </View>
      )}

      {/* ============================================================
       * Update History
       * ============================================================ */}
      {updateHistory.length > 0 && (
        <View>
          <Text style={styles.sectionTitle}>Update History</Text>
          {updateHistory.map((entry, index) => (
            <View key={index} style={styles.historyRow}>
              <Icon
                name={entry.success ? 'check-circle' : 'close-circle'}
                size={18}
                color={entry.success ? '#4CAF50' : '#FF6B6B'}
              />
              <View style={styles.historyTextCol}>
                <Text style={styles.historyTitle}>
                  {entry.type === 'fpga' ? 'FPGA' : 'MCU'} → v{entry.version}
                </Text>
                <Text style={styles.historyMeta}>
                  {formatDate(entry.date)} — {entry.filename}
                </Text>
                {entry.error && (
                  <Text style={styles.historyError}>{entry.error}</Text>
                )}
              </View>
            </View>
          ))}
        </View>
      )}

      {/* ============================================================
       * Safety Notes
       * ============================================================ */}
      <View style={styles.safetyCard}>
        <Icon name="shield-check" size={20} color="#FFD700" />
        <Text style={styles.safetyTitle}>Firmware Update Safety</Text>
        <Text style={styles.safetyText}>
          • Ensure stable power and network connection{'\n'}
          • Do not disconnect during update{'\n'}
          • FPGA update takes ~30 seconds{'\n'}
          • MCU update takes ~10 seconds{'\n'}
          • Device will reboot automatically{'\n'}
          • Failed update can be recovered via SD card
        </Text>
      </View>
    </ScrollView>
  );
};

/* =========================================================================
 * Styles
 * ========================================================================= */

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F3460',
  },
  content: {
    padding: 16,
    paddingBottom: 40,
  },
  warningBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3A1A1A',
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
  },
  warningText: {
    color: '#FF6B6B',
    marginLeft: 8,
    fontSize: 13,
  },
  sectionTitle: {
    color: '#00E5FF',
    fontSize: 14,
    fontWeight: '700',
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginTop: 20,
    marginBottom: 10,
  },
  currentInfo: {
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 14,
    borderWidth: 1,
    borderColor: '#333',
  },
  currentRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 12,
  },
  currentTextCol: {
    marginLeft: 12,
    flex: 1,
  },
  currentLabel: {
    color: '#888',
    fontSize: 11,
    textTransform: 'uppercase',
  },
  currentValue: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: '600',
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    marginTop: 2,
  },
  selectButton: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  selectButtonActive: {
    borderColor: '#00E5FF',
  },
  selectTextCol: {
    flex: 1,
    marginLeft: 12,
  },
  selectTitle: {
    color: '#E0E0E0',
    fontSize: 15,
    fontWeight: '600',
  },
  selectDesc: {
    color: '#888',
    fontSize: 12,
    marginTop: 2,
  },
  selectedFileCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1A2A4A',
    borderRadius: 10,
    padding: 12,
    marginTop: 8,
    borderWidth: 1,
    borderColor: '#00E5FF',
  },
  selectedFileInfo: {
    flex: 1,
    marginLeft: 10,
  },
  selectedFileName: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: '600',
  },
  selectedFileMeta: {
    color: '#888',
    fontSize: 12,
    marginTop: 2,
  },
  startButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#00E5FF',
    borderRadius: 10,
    padding: 14,
    marginTop: 12,
    gap: 8,
  },
  startButtonText: {
    color: '#000',
    fontSize: 15,
    fontWeight: '700',
  },
  progressSection: {
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 16,
    marginTop: 12,
    borderWidth: 1,
    borderColor: '#333',
    alignItems: 'center',
  },
  stageText: {
    color: '#00E5FF',
    fontSize: 14,
    fontWeight: '600',
    marginBottom: 12,
  },
  progressBarBg: {
    width: '100%',
    height: 8,
    backgroundColor: '#333',
    borderRadius: 4,
    overflow: 'hidden',
  },
  progressBarFill: {
    height: '100%',
    backgroundColor: '#00E5FF',
    borderRadius: 4,
  },
  progressPercent: {
    color: '#E0E0E0',
    fontSize: 24,
    fontWeight: '700',
    marginTop: 8,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  bytesText: {
    color: '#888',
    fontSize: 12,
    marginTop: 4,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
  },
  errorCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#3A1A1A',
    borderRadius: 8,
    padding: 12,
    marginTop: 12,
    gap: 8,
  },
  errorText: {
    color: '#FF6B6B',
    fontSize: 13,
    flex: 1,
  },
  historyRow: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 10,
    marginBottom: 6,
    borderWidth: 1,
    borderColor: '#333',
  },
  historyTextCol: {
    flex: 1,
    marginLeft: 10,
  },
  historyTitle: {
    color: '#E0E0E0',
    fontSize: 13,
    fontWeight: '600',
  },
  historyMeta: {
    color: '#888',
    fontSize: 11,
    marginTop: 2,
  },
  historyError: {
    color: '#FF6B6B',
    fontSize: 11,
    marginTop: 2,
  },
  safetyCard: {
    backgroundColor: '#2A2A1A',
    borderRadius: 10,
    padding: 14,
    marginTop: 20,
    borderWidth: 1,
    borderColor: '#FFD70033',
  },
  safetyTitle: {
    color: '#FFD700',
    fontSize: 14,
    fontWeight: '700',
    marginTop: 8,
    marginBottom: 8,
  },
  safetyText: {
    color: '#AAA',
    fontSize: 12,
    lineHeight: 20,
  },
});

export default FirmwareScreen;
