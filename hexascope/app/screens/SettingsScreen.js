/**
 * SettingsScreen.js — Device settings, channel config, Wi-Fi, calibration
 */

import React, { useState, useCallback } from 'react';
import { View, StyleSheet, ScrollView, Switch, Text } from 'react-native';
import { List, Divider, Button, TextInput, ToggleButton, Card, Paragraph } from 'react-native-paper';
import { HexaScopeProtocol, COMMANDS } from '../utils/protocol';

// Input impedance options
const IMPEDANCE_OPTIONS = ['1 MΩ', '50 Ω'];

// Coupling options
const COUPLING_OPTIONS = ['DC', 'AC', 'GND'];

// Trigger type options
const TRIGGER_TYPES = [
  { label: '↑ Rising', value: 'rising' },
  { label: '↓ Falling', value: 'falling' },
  { label: '↕ Both', value: 'both' },
  { label: '⊢ Pulse', value: 'pulse' },
  { label: '⏱ Timeout', value: 'timeout' },
  { label: '≀ Runt', value: 'runt' },
  { label: '⊞ Window', value: 'window' },
];

export default function SettingsScreen({ navigation }) {
  const [connected, setConnected] = useState(false);
  const [wifiEnabled, setWifiEnabled] = useState(false);
  const [wifiSSID, setWifiSSID] = useState('HexaScope-XXXX');
  const [wifiPassword, setWifiPassword] = useState('scope123');
  const [wifiMode, setWifiMode] = useState('ap');  // 'ap' or 'sta'

  // Per-channel settings
  const [channels, setChannels] = useState([
    { enabled: true, impedance: 0, coupling: 0, vdiv: 5, probe: '1x', label: 'CH1' },
    { enabled: true, impedance: 0, coupling: 0, vdiv: 5, probe: '1x', label: 'CH2' },
    { enabled: true, impedance: 0, coupling: 0, vdiv: 5, probe: '1x', label: 'CH3' },
    { enabled: true, impedance: 0, coupling: 0, vdiv: 5, probe: '1x', label: 'CH4' },
  ]);

  // Digital channel settings
  const [digitalChannels, setDigitalChannels] = useState([
    { enabled: true, threshold: 1.5, label: 'D1' },
    { enabled: true, threshold: 1.5, label: 'D2' },
  ]);

  // Calibration state
  const [calibrating, setCalibrating] = useState(false);

  const toggleChannel = useCallback((index) => {
    setChannels(prev => {
      const next = [...prev];
      next[index] = { ...next[index], enabled: !next[index].enabled };
      return next;
    });
  }, []);

  const toggleDigitalChannel = useCallback((index) => {
    setDigitalChannels(prev => {
      const next = [...prev];
      next[index] = { ...next[index], enabled: !next[index].enabled };
      return next;
    });
  }, []);

  const handleCalibrate = useCallback(() => {
    setCalibrating(true);
    // Send calibration command to device
    setTimeout(() => {
      setCalibrating(false);
    }, 5000);
  }, []);

  const handleWifiToggle = useCallback(() => {
    setWifiEnabled(prev => !prev);
  }, []);

  return (
    <ScrollView style={styles.container}>
      {/* Device Info Card */}
      <Card style={styles.card}>
        <Card.Title title="Device Info" />
        <Card.Content>
          <Paragraph style={styles.infoText}>
            Model: HexaScope 6-Channel Oscilloscope
          </Paragraph>
          <Paragraph style={styles.infoText}>
            Firmware: v1.0.0
          </Paragraph>
          <Paragraph style={styles.infoText}>
            FPGA: Lattice ECP5-5G (45K LUTs)
          </Paragraph>
          <Paragraph style={styles.infoText}>
            Sample Rate: 250 MS/s (analog), 200 MS/s (digital)
          </Paragraph>
          <Paragraph style={styles.infoText}>
            Memory: 128 Mpts/channel
          </Paragraph>
          <Paragraph style={styles.infoText}>
            USB: {connected ? 'Connected (USB 3.0)' : 'Disconnected'}
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Analog Channel Settings */}
      <Card style={styles.card}>
        <Card.Title title="Analog Channels" />
        <Card.Content>
          {channels.map((ch, index) => (
            <View key={index}>
              <List.Item
                title={`${ch.label} ${ch.enabled ? '●' : '○'}`}
                description={`${IMPEDANCE_OPTIONS[ch.impedance]} | ${COUPLING_OPTIONS[ch.coupling]} | Probe: ${ch.probe}`}
                left={props => (
                  <Switch
                    value={ch.enabled}
                    onValueChange={() => toggleChannel(index)}
                    color={index === 0 ? '#FFD700' :
                           index === 1 ? '#00FF00' :
                           index === 2 ? '#FF4444' : '#4488FF'}
                  />
                )}
                right={props => (
                  <View style={styles.channelButtons}>
                    <ToggleButton.Row onValueChange={(v) => {
                      setChannels(prev => {
                        const next = [...prev];
                        next[index] = { ...next[index], coupling: parseInt(v) };
                        return next;
                      });
                    }} value={String(ch.coupling)}>
                      <ToggleButton icon="dc" value="0" />
                      <ToggleButton icon="ac" value="1" />
                      <ToggleButton icon="ground" value="2" />
                    </ToggleButton.Row>
                  </View>
                )}
              />
              {index < 3 && <Divider />}
            </View>
          ))}
        </Card.Content>
      </Card>

      {/* Digital Channel Settings */}
      <Card style={styles.card}>
        <Card.Title title="Digital Channels" />
        <Card.Content>
          {digitalChannels.map((ch, index) => (
            <View key={index}>
              <List.Item
                title={`${ch.label} — Threshold: ${ch.threshold.toFixed(2)}V`}
                left={props => (
                  <Switch
                    value={ch.enabled}
                    onValueChange={() => toggleDigitalChannel(index)}
                    color={index === 0 ? '#FF00FF' : '#00FFFF'}
                  />
                )}
                right={props => (
                  <View style={styles.thresholdControl}>
                    <TextInput
                      mode="outlined"
                      keyboardType="decimal-pad"
                      value={String(ch.threshold)}
                      onChangeText={(v) => {
                        const val = parseFloat(v);
                        if (!isNaN(val) && val >= 0 && val <= 5) {
                          setDigitalChannels(prev => {
                            const next = [...prev];
                            next[index] = { ...next[index], threshold: val };
                            return next;
                          });
                        }
                      }}
                      style={styles.thresholdInput}
                      dense
                    />
                  </View>
                )}
              />
              {index < 1 && <Divider />}
            </View>
          ))}
        </Card.Content>
      </Card>

      {/* Wi-Fi Settings */}
      <Card style={styles.card}>
        <Card.Title title="Wi-Fi Settings" />
        <Card.Content>
          <View style={styles.row}>
            <Text style={styles.label}>Wi-Fi</Text>
            <Switch value={wifiEnabled} onValueChange={handleWifiToggle} />
          </View>
          {wifiEnabled && (
            <>
              <View style={styles.row}>
                <ToggleButton.Row
                  onValueChange={setWifiMode}
                  value={wifiMode}
                >
                  <ToggleButton icon="access-point" value="ap" label="AP" />
                  <ToggleButton icon="wifi" value="sta" label="STA" />
                </ToggleButton.Row>
              </View>
              <TextInput
                label="SSID"
                mode="outlined"
                value={wifiSSID}
                onChangeText={setWifiSSID}
                style={styles.input}
              />
              <TextInput
                label="Password"
                mode="outlined"
                value={wifiPassword}
                onChangeText={setWifiPassword}
                secureTextEntry
                style={styles.input}
              />
            </>
          )}
        </Card.Content>
      </Card>

      {/* Calibration */}
      <Card style={styles.card}>
        <Card.Title title="Calibration" />
        <Card.Content>
          <Paragraph style={styles.infoText}>
            Run self-calibration to adjust gain and offset for all channels.
            This requires all inputs to be disconnected or shorted.
          </Paragraph>
          <Button
            mode="contained"
            onPress={handleCalibrate}
            loading={calibrating}
            disabled={calibrating}
            style={styles.calibrateButton}
          >
            {calibrating ? 'Calibrating...' : 'Run Self-Calibration'}
          </Button>
        </Card.Content>
      </Card>

      {/* Trigger Settings */}
      <Card style={styles.card}>
        <Card.Title title="Trigger Configuration" />
        <Card.Content>
          {TRIGGER_TYPES.map((type) => (
            <List.Item
              key={type.value}
              title={type.label}
              onPress={() => {}}
            />
          ))}
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0a0a1a',
  },
  card: {
    margin: 8,
    backgroundColor: '#16213e',
  },
  infoText: {
    color: '#cccccc',
    fontSize: 13,
    fontFamily: 'monospace',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingVertical: 8,
  },
  label: {
    color: '#ffffff',
    fontSize: 14,
  },
  channelButtons: {
    flexDirection: 'row',
  },
  thresholdControl: {
    width: 80,
  },
  thresholdInput: {
    width: 80,
    fontSize: 12,
  },
  input: {
    marginVertical: 4,
  },
  calibrateButton: {
    marginTop: 8,
  },
});