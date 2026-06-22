/**
 * InsightsScreen.js — AI-generated insights from breath data
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useMemo } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  RefreshControl,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import {
  getKetosisLevel,
  getGutFermentationLevel,
  getVocCategory,
  formatDuration,
} from '../utils/protocol';

const InsightCard = ({ title, message, type, icon }) => {
  const colors = {
    info: '#2E86AB',
    warning: '#FF9800',
    alert: '#F44336',
    positive: '#4CAF50',
    neutral: '#9E9E9E',
  };
  const color = colors[type] || colors.neutral;

  return (
    <View style={[styles.insightCard, { borderLeftColor: color }]}>
      <Text style={styles.insightIcon}>{icon}</Text>
      <View style={styles.insightContent}>
        <Text style={[styles.insightTitle, { color }]}>{title}</Text>
        <Text style={styles.insightMessage}>{message}</Text>
      </View>
    </View>
  );
};

const InsightsScreen = () => {
  const { history, lastResult } = useBle();

  const insights = useMemo(() => {
    const result = [];
    if (history.length === 0 && !lastResult) {
      result.push({
        title: 'No Data Yet',
        message:
          'Take your first breath sample to start receiving personalized insights about your metabolic health.',
        type: 'neutral',
        icon: '💡',
      });
      return result;
    }

    const validSamples = history.filter((s) => s.isValid);
    if (validSamples.length === 0) {
      result.push({
        title: 'Start Analyzing',
        message:
          'Your breath samples will appear here as insights once you begin tracking.',
        type: 'info',
        icon: '🌬',
      });
      return result;
    }

    // --- Ketosis Insights ---
    const recentSamples = validSamples.slice(-10);
    const avgAcetone =
      recentSamples.reduce((sum, s) => sum + s.acetonePpm, 0) /
      recentSamples.length;
    const ketosis = getKetosisLevel(avgAcetone);

    if (ketosis.level === 'None') {
      result.push({
        title: 'Not in Ketosis',
        message:
          'Your breath acetone levels indicate you are not currently in ketosis. This is expected if you are not following a low-carb diet.',
        type: 'neutral',
        icon: '🥗',
      });
    } else if (ketosis.level === 'Light') {
      result.push({
        title: 'Light Ketosis Detected',
        message: `Your average breath acetone is ${avgAcetone.toFixed(
          2
        )} ppm, indicating light ketosis. Your body is beginning to burn fat for fuel.`,
        type: 'positive',
        icon: '🔥',
      });
    } else if (ketosis.level === 'Moderate') {
      result.push({
        title: 'Moderate Ketosis',
        message: `Average acetone: ${avgAcetone.toFixed(
          2
        )} ppm. You are in a solid fat-burning state. Great for ketogenic diet compliance.`,
        type: 'positive',
        icon: '🔥',
      });
    } else if (ketosis.level === 'Deep' || ketosis.level === 'Very Deep') {
      result.push({
        title: `${ketosis.level} Ketosis`,
        message: `Average acetone: ${avgAcetone.toFixed(
          2
        )} ppm. You are in deep ketosis. Monitor hydration and electrolytes.`,
        type: 'info',
        icon: '🔥',
      });
    }

    // --- Gut Fermentation Insights ---
    const avgH2 =
      recentSamples.reduce((sum, s) => sum + s.h2Ppm, 0) /
      recentSamples.length;
    const avgCH4 =
      recentSamples.reduce((sum, s) => sum + s.ch4Ppm, 0) /
      recentSamples.length;
    const fermentation = getGutFermentationLevel(avgH2, avgCH4);

    if (fermentation.level === 'Normal') {
      result.push({
        title: 'Gut Fermentation: Normal',
        message: `Your H₂ (${avgH2.toFixed(
          1
        )} ppm) and CH₄ (${avgCH4.toFixed(
          1
        )} ppm) levels are within normal range. Your gut microbiome appears healthy.`,
        type: 'positive',
        icon: '✅',
      });
    } else if (fermentation.level === 'Mild') {
      result.push({
        title: 'Mild Gut Fermentation',
        message: `Slightly elevated H₂ (${avgH2.toFixed(
          1
        )} ppm). This may indicate mild carbohydrate fermentation. Consider tracking which foods correlate with higher H₂.`,
        type: 'info',
        icon: '🦠',
      });
    } else if (fermentation.level === 'Moderate' || fermentation.level === 'High') {
      result.push({
        title: `${fermentation.level} Gut Fermentation`,
        message: `Elevated H₂ (${avgH2.toFixed(
          1
        )} ppm) and/or CH₄ (${avgCH4.toFixed(
          1
        )} ppm). This pattern may indicate carbohydrate malabsorption, SIBO, or food intolerance. Consider consulting a healthcare provider if symptoms persist.`,
        type: 'warning',
        icon: '⚠️',
      });
    }

    // --- Trend Insights ---
    if (validSamples.length >= 5) {
      const oldest = validSamples.slice(-5, -3);
      const newest = validSamples.slice(-2);
      const oldAvgH2 =
        oldest.reduce((sum, s) => sum + s.h2Ppm, 0) / oldest.length;
      const newAvgH2 =
        newest.reduce((sum, s) => sum + s.h2Ppm, 0) / newest.length;

      if (newAvgH2 > oldAvgH2 * 1.5) {
        result.push({
          title: 'H₂ Levels Rising',
          message:
            'Your breath hydrogen has increased recently. This may indicate changes in gut bacteria activity. Review recent dietary changes.',
          type: 'warning',
          icon: '📈',
        });
      } else if (newAvgH2 < oldAvgH2 * 0.5) {
        result.push({
          title: 'H₂ Levels Improving',
          message:
            'Your breath hydrogen has decreased. This may indicate improved gut health or successful dietary adjustments.',
          type: 'positive',
          icon: '📉',
        });
      }

      // Acetone trend
      const oldAvgAcetone =
        oldest.reduce((sum, s) => sum + s.acetonePpm, 0) / oldest.length;
      const newAvgAcetone =
        newest.reduce((sum, s) => sum + s.acetonePpm, 0) / newest.length;

      if (newAvgAcetone > oldAvgAcetone * 1.3) {
        result.push({
          title: 'Ketosis Increasing',
          message:
            'Your breath acetone is trending upward, suggesting increased fat metabolism. This may be from fasting, exercise, or dietary changes.',
          type: 'info',
          icon: '⬆️',
        });
      }
    }

    // --- VOC / Environmental Insights ---
    if (lastResult) {
      const voc = getVocCategory(lastResult.vocIndex);
      if (voc.category === 'Poor' || voc.category === 'Bad') {
        result.push({
          title: 'High VOC Levels',
          message:
            'Your breath VOC index is elevated. This could be from environmental exposure, recent meals, or oral hygiene products. Consider testing in a clean environment.',
          type: 'warning',
          icon: '🌫',
        });
      }
    }

    // --- CO2 Quality Insight ---
    if (lastResult && lastResult.co2Ppm < 35000 && lastResult.co2Ppm > 0) {
      result.push({
        title: 'Breath Sample Quality',
        message:
          'Your last sample had low CO₂, suggesting it may not have been a full deep breath. Try to exhale completely into the mouthpiece for best results.',
        type: 'info',
        icon: '💨',
      });
    }

    // --- Consistency Insight ---
    if (validSamples.length >= 3) {
      const timestamps = validSamples.slice(-7).map((s) => s.timestamp);
      const span = (timestamps[timestamps.length - 1] - timestamps[0]) / 86400;
      if (span < 1 && validSamples.length >= 5) {
        result.push({
          title: 'Great Tracking Consistency!',
          message:
            'You have been taking regular breath samples. This helps build a more accurate picture of your metabolic patterns.',
          type: 'positive',
          icon: '🏆',
        });
      } else if (validSamples.length < 3) {
        result.push({
          title: 'Track More Often',
          message:
            'Take breath samples at different times of day (morning, post-meal, post-exercise) for the most comprehensive metabolic picture.',
          type: 'info',
          icon: '📅',
        });
      }
    }

    return result;
  }, [history, lastResult]);

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl
          refreshing={false}
          onRefresh={() => {}}
          tintColor="#2E86AB"
        />
      }
    >
      <Text style={styles.pageTitle}>Insights</Text>

      <Text style={styles.pageDescription}>
        Personalized analysis of your breath data. These insights are
        generated on-device and in-app — no cloud processing required.
      </Text>

      {insights.map((insight, idx) => (
        <InsightCard
          key={idx}
          title={insight.title}
          message={insight.message}
          type={insight.type}
          icon={insight.icon}
        />
      ))}

      {/* Summary stats */}
      {history.length > 0 && (
        <View style={styles.summaryCard}>
          <Text style={styles.summaryTitle}>Your Summary</Text>
          <View style={styles.summaryRow}>
            <Text style={styles.summaryLabel}>Total samples:</Text>
            <Text style={styles.summaryValue}>{history.length}</Text>
          </View>
          <View style={styles.summaryRow}>
            <Text style={styles.summaryLabel}>Valid samples:</Text>
            <Text style={styles.summaryValue}>
              {history.filter((s) => s.isValid).length}
            </Text>
          </View>
          <View style={styles.summaryRow}>
            <Text style={styles.summaryLabel}>Days tracked:</Text>
            <Text style={styles.summaryValue}>
              {Math.ceil(
                (Date.now() / 1000 - history[0].timestamp) / 86400
              )}
            </Text>
          </View>
        </View>
      )}

      <Text style={styles.disclaimer}>
        BreathPrint is a wellness device, not a medical device. Insights are
        for informational purposes only and should not replace professional
        medical advice.
      </Text>

      <Text style={styles.footer}>Author: jayis1 — MIT License (c) 2026</Text>
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
    paddingBottom: 8,
  },
  pageDescription: {
    color: '#888',
    fontSize: 14,
    paddingHorizontal: 20,
    paddingBottom: 16,
    lineHeight: 20,
  },
  insightCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 16,
    marginHorizontal: 16,
    marginBottom: 12,
    flexDirection: 'row',
    borderLeftWidth: 4,
  },
  insightIcon: {
    fontSize: 24,
    marginRight: 12,
    marginTop: 2,
  },
  insightContent: {
    flex: 1,
  },
  insightTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  insightMessage: {
    color: '#aaa',
    fontSize: 14,
    lineHeight: 20,
  },
  summaryCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
  },
  summaryTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 16,
  },
  summaryRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  summaryLabel: {
    color: '#aaa',
    fontSize: 14,
  },
  summaryValue: {
    color: '#fff',
    fontSize: 14,
    fontWeight: 'bold',
  },
  disclaimer: {
    color: '#555',
    fontSize: 12,
    paddingHorizontal: 20,
    marginTop: 16,
    lineHeight: 18,
    textAlign: 'center',
  },
  footer: {
    color: '#444',
    fontSize: 11,
    textAlign: 'center',
    padding: 20,
  },
});

export default InsightsScreen;