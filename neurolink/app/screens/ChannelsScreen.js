/**
 * NeuroLink Channels Screen — Channel configuration, impedance checking, gain control
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Switch,
  FlatList,
} from 'react-native';
import { useBle } from '../components/BleContext';
import ChannelCard from '../components/ChannelCard';
import { COMMAND_IDS, buildSetGainCommand, buildEnableChannelsCommand } from '../utils/protocol';

const GAIN_OPTIONS = [1, 2, 4, 6, 8, 12, 24];
const CHANNEL_LABELS = [
  'Fp1', 'Fp2', 'F3', 'F4', 'C3', 'C4', 'P3', 'P4',
  'O1', 'O2', 'F7', 'F8', 'T3', 'T4', 'T5', 'T6',
  'Fz', 'Cz', 'Pz', 'Oz', 'A1', 'A2', 'EMG1', 'EMG2',
  'EMG3', 'EMG4', 'EMG5', 'EMG6', 'EMG7', 'EMG8', 'GND', 'REF',
];

export default function ChannelsScreen() {
  const {
    connected,
    channelData,
    impedanceData,
    signalQuality,
    sendCommand,
    checkImpedance,
  } = useBle();

  const [channelEnabled, setChannelEnabled] = useState(
    new Array(32).fill(true)
  );
  const [channelGain, setChannelGain] = useState(
    new Array(32).fill(6)
  );
  const [checkingImpedance, setCheckingImpedance] = useState(false);
  const [selectedGroup, setSelectedGroup] = useState(0); // 0=EEG, 1=EMG

  const handleToggleChannel = useCallback((index) => {
    setChannelEnabled((prev) => {
      const next = [...prev];
      next[index] = !next[index];

      // Send enable/disable command
      const enabledChannels = next
        .map((enabled, i) => (enabled ? i : -1))
        .filter((i) => i >= 0);

      if (connected) {
        sendCommand(COMMAND_IDS.ENABLE_CHANNELS, enabledChannels);
      }

      return next;
    });
  }, [connected, sendCommand]);

  const handleGainChange = useCallback((channel, gain) => {
    setChannelGain((prev) => {
      const next = [...prev];
      next[channel] = gain;

      if (connected) {
        const cmd = buildSetGainCommand(channel, gain);
        sendCommand(COMMAND_IDS.SET_CHANNEL_GAIN, Array.from(cmd).slice(5));
      }

      return next;
    });
  }, [connected, sendCommand]);

  const handleImpedanceCheck = useCallback(async () => {
    if (!connected || checkingImpedance) return;

    setCheckingImpedance(true);
    try {
      await checkImpedance();
    } finally {
      setCheckingImpedance(false);
    }
  }, [connected, checkingImpedance, checkImpedance]);

  const handleSelectAll = useCallback(() => {
    setChannelEnabled(new Array(32).fill(true));
    if (connected) {
      const allChannels = Array.from({ length: 32 }, (_, i) => i);
      sendCommand(COMMAND_IDS.ENABLE_CHANNELS, allChannels);
    }
  }, [connected, sendCommand]);

  const handleDeselectAll = useCallback(() => {
    setChannelEnabled(new Array(32).fill(false));
    if (connected) {
      sendCommand(COMMAND_IDS.DISABLE_CHANNELS, []);
    }
  }, [connected, sendCommand]);

  const renderChannel = ({ item: index }) => {
    const isEnabled = channelEnabled[index];
    const gain = channelGain[index];
    const value = channelData?.[index] ?? 0;
    const imp = impedanceData?.[index] ?? 0;
    const quality = signalQuality?.[index] ?? 100;

    return (
      <View style={styles.channelRow}>
        <View style={styles.channelHeader}>
          <Switch
            value={isEnabled}
            onValueChange={() => handleToggleChannel(index)}
            trackColor={{ false: '#2A3A5C', true: '#1A5C3A' }}
            thumbColor={isEnabled ? '#4CAF50' : '#78909C'}
          />
          <Text style={[styles.channelLabel, !isEnabled && styles.channelLabelDisabled]}>
            {CHANNEL_LABELS[index]} (CH {index})
          </Text>
          <View style={[styles.qualityDot, {
            backgroundColor: quality >= 80 ? '#4CAF50' : quality >= 50 ? '#FFC107' : '#F44336',
          }]} />
        </View>

        <View style={styles.channelBody}>
          <ChannelCard
            channelNumber={index}
            label={CHANNEL_LABELS[index]}
            value={value}
            impedance={imp}
            quality={quality}
            gain={gain}
            isActive={isEnabled}
          />

          {/* Gain Selector */}
          <View style={styles.gainRow}>
            <Text style={styles.gainLabel}>Gain:</Text>
            {GAIN_OPTIONS.map((g) => (
              <TouchableOpacity
                key={g}
                style={[styles.gainBtn, gain === g && styles.gainBtnActive]}
                onPress={() => handleGainChange(index, g)}
              >
                <Text style={[styles.gainBtnText, gain === g && styles.gainBtnTextActive]}>
                  ×{g}
                </Text>
              </TouchableOpacity>
            ))}
          </View>
        </View>
      </View>
    );
  };

  const channelGroups = [
    { name: 'EEG Channels', start: 0, end: 21 },
    { name: 'EMG Channels', start: 22, end: 29 },
  ];

  const groupRange = channelGroups[selectedGroup];

  return (
    <View style={styles.container}>
      {/* Group Selector */}
      <View style={styles.groupSelector}>
        <TouchableOpacity
          style={[styles.groupBtn, selectedGroup === 0 && styles.groupBtnActive]}
          onPress={() => setSelectedGroup(0)}
        >
          <Text style={[styles.groupBtnText, selectedGroup === 0 && styles.groupBtnTextActive]}>
            EEG (22ch)
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.groupBtn, selectedGroup === 1 && styles.groupBtnActive]}
          onPress={() => setSelectedGroup(1)}
        >
          <Text style={[styles.groupBtnText, selectedGroup === 1 && styles.groupBtnTextActive]}>
            EMG (8ch)
          </Text>
        </TouchableOpacity>
      </View>

      {/* Action Buttons */}
      <View style={styles.actionRow}>
        <TouchableOpacity style={styles.actionBtn} onPress={handleSelectAll}>
          <Text style={styles.actionBtnText}>Select All</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.actionBtn} onPress={handleDeselectAll}>
          <Text style={styles.actionBtnText}>None</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionBtn, styles.impedanceBtn]}
          onPress={handleImpedanceCheck}
          disabled={!connected || checkingImpedance}
        >
          <Text style={styles.actionBtnText}>
            {checkingImpedance ? 'Checking...' : 'Check Impedance'}
          </Text>
        </TouchableOpacity>
      </View>

      {/* Channel List */}
      <FlatList
        data={Array.from(
          { length: groupRange.end - groupRange.start + 1 },
          (_, i) => groupRange.start + i
        )}
        renderItem={renderChannel}
        keyExtractor={(item) => item.toString()}
        contentContainerStyle={styles.channelList}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0F0F1A',
    padding: 16,
  },
  groupSelector: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  groupBtn: {
    flex: 1,
    backgroundColor: '#1E2A45',
    borderRadius: 8,
    paddingVertical: 10,
    alignItems: 'center',
    marginHorizontal: 4,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  groupBtnActive: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
  },
  groupBtnText: {
    color: '#78909C',
    fontSize: 14,
    fontWeight: '600',
  },
  groupBtnTextActive: {
    color: '#4FC3F7',
  },
  actionRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 12,
  },
  actionBtn: {
    backgroundColor: '#1E2A45',
    borderRadius: 8,
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  impedanceBtn: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
    flex: 2,
    alignItems: 'center',
  },
  actionBtnText: {
    color: '#E8E8E8',
    fontSize: 12,
    fontWeight: '600',
  },
  channelList: {
    paddingBottom: 16,
  },
  channelRow: {
    marginBottom: 4,
  },
  channelHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  channelLabel: {
    color: '#E8E8E8',
    fontSize: 13,
    fontWeight: '600',
    flex: 1,
    marginLeft: 8,
    fontFamily: 'monospace',
  },
  channelLabelDisabled: {
    color: '#607D8B',
  },
  qualityDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  channelBody: {
    marginLeft: 50,
  },
  gainRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 6,
    marginBottom: 4,
  },
  gainLabel: {
    color: '#607D8B',
    fontSize: 11,
    marginRight: 6,
  },
  gainBtn: {
    backgroundColor: '#1E2A45',
    borderRadius: 4,
    paddingHorizontal: 8,
    paddingVertical: 4,
    marginHorizontal: 2,
    borderWidth: 1,
    borderColor: '#2A3A5C',
  },
  gainBtnActive: {
    backgroundColor: '#1A3A5C',
    borderColor: '#4FC3F7',
  },
  gainBtnText: {
    color: '#78909C',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  gainBtnTextActive: {
    color: '#4FC3F7',
    fontWeight: '700',
  },
});