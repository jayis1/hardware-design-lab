/**
 * DietScreen.js — Food log with breath marker correlation
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  TextInput,
  Modal,
  FlatList,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { formatTimestamp, formatDuration } from '../utils/protocol';

const FOOD_CATEGORIES = [
  { key: 'dairy', label: 'Dairy', icon: '🥛', color: '#FFD700' },
  { key: 'grains', label: 'Grains', icon: '🍞', color: '#D2691E' },
  { key: 'protein', label: 'Protein', icon: '🥩', color: '#DC143C' },
  { key: 'vegetables', label: 'Vegetables', icon: '🥦', color: '#228B22' },
  { key: 'fruits', label: 'Fruits', icon: '🍎', color: '#FF6347' },
  { key: 'sweets', label: 'Sweets', icon: '🍰', color: '#FF69B4' },
  { key: 'fats', label: 'Fats/Oils', icon: '🫒', color: '#808000' },
  { key: 'beverages', label: 'Beverages', icon: '☕', color: '#8B4513' },
  { key: 'fermented', label: 'Fermented', icon: '🧀', color: '#FFA500' },
  { key: 'fodmap', label: 'High FODMAP', icon: '🧄', color: '#9370DB' },
];

const DietScreen = () => {
  const { history } = useBle();
  const [foodLog, setFoodLog] = useState([]);
  const [showAddModal, setShowAddModal] = useState(false);
  const [selectedCategory, setSelectedCategory] = useState(null);
  const [foodName, setFoodName] = useState('');

  useEffect(() => {
    loadFoodLog();
  }, []);

  const loadFoodLog = async () => {
    try {
      const stored = await AsyncStorage.getItem('breathprint_foodlog');
      if (stored) setFoodLog(JSON.parse(stored));
    } catch (e) {
      console.error('Failed to load food log:', e);
    }
  };

  const saveFoodLog = async (newLog) => {
    try {
      await AsyncStorage.setItem('breathprint_foodlog', JSON.stringify(newLog));
    } catch (e) {
      console.error('Failed to save food log:', e);
    }
  };

  const addFoodEntry = () => {
    if (!foodName.trim() || !selectedCategory) return;

    const entry = {
      id: Date.now(),
      name: foodName.trim(),
      category: selectedCategory,
      timestamp: Math.floor(Date.now() / 1000),
    };

    const newLog = [...foodLog, entry];
    setFoodLog(newLog);
    saveFoodLog(newLog);
    setFoodName('');
    setSelectedCategory(null);
    setShowAddModal(false);
  };

  const deleteEntry = (id) => {
    const newLog = foodLog.filter((e) => e.id !== id);
    setFoodLog(newLog);
    saveFoodLog(newLog);
  };

  // Find breath samples taken within 3 hours of a food entry
  const getCorrelatedSamples = (foodEntry) => {
    const foodTime = foodEntry.timestamp;
    const windowMs = 3 * 3600 * 1000; // 3 hours

    return history.filter((sample) => {
      const sampleTime = sample.timestamp * 1000;
      const diff = sampleTime - foodTime * 1000;
      return diff > 0 && diff < windowMs;
    });
  };

  // Get category breakdown
  const categoryStats = FOOD_CATEGORIES.map((cat) => {
    const entries = foodLog.filter((e) => e.category === cat.key);
    return { ...cat, count: entries.length };
  }).filter((c) => c.count > 0);

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.pageTitle}>Diet & Correlation</Text>

      {/* Add food button */}
      <TouchableOpacity
        style={styles.addButton}
        onPress={() => setShowAddModal(true)}
      >
        <Text style={styles.addButtonText}>+ Log Food Entry</Text>
      </TouchableOpacity>

      {/* Category breakdown */}
      {categoryStats.length > 0 && (
        <View style={styles.card}>
          <Text style={styles.cardTitle}>Diet Breakdown</Text>
          <View style={styles.categoryGrid}>
            {categoryStats.map((cat) => (
              <View key={cat.key} style={styles.categoryItem}>
                <Text style={styles.categoryIcon}>{cat.icon}</Text>
                <Text style={styles.categoryLabel}>{cat.label}</Text>
                <Text style={styles.categoryCount}>{cat.count}×</Text>
              </View>
            ))}
          </View>
        </View>
      )}

      {/* Food entries with correlated breath data */}
      <View style={styles.card}>
        <Text style={styles.cardTitle}>Food & Breath Correlations</Text>
        {foodLog.length === 0 ? (
          <Text style={styles.emptyText}>
            No food entries yet. Log what you eat to see how it affects
            your breath markers.
          </Text>
        ) : (
          foodLog
            .slice(-15)
            .reverse()
            .map((entry) => {
              const cat = FOOD_CATEGORIES.find(
                (c) => c.key === entry.category
              );
              const correlated = getCorrelatedSamples(entry);

              return (
                <View key={entry.id} style={styles.foodEntry}>
                  <View style={styles.foodHeader}>
                    <Text style={styles.foodIcon}>{cat?.icon || '🍽'}</Text>
                    <View style={styles.foodInfo}>
                      <Text style={styles.foodName}>{entry.name}</Text>
                      <Text style={styles.foodCategory}>{cat?.label}</Text>
                      <Text style={styles.foodTime}>
                        {formatTimestamp(entry.timestamp)}
                      </Text>
                    </View>
                    <TouchableOpacity
                      onPress={() => deleteEntry(entry.id)}
                    >
                      <Text style={styles.deleteBtn}>✕</Text>
                    </TouchableOpacity>
                  </View>

                  {/* Correlated breath samples */}
                  {correlated.length > 0 && (
                    <View style={styles.correlatedSection}>
                      <Text style={styles.correlatedTitle}>
                        Breath samples (within 3h):
                      </Text>
                      {correlated.map((sample, idx) => (
                        <View key={idx} style={styles.correlatedItem}>
                          <View
                            style={[
                              styles.stateDot,
                              { backgroundColor: sample.stateColor },
                            ]}
                          />
                          <Text style={styles.correlatedState}>
                            {sample.stateName}
                          </Text>
                          <Text style={styles.correlatedMetrics}>
                            H₂: {sample.h2Ppm.toFixed(1)} | CH₄:{' '}
                            {sample.ch4Ppm.toFixed(1)} | Acet:{' '}
                            {sample.acetonePpm.toFixed(1)}
                          </Text>
                        </View>
                      ))}
                    </View>
                  )}
                  {correlated.length === 0 && (
                    <Text style={styles.noCorrelation}>
                      No breath samples within 3 hours
                    </Text>
                  )}
                </View>
              );
            })
        )}
      </View>

      {/* Add food modal */}
      <Modal
        visible={showAddModal}
        animationType="slide"
        transparent={true}
        onRequestClose={() => setShowAddModal(false)}
      >
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>Log Food Entry</Text>

            <Text style={styles.modalLabel}>Category:</Text>
            <View style={styles.categorySelector}>
              {FOOD_CATEGORIES.map((cat) => (
                <TouchableOpacity
                  key={cat.key}
                  style={[
                    styles.categorySelectorItem,
                    selectedCategory === cat.key && {
                      backgroundColor: cat.color + '30',
                      borderColor: cat.color,
                    },
                  ]}
                  onPress={() => setSelectedCategory(cat.key)}
                >
                  <Text style={styles.categorySelectorIcon}>{cat.icon}</Text>
                  <Text style={styles.categorySelectorLabel}>{cat.label}</Text>
                </TouchableOpacity>
              ))}
            </View>

            <Text style={styles.modalLabel}>Food name:</Text>
            <TextInput
              style={styles.textInput}
              placeholder="e.g., Greek yogurt, sourdough bread..."
              placeholderTextColor="#666"
              value={foodName}
              onChangeText={setFoodName}
            />

            <View style={styles.modalButtons}>
              <TouchableOpacity
                style={[styles.modalButton, styles.modalButtonCancel]}
                onPress={() => {
                  setShowAddModal(false);
                  setFoodName('');
                  setSelectedCategory(null);
                }}
              >
                <Text style={styles.modalButtonText}>Cancel</Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.modalButton, styles.modalButtonAdd]}
                onPress={addFoodEntry}
                disabled={!foodName.trim() || !selectedCategory}
              >
                <Text style={styles.modalButtonText}>Add Entry</Text>
              </TouchableOpacity>
            </View>
          </View>
        </View>
      </Modal>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1e',
  },
  pageTitle: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
    padding: 20,
  },
  addButton: {
    backgroundColor: '#2E86AB',
    borderRadius: 12,
    paddingVertical: 14,
    marginHorizontal: 16,
    marginBottom: 16,
    alignItems: 'center',
  },
  addButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  card: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
  },
  cardTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 16,
  },
  categoryGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  categoryItem: {
    alignItems: 'center',
    width: 80,
    marginVertical: 8,
  },
  categoryIcon: {
    fontSize: 24,
  },
  categoryLabel: {
    color: '#aaa',
    fontSize: 11,
    marginTop: 4,
    textAlign: 'center',
  },
  categoryCount: {
    color: '#2E86AB',
    fontSize: 14,
    fontWeight: 'bold',
    marginTop: 2,
  },
  emptyText: {
    color: '#666',
    fontSize: 14,
    lineHeight: 20,
  },
  foodEntry: {
    backgroundColor: '#0f0f1e',
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
  },
  foodHeader: {
    flexDirection: 'row',
    alignItems: 'flex-start',
  },
  foodIcon: {
    fontSize: 28,
    marginRight: 12,
  },
  foodInfo: {
    flex: 1,
  },
  foodName: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  foodCategory: {
    color: '#888',
    fontSize: 13,
    marginTop: 2,
  },
  foodTime: {
    color: '#555',
    fontSize: 12,
    marginTop: 4,
  },
  deleteBtn: {
    color: '#F44336',
    fontSize: 18,
    padding: 4,
  },
  correlatedSection: {
    marginTop: 12,
    paddingTop: 12,
    borderTopWidth: 1,
    borderTopColor: '#333',
  },
  correlatedTitle: {
    color: '#aaa',
    fontSize: 12,
    marginBottom: 8,
  },
  correlatedItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
  },
  stateDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 8,
  },
  correlatedState: {
    color: '#fff',
    fontSize: 13,
    marginRight: 12,
  },
  correlatedMetrics: {
    color: '#888',
    fontSize: 11,
    flex: 1,
  },
  noCorrelation: {
    color: '#555',
    fontSize: 12,
    marginTop: 8,
    fontStyle: 'italic',
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'center',
    padding: 20,
  },
  modalContent: {
    backgroundColor: '#1a1a2e',
    borderRadius: 20,
    padding: 24,
  },
  modalTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 20,
  },
  modalLabel: {
    color: '#aaa',
    fontSize: 14,
    marginBottom: 8,
  },
  categorySelector: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginBottom: 20,
  },
  categorySelectorItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
    paddingVertical: 8,
    backgroundColor: '#0f0f1e',
    margin: 4,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: '#333',
  },
  categorySelectorIcon: {
    fontSize: 16,
    marginRight: 6,
  },
  categorySelectorLabel: {
    color: '#ccc',
    fontSize: 13,
  },
  textInput: {
    backgroundColor: '#0f0f1e',
    color: '#fff',
    borderRadius: 12,
    paddingHorizontal: 16,
    paddingVertical: 12,
    fontSize: 16,
    marginBottom: 20,
  },
  modalButtons: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  modalButton: {
    flex: 1,
    paddingVertical: 14,
    borderRadius: 12,
    alignItems: 'center',
    marginHorizontal: 4,
  },
  modalButtonCancel: {
    backgroundColor: '#333',
  },
  modalButtonAdd: {
    backgroundColor: '#2E86AB',
  },
  modalButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
});

export default DietScreen;