/**
 * @file    DiveHistoryScreen.js
 * @brief   Dive history list with replay view and data export.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, FlatList, TouchableOpacity,
  Modal, ScrollView, Alert, Share,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { useTideBand } from '../services/TideBandContext';
import { profileToCSV, formatDepth, formatSpeed } from '../utils/protocol';
import DiveProfileChart from '../components/DiveProfileChart';

const DIVE_STORAGE_KEY = '@tideband_dives';

export default function DiveHistoryScreen() {
  const { diveCount, profileData, connected, sendCommand, units } = useTideBand();
  const [savedDives, setSavedDives] = useState([]);
  const [selectedDive, setSelectedDive] = useState(null);
  const [modalVisible, setModalVisible] = useState(false);
  const [replayData, setReplayData] = useState([]);

  // Load saved dives from storage
  useEffect(() => {
    loadDives();
  }, []);

  const loadDives = async () => {
    try {
      const stored = await AsyncStorage.getItem(DIVE_STORAGE_KEY);
      if (stored) {
        setSavedDives(JSON.parse(stored));
      }
    } catch (err) {
      console.error('Failed to load dives:', err);
    }
  };

  const saveDive = async (dive) => {
    try {
    const updated = [...savedDives, dive];
      setSavedDives(updated);
      await AsyncStorage.setItem(DIVE_STORAGE_KEY, JSON.stringify(updated));
    } catch (err) {
      console.error('Failed to save dive:', err);
    }
  };

  // When dive ends, save the current profile data
  const handleExportCurrentDive = () => {
    if (profileData.length === 0) {
      Alert.alert('No Data', 'No profile data to export.');
      return;
    }

    const dive = {
      id: Date.now(),
      date: new Date().toISOString(),
      duration: profileData.length > 0
        ? profileData[profileData.length - 1].timestamp - profileData[0].timestamp
        : 0,
      maxDepth: Math.max(...profileData.map(p => p.depth)),
      avgSpeed: profileData.reduce((sum, p) =>
        sum + Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2), 0
      ) / profileData.length,
      recordCount: profileData.length,
      records: profileData,
    };

    saveDive(dive);
    Alert.alert('Saved', `Dive saved with ${profileData.length} samples.`);
  };

  const handleExportCSV = async (dive) => {
    const csv = profileToCSV(dive.records);
    const filename = `tideband_dive_${dive.id}.csv`;

    try {
      const RNFS = require('react-native-fs');
      const path = `${RNFS.DocumentDirectoryPath}/${filename}`;
      await RNFS.writeFile(path, csv, 'utf8');
      await Share.share({
        url: path,
        title: `TideBand Dive Export — ${dive.date}`,
      });
    } catch (err) {
      Alert.alert('Export Failed', err.message);
    }
  };

  const handleDivePress = (dive) => {
    setSelectedDive(dive);
    setReplayData(dive.records);
    setModalVisible(true);
  };

  const handleDeleteDive = (diveId) => {
    Alert.alert(
      'Delete Dive',
      'Are you sure you want to delete this dive?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            const updated = savedDives.filter(d => d.id !== diveId);
            setSavedDives(updated);
            await AsyncStorage.setItem(DIVE_STORAGE_KEY, JSON.stringify(updated));
          },
        },
      ]
    );
  };

  const formatDuration = (seconds) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  const renderDiveItem = ({ item }) => (
    <TouchableOpacity
      style={styles.diveItem}
      onPress={() => handleDivePress(item)}
      onLongPress={() => handleDeleteDive(item.id)}
    >
      <View style={styles.diveItemHeader}>
        <Text style={styles.diveDate}>
          {new Date(item.date).toLocaleDateString()} {new Date(item.date).toLocaleTimeString()}
        </Text>
        <Text style={styles.diveDuration}>{formatDuration(item.duration)}</Text>
      </View>
      <View style={styles.diveItemStats}>
        <Text style={styles.diveStat}>
          Max: {formatDepth(item.maxDepth, units)}
        </Text>
        <Text style={styles.diveStat}>
          Avg: {formatSpeed(item.avgSpeed, units)}
        </Text>
        <Text style={styles.diveStat}>{item.recordCount} samples</Text>
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Dive History</Text>
        <Text style={styles.headerCount}>{savedDives.length} dives</Text>
      </View>

      {connected && (
        <TouchableOpacity
          style={styles.exportButton}
          onPress={handleExportCurrentDive}
        >
          <Text style={styles.exportButtonText}>Save Current Session</Text>
        </TouchableOpacity>
      )}

      {savedDives.length === 0 ? (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>No saved dives yet</Text>
          <Text style={styles.emptySubtext}>
            Connect to your TideBand and dive to start recording.
          </Text>
        </View>
      ) : (
        <FlatList
          data={savedDives}
          renderItem={renderDiveItem}
          keyExtractor={(item) => item.id.toString()}
          contentContainerStyle={styles.list}
        />
      )}

      {/* Dive replay modal */}
      <Modal
        visible={modalVisible}
        animationType="slide"
        presentationStyle="pageSheet"
      >
        <View style={styles.modalContainer}>
          <View style={styles.modalHeader}>
            <TouchableOpacity onPress={() => setModalVisible(false)}>
              <Text style={styles.closeButton}>Close</Text>
            </TouchableOpacity>
            <Text style={styles.modalTitle}>Dive Replay</Text>
            <TouchableOpacity
              onPress={() => selectedDive && handleExportCSV(selectedDive)}
            >
              <Text style={styles.exportLink}>Export CSV</Text>
            </TouchableOpacity>
          </View>

          {selectedDive && (
            <ScrollView style={styles.modalContent}>
              <Text style={styles.modalDate}>
                {new Date(selectedDive.date).toLocaleString()}
              </Text>
              <View style={styles.modalStats}>
                <View style={styles.modalStatBox}>
                  <Text style={styles.modalStatLabel}>Duration</Text>
                  <Text style={styles.modalStatValue}>
                    {formatDuration(selectedDive.duration)}
                  </Text>
                </View>
                <View style={styles.modalStatBox}>
                  <Text style={styles.modalStatLabel}>Max Depth</Text>
                  <Text style={styles.modalStatValue}>
                    {formatDepth(selectedDive.maxDepth, units)}
                  </Text>
                </View>
                <View style={styles.modalStatBox}>
                  <Text style={styles.modalStatLabel}>Avg Current</Text>
                  <Text style={styles.modalStatValue}>
                    {formatSpeed(selectedDive.avgSpeed, units)}
                  </Text>
                </View>
              </View>

              <Text style={styles.chartTitle}>Depth & Current Profile</Text>
              <DiveProfileChart data={replayData} units={units} />

              <Text style={styles.chartTitle}>Current Speed Over Time</Text>
              <CurrentSpeedChart data={replayData} units={units} />
            </ScrollView>
          )}
        </View>
      </Modal>
    </View>
  );
}

// ---- Simple current speed chart (inline) ----
import Svg, { Polyline, Line, Text as SvgText } from 'react-native-svg';
import { Dimensions } from 'react-native';

function CurrentSpeedChart({ data, units }) {
  const screenWidth = Dimensions.get('window').width - 32;
  const chartHeight = 100;

  if (data.length === 0) return null;

  const speeds = data.map(p => Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2));
  const maxSpeed = Math.max(...speeds, 0.5);
  const stepX = screenWidth / Math.max(data.length - 1, 1);

  const points = speeds.map((s, i) =>
    `${i * stepX},${chartHeight - (s / maxSpeed) * chartHeight}`
  ).join(' ');

  return (
    <View style={styles.chartContainer}>
      <Svg width={screenWidth} height={chartHeight}>
        <Polyline
          points={points}
          fill="none"
          stroke="#0080FF"
          strokeWidth={2}
        />
        <Line x1={0} y1={chartHeight} x2={screenWidth} y2={chartHeight}
          stroke="#808080" strokeWidth={1} />
        <SvgText x={5} y={12} fontSize={10} fill="#808080">
          Max: {maxSpeed.toFixed(2)} m/s
        </SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F0F0F0',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 12,
    backgroundColor: '#FFFFFF',
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#333333',
  },
  headerCount: {
    fontSize: 14,
    color: '#808080',
  },
  exportButton: {
    backgroundColor: '#0080FF',
    paddingVertical: 10,
    paddingHorizontal: 20,
    borderRadius: 8,
    margin: 16,
    alignItems: 'center',
  },
  exportButtonText: {
    color: '#FFFFFF',
    fontSize: 16,
    fontWeight: 'bold',
  },
  emptyState: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    padding: 40,
  },
  emptyText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#808080',
    marginBottom: 8,
  },
  emptySubtext: {
    fontSize: 14,
    color: '#AAAAAA',
    textAlign: 'center',
  },
  list: {
    padding: 8,
  },
  diveItem: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 14,
    marginVertical: 4,
    marginHorizontal: 8,
  },
  diveItemHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  diveDate: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333333',
  },
  diveDuration: {
    fontSize: 14,
    color: '#0080FF',
    fontWeight: 'bold',
  },
  diveItemStats: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  diveStat: {
    fontSize: 12,
    color: '#808080',
  },
  modalContainer: {
    flex: 1,
    backgroundColor: '#F0F0F0',
  },
  modalHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 14,
    backgroundColor: '#0080FF',
  },
  closeButton: {
    color: '#FFFFFF',
    fontSize: 16,
  },
  modalTitle: {
    color: '#FFFFFF',
    fontSize: 18,
    fontWeight: 'bold',
  },
  exportLink: {
    color: '#FFFFFF',
    fontSize: 14,
  },
  modalContent: {
    flex: 1,
    padding: 16,
  },
  modalDate: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333333',
    marginBottom: 16,
  },
  modalStats: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 24,
  },
  modalStatBox: {
    flex: 1,
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 12,
    marginHorizontal: 4,
    alignItems: 'center',
  },
  modalStatLabel: {
    fontSize: 12,
    color: '#808080',
    marginBottom: 4,
  },
  modalStatValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#0080FF',
  },
  chartTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#333333',
    marginBottom: 8,
    marginTop: 16,
  },
  chartContainer: {
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 8,
    marginBottom: 16,
  },
});