/**
 * SettingsScreen.js — Nebula Matrix Configuration Screen
 *
 * Configuration interface for:
 *   - Matrix layout (width, height, panel scan type)
 *   - Network settings (IP address, Art-Net universe, sACN priority)
 *   - Color calibration (per-panel gamma, white point)
 *   - Panel mapping (universe-to-pixel mapping)
 *   - Device identity (name, location)
 */

import React, { useState, useEffect, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TextInput,
  TouchableOpacity,
  Switch,
  Alert,
  ActivityIndicator,
  Platform,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

/* =========================================================================
 * Constants
 * ========================================================================= */

const SCAN_TYPES = [
  { id: 32, label: '1/32 Scan', desc: 'Standard 64×64 panels' },
  { id: 16, label: '1/16 Scan', desc: '64×32 or 32×32 panels' },
  { id: 8,  label: '1/8 Scan', desc: '32×16 panels' },
  { id: 4,  label: '1/4 Scan', desc: '16×16 or smaller panels' },
];

const DITHER_MODES = [
  { id: 0, label: 'None' },
  { id: 1, label: 'Floyd-Steinberg' },
  { id: 2, label: 'Atkinson' },
  { id: 3, label: 'Blue Noise' },
  { id: 4, label: 'Temporal' },
  { id: 5, label: 'Hybrid' },
];

/* =========================================================================
 * SettingsScreen Component
 * ========================================================================= */

const SettingsScreen = ({ protocol, connected, deviceInfo }) => {
  /* Matrix settings */
  const [matrixWidth, setMatrixWidth] = useState('256');
  const [matrixHeight, setMatrixHeight] = useState('256');
  const [panelScan, setPanelScan] = useState(32);
  const [ditherMode, setDitherMode] = useState(5);
  const [ditherStrength, setDitherStrength] = useState('128');

  /* Network settings */
  const [ipAddress, setIpAddress] = useState('192.168.1.100');
  const [netmask, setNetmask] = useState('255.255.255.0');
  const [gateway, setGateway] = useState('192.168.1.1');
  const [artnetUniverse, setArtnetUniverse] = useState('0');
  const [sacnPriority, setSacnPriority] = useState('100');
  const [dhcpEnabled, setDhcpEnabled] = useState(false);

  /* Color calibration */
  const [gammaR, setGammaR] = useState('2.2');
  const [gammaG, setGammaG] = useState('2.2');
  const [gammaB, setGammaB] = useState('2.2');
  const [whitePointR, setWhitePointR] = useState('255');
  const [whitePointG, setWhitePointG] = useState('255');
  const [whitePointB, setWhitePointB] = useState('255');

  /* Device identity */
  const [deviceName, setDeviceName] = useState('Nebula Matrix');
  const [deviceLocation, setDeviceLocation] = useState('');

  /* State */
  const [isSaving, setIsSaving] = useState(false);
  const [saveStatus, setSaveStatus] = useState('');
  const [activeSection, setActiveSection] = useState('matrix');

  /* =====================================================================
   * Load settings from device on connection
   * ===================================================================== */

  useEffect(() => {
    if (connected && deviceInfo) {
      if (deviceInfo.matrixWidth) setMatrixWidth(String(deviceInfo.matrixWidth));
      if (deviceInfo.matrixHeight) setMatrixHeight(String(deviceInfo.matrixHeight));
      if (deviceInfo.panelScan) setPanelScan(deviceInfo.panelScan);
      if (deviceInfo.ditherMode != null) setDitherMode(deviceInfo.ditherMode);
      if (deviceInfo.ipAddress) setIpAddress(deviceInfo.ipAddress);
    }
  }, [connected, deviceInfo]);

  /* =====================================================================
   * Save handlers
   * ===================================================================== */

  const saveMatrixSettings = useCallback(async () => {
    if (!connected || !protocol) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    setIsSaving(true);
    setSaveStatus('Saving matrix settings...');

    try {
      await protocol.setMatrixConfig(
        parseInt(matrixWidth, 10),
        parseInt(matrixHeight, 10),
        panelScan
      );
      await protocol.setDitherConfig(ditherMode, parseInt(ditherStrength, 10));
      setSaveStatus('Matrix settings saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      Alert.alert('Save Failed', error.message);
      setSaveStatus('Save failed');
    } finally {
      setIsSaving(false);
    }
  }, [connected, protocol, matrixWidth, matrixHeight, panelScan, ditherMode, ditherStrength]);

  const saveNetworkSettings = useCallback(async () => {
    if (!connected || !protocol) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    setIsSaving(true);
    setSaveStatus('Saving network settings...');

    try {
      await protocol.setNetworkConfig(
        ipAddress,
        netmask,
        gateway,
        dhcpEnabled,
        parseInt(artnetUniverse, 10),
        parseInt(sacnPriority, 10)
      );
      setSaveStatus('Network settings saved ✓ (reboot required)');
      setTimeout(() => setSaveStatus(''), 5000);
    } catch (error) {
      Alert.alert('Save Failed', error.message);
      setSaveStatus('Save failed');
    } finally {
      setIsSaving(false);
    }
  }, [connected, protocol, ipAddress, netmask, gateway, dhcpEnabled, artnetUniverse, sacnPriority]);

  const saveColorSettings = useCallback(async () => {
    if (!connected || !protocol) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    setIsSaving(true);
    setSaveStatus('Saving color calibration...');

    try {
      await protocol.setGammaLUTs(
        parseFloat(gammaR),
        parseFloat(gammaG),
        parseFloat(gammaB)
      );
      await protocol.setWhitePoint(
        parseInt(whitePointR, 10),
        parseInt(whitePointG, 10),
        parseInt(whitePointB, 10)
      );
      setSaveStatus('Color calibration saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      Alert.alert('Save Failed', error.message);
      setSaveStatus('Save failed');
    } finally {
      setIsSaving(false);
    }
  }, [connected, protocol, gammaR, gammaG, gammaB, whitePointR, whitePointG, whitePointB]);

  const saveIdentitySettings = useCallback(async () => {
    if (!connected || !protocol) {
      Alert.alert('Not Connected', 'Connect to device first.');
      return;
    }

    setIsSaving(true);
    setSaveStatus('Saving device identity...');

    try {
      await protocol.setDeviceIdentity(deviceName, deviceLocation);
      setSaveStatus('Identity saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      Alert.alert('Save Failed', error.message);
      setSaveStatus('Save failed');
    } finally {
      setIsSaving(false);
    }
  }, [connected, protocol, deviceName, deviceLocation]);

  /* =====================================================================
   * Render
   * ===================================================================== */

  return (
    <ScrollView style={styles.container} contentContainerStyle={styles.content}>
      {/* Connection warning */}
      {!connected && (
        <View style={styles.warningBanner}>
          <Icon name="lan-disconnect" size={20} color="#FF6B6B" />
          <Text style={styles.warningText}>Not connected — settings are read-only</Text>
        </View>
      )}

      {/* Save status */}
      {saveStatus !== '' && (
        <View style={styles.statusBanner}>
          {isSaving && <ActivityIndicator size="small" color="#00E5FF" />}
          <Text style={styles.statusText}>{saveStatus}</Text>
        </View>
      )}

      {/* ============================================================
       * Section Tabs
       * ============================================================ */}
      <View style={styles.sectionTabs}>
        {['matrix', 'network', 'color', 'identity'].map((section) => (
          <TouchableOpacity
            key={section}
            style={[styles.sectionTab, activeSection === section && styles.sectionTabActive]}
            onPress={() => setActiveSection(section)}
          >
            <Text style={[styles.sectionTabText, activeSection === section && styles.sectionTabTextActive]}>
              {section.charAt(0).toUpperCase() + section.slice(1)}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* ============================================================
       * Matrix Settings
       * ============================================================ */}
      {activeSection === 'matrix' && (
        <View>
          <Text style={styles.fieldLabel}>Matrix Width (pixels)</Text>
          <TextInput
            style={styles.textInput}
            value={matrixWidth}
            onChangeText={setMatrixWidth}
            keyboardType="number-pad"
            maxLength={4}
            editable={connected}
            placeholder="256"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Matrix Height (pixels)</Text>
          <TextInput
            style={styles.textInput}
            value={matrixHeight}
            onChangeText={setMatrixHeight}
            keyboardType="number-pad"
            maxLength={4}
            editable={connected}
            placeholder="256"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Panel Scan Type</Text>
          <View style={styles.optionGrid}>
            {SCAN_TYPES.map((scan) => (
              <TouchableOpacity
                key={scan.id}
                style={[styles.optionButton, panelScan === scan.id && styles.optionButtonActive]}
                onPress={() => setPanelScan(scan.id)}
                disabled={!connected}
              >
                <Text style={[styles.optionLabel, panelScan === scan.id && styles.optionLabelActive]}>
                  {scan.label}
                </Text>
                <Text style={styles.optionDesc}>{scan.desc}</Text>
              </TouchableOpacity>
            ))}
          </View>

          <Text style={styles.fieldLabel}>Dithering Mode</Text>
          <View style={styles.optionRow}>
            {DITHER_MODES.map((mode) => (
              <TouchableOpacity
                key={mode.id}
                style={[styles.chipButton, ditherMode === mode.id && styles.chipButtonActive]}
                onPress={() => setDitherMode(mode.id)}
                disabled={!connected}
              >
                <Text style={[styles.chipText, ditherMode === mode.id && styles.chipTextActive]}>
                  {mode.label}
                </Text>
              </TouchableOpacity>
            ))}
          </View>

          <Text style={styles.fieldLabel}>Dither Strength (0-255)</Text>
          <TextInput
            style={styles.textInput}
            value={ditherStrength}
            onChangeText={setDitherStrength}
            keyboardType="number-pad"
            maxLength={3}
            editable={connected}
            placeholder="128"
            placeholderTextColor="#555"
          />

          <TouchableOpacity
            style={[styles.saveButton, !connected && styles.saveButtonDisabled]}
            onPress={saveMatrixSettings}
            disabled={!connected || isSaving}
          >
            <Icon name="content-save" size={18} color="#fff" />
            <Text style={styles.saveButtonText}>Save Matrix Settings</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* ============================================================
       * Network Settings
       * ============================================================ */}
      {activeSection === 'network' && (
        <View>
          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>DHCP</Text>
            <Switch
              value={dhcpEnabled}
              onValueChange={setDhcpEnabled}
              trackColor={{ false: '#333', true: '#4CAF50' }}
              thumbColor={dhcpEnabled ? '#fff' : '#888'}
              disabled={!connected}
            />
          </View>

          <Text style={styles.fieldLabel}>IP Address</Text>
          <TextInput
            style={[styles.textInput, dhcpEnabled && styles.textInputDisabled]}
            value={ipAddress}
            onChangeText={setIpAddress}
            keyboardType="decimal-pad"
            editable={connected && !dhcpEnabled}
            placeholder="192.168.1.100"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Netmask</Text>
          <TextInput
            style={[styles.textInput, dhcpEnabled && styles.textInputDisabled]}
            value={netmask}
            onChangeText={setNetmask}
            keyboardType="decimal-pad"
            editable={connected && !dhcpEnabled}
            placeholder="255.255.255.0"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Gateway</Text>
          <TextInput
            style={[styles.textInput, dhcpEnabled && styles.textInputDisabled]}
            value={gateway}
            onChangeText={setGateway}
            keyboardType="decimal-pad"
            editable={connected && !dhcpEnabled}
            placeholder="192.168.1.1"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Art-Net Start Universe</Text>
          <TextInput
            style={styles.textInput}
            value={artnetUniverse}
            onChangeText={setArtnetUniverse}
            keyboardType="number-pad"
            maxLength={5}
            editable={connected}
            placeholder="0"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>sACN Priority (0-200)</Text>
          <TextInput
            style={styles.textInput}
            value={sacnPriority}
            onChangeText={setSacnPriority}
            keyboardType="number-pad"
            maxLength={3}
            editable={connected}
            placeholder="100"
            placeholderTextColor="#555"
          />

          <TouchableOpacity
            style={[styles.saveButton, !connected && styles.saveButtonDisabled]}
            onPress={saveNetworkSettings}
            disabled={!connected || isSaving}
          >
            <Icon name="content-save" size={18} color="#fff" />
            <Text style={styles.saveButtonText}>Save Network Settings</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* ============================================================
       * Color Calibration
       * ============================================================ */}
      {activeSection === 'color' && (
        <View>
          <Text style={styles.sectionDesc}>
            Gamma correction values for each color channel.
            Standard sRGB uses ~2.2, DCI-P3 uses ~2.6.
          </Text>

          <Text style={styles.fieldLabel}>Red Gamma</Text>
          <TextInput
            style={styles.textInput}
            value={gammaR}
            onChangeText={setGammaR}
            keyboardType="decimal-pad"
            editable={connected}
            placeholder="2.2"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Green Gamma</Text>
          <TextInput
            style={styles.textInput}
            value={gammaG}
            onChangeText={setGammaG}
            keyboardType="decimal-pad"
            editable={connected}
            placeholder="2.2"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Blue Gamma</Text>
          <TextInput
            style={styles.textInput}
            value={gammaB}
            onChangeText={setGammaB}
            keyboardType="decimal-pad"
            editable={connected}
            placeholder="2.2"
            placeholderTextColor="#555"
          />

          <Text style={styles.sectionDesc}>
            White point calibration (0-255 per channel).
            Adjust to match panel color temperature.
          </Text>

          <View style={styles.whitePointRow}>
            <View style={styles.whitePointCol}>
              <Text style={styles.fieldLabel}>R</Text>
              <TextInput
                style={styles.textInputSmall}
                value={whitePointR}
                onChangeText={setWhitePointR}
                keyboardType="number-pad"
                maxLength={3}
                editable={connected}
              />
            </View>
            <View style={styles.whitePointCol}>
              <Text style={styles.fieldLabel}>G</Text>
              <TextInput
                style={styles.textInputSmall}
                value={whitePointG}
                onChangeText={setWhitePointG}
                keyboardType="number-pad"
                maxLength={3}
                editable={connected}
              />
            </View>
            <View style={styles.whitePointCol}>
              <Text style={styles.fieldLabel}>B</Text>
              <TextInput
                style={styles.textInputSmall}
                value={whitePointB}
                onChangeText={setWhitePointB}
                keyboardType="number-pad"
                maxLength={3}
                editable={connected}
              />
            </View>
          </View>

          <TouchableOpacity
            style={[styles.saveButton, !connected && styles.saveButtonDisabled]}
            onPress={saveColorSettings}
            disabled={!connected || isSaving}
          >
            <Icon name="palette" size={18} color="#fff" />
            <Text style={styles.saveButtonText}>Save Color Calibration</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* ============================================================
       * Device Identity
       * ============================================================ */}
      {activeSection === 'identity' && (
        <View>
          <Text style={styles.fieldLabel}>Device Name</Text>
          <TextInput
            style={styles.textInput}
            value={deviceName}
            onChangeText={setDeviceName}
            maxLength={32}
            editable={connected}
            placeholder="Nebula Matrix"
            placeholderTextColor="#555"
          />

          <Text style={styles.fieldLabel}>Location</Text>
          <TextInput
            style={styles.textInput}
            value={deviceLocation}
            onChangeText={setDeviceLocation}
            maxLength={64}
            editable={connected}
            placeholder="Stage Left, Main Video Wall"
            placeholderTextColor="#555"
          />

          <TouchableOpacity
            style={[styles.saveButton, !connected && styles.saveButtonDisabled]}
            onPress={saveIdentitySettings}
            disabled={!connected || isSaving}
          >
            <Icon name="content-save" size={18} color="#fff" />
            <Text style={styles.saveButtonText}>Save Identity</Text>
          </TouchableOpacity>
        </View>
      )}
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
  statusBanner: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 10,
    marginBottom: 12,
    gap: 8,
  },
  statusText: {
    color: '#00E5FF',
    fontSize: 13,
  },
  sectionTabs: {
    flexDirection: 'row',
    backgroundColor: '#16213E',
    borderRadius: 10,
    padding: 4,
    marginBottom: 16,
  },
  sectionTab: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    borderRadius: 8,
  },
  sectionTabActive: {
    backgroundColor: '#0F3460',
  },
  sectionTabText: {
    color: '#888',
    fontSize: 12,
    fontWeight: '600',
  },
  sectionTabTextActive: {
    color: '#00E5FF',
  },
  sectionDesc: {
    color: '#888',
    fontSize: 12,
    marginBottom: 12,
    lineHeight: 18,
  },
  fieldLabel: {
    color: '#AAA',
    fontSize: 12,
    fontWeight: '600',
    textTransform: 'uppercase',
    letterSpacing: 0.5,
    marginTop: 14,
    marginBottom: 6,
  },
  textInput: {
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    color: '#E0E0E0',
    fontSize: 15,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    borderWidth: 1,
    borderColor: '#333',
  },
  textInputDisabled: {
    opacity: 0.5,
  },
  textInputSmall: {
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 10,
    color: '#E0E0E0',
    fontSize: 15,
    fontFamily: Platform.OS === 'ios' ? 'Menlo' : 'monospace',
    borderWidth: 1,
    borderColor: '#333',
    textAlign: 'center',
  },
  optionGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  optionButton: {
    width: '47%',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: '#333',
  },
  optionButtonActive: {
    borderColor: '#00E5FF',
    backgroundColor: '#1A2A4A',
  },
  optionLabel: {
    color: '#E0E0E0',
    fontSize: 13,
    fontWeight: '600',
  },
  optionLabelActive: {
    color: '#00E5FF',
  },
  optionDesc: {
    color: '#888',
    fontSize: 11,
    marginTop: 2,
  },
  optionRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 6,
  },
  chipButton: {
    backgroundColor: '#16213E',
    borderRadius: 16,
    paddingVertical: 8,
    paddingHorizontal: 14,
    borderWidth: 1,
    borderColor: '#333',
  },
  chipButtonActive: {
    borderColor: '#00E5FF',
    backgroundColor: '#1A2A4A',
  },
  chipText: {
    color: '#888',
    fontSize: 12,
    fontWeight: '600',
  },
  chipTextActive: {
    color: '#00E5FF',
  },
  switchRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: '#16213E',
    borderRadius: 8,
    padding: 12,
    marginBottom: 12,
  },
  switchLabel: {
    color: '#E0E0E0',
    fontSize: 14,
    fontWeight: '500',
  },
  whitePointRow: {
    flexDirection: 'row',
    gap: 10,
  },
  whitePointCol: {
    flex: 1,
  },
  saveButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#00E5FF',
    borderRadius: 10,
    padding: 14,
    marginTop: 20,
    gap: 8,
  },
  saveButtonDisabled: {
    backgroundColor: '#333',
    opacity: 0.5,
  },
  saveButtonText: {
    color: '#000',
    fontSize: 15,
    fontWeight: '700',
  },
});

export default SettingsScreen;
