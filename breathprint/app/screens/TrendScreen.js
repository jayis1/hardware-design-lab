/**
 * TrendScreen.js — Historical breath marker trends
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useMemo } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Dimensions,
} from 'react-native';
import { useBle } from '../utils/BleContext';
import { formatTimestamp } from '../utils/protocol';

const { width } = Dimensions.get('window');

const MARKERS = [
  { key: 'acetonePpm', label: 'Acetone', unit: 'ppm', color: '#FF9800' },
  { key: 'h2Ppm', label: 'Hydrogen (H₂)', unit: 'ppm', color: '#2196F3' },
  { key: 'ch4Ppm', label: 'Methane (CH₄)', unit: 'ppm', color: '#4CAF50' },
  { key: 'isoprenePpb', label: 'Isoprene', unit: 'ppb', color: '#3F51B5' },
  { key: 'vocIndex', label: 'VOC Index', unit: '', color: '#9C27B0' },
  { key: 'co2Ppm', label: 'CO₂', unit: 'ppm', color: '#FF5722' },
];

const TIME_RANGES = [
  { key: '24h', label: '24 Hours', ms: 86400000 },
  { key: '7d', label: '7 Days', ms: 604800000 },
  { key: '30d', label: '30 Days', ms: 2592000000 },
  { key: 'all', label: 'All', ms: Infinity },
];

const TrendScreen = () => {
  const { history } = useBle();
  const [selectedMarker, setSelectedMarker] = useState(0);
  const [timeRange, setTimeRange] = useState(1); // 7 days default

  // Filter history by time range
  const filteredHistory = useMemo(() => {
    if (history.length === 0) return [];
    const range = TIME_RANGES[timeRange];
    const cutoff = Date.now() - range.ms;
    return history.filter(
      (item) => item.timestamp * 1000 >= cutoff && item.isValid
    );
  }, [history, timeRange]);

  // Extract data for selected marker
  const markerData = useMemo(() => {
    const marker = MARKERS[selectedMarker];
    return filteredHistory.map((item) => ({
      x: item.timestamp * 1000,
      y: item[marker.key],
      state: item.stateName,
      stateColor: item.stateColor,
    }));
  }, [filteredHistory, selectedMarker]);

  // Compute statistics
  const stats = useMemo(() => {
    if (markerData.length === 0) return null;
    const values = markerData.map((d) => d.y);
    const min = Math.min(...values);
    const max = Math.max(...values);
    const avg = values.reduce((a, b) => a + b, 0) / values.length;
    const latest = values[values.length - 1];
    return { min, max, avg, latest, count: values.length };
  }, [markerData]);

  // Simple chart rendering (custom, no external lib needed)
  const renderChart = () => {
    if (markerData.length < 2) {
      return (
        <View style={styles.noDataChart}>
          <Text style={styles.noDataText}>Not enough data for chart</Text>
          <Text style={styles.noDataSubtext}>
            Take more breath samples to see trends
          </Text>
        </View>
      );
    }

    const marker = MARKERS[selectedMarker];
    const chartWidth = width - 64;
    const chartHeight = 180;
    const padding = 20;

    // Calculate scales
    const xMin = markerData[0].x;
    const xMax = markerData[markerData.length - 1].x;
    const xRange = xMax - xMin || 1;
    const yMin = Math.min(...markerData.map((d) => d.y));
    const yMax = Math.max(...markerData.map((d) => d.y));
    const yRange = yMax - yMin || 1;

    // Generate SVG-like points (using View positions)
    const points = markerData.map((d) => {
      const px = padding + ((d.x - xMin) / xRange) * (chartWidth - 2 * padding);
      const py =
        chartHeight -
        padding -
        ((d.y - yMin) / yRange) * (chartHeight - 2 * padding);
      return { px, py, value: d.y, state: d.state, stateColor: d.stateColor };
    });

    return (
      <View style={styles.chartContainer}>
        {/* Y-axis labels */}
        <View style={styles.yAxis}>
          <Text style={styles.axisLabel}>{yMax.toFixed(1)}</Text>
          <Text style={styles.axisLabel}>
            {((yMax + yMin) / 2).toFixed(1)}
          </Text>
          <Text style={styles.axisLabel}>{yMin.toFixed(1)}</Text>
        </View>

        {/* Chart area */}
        <View
          style={[styles.chartArea, { width: chartWidth, height: chartHeight }]}
        >
          {/* Grid lines */}
          <View style={styles.gridLine} />
          <View style={[styles.gridLine, { top: chartHeight / 2 }]} />
          <View style={[styles.gridLine, { top: chartHeight - padding }]} />

          {/* Data points */}
          {points.map((p, i) => (
            <View
              key={i}
              style={[
                styles.dataPoint,
                {
                  left: p.px - 3,
                  top: p.py - 3,
                  backgroundColor: marker.color,
                },
              ]}
            />
          ))}

          {/* Connecting line segments (simplified) */}
          {points.slice(0, -1).map((p, i) => {
            const next = points[i + 1];
            const dx = next.px - p.px;
            const dy = next.py - p.py;
            const length = Math.sqrt(dx * dx + dy * dy);
            const angle = (Math.atan2(dy, dx) * 180) / Math.PI;
            return (
              <View
                key={`line-${i}`}
                style={[
                  styles.dataLine,
                  {
                    left: p.px,
                    top: p.py,
                    width: length,
                    transform: [{ rotate: `${angle}deg` }],
                    backgroundColor: marker.color + '60',
                  },
                ]}
              />
            );
          })}
        </View>

        {/* X-axis labels */}
        <View style={styles.xAxis}>
          <Text style={styles.axisLabel}>
            {new Date(xMin).toLocaleDateString()}
          </Text>
          <Text style={styles.axisLabel}>
            {new Date(xMax).toLocaleDateString()}
          </Text>
        </View>
      </View>
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.pageTitle}>Breath Marker Trends</Text>

      {/* Time range selector */}
      <View style={styles.rangeSelector}>
        {TIME_RANGES.map((range, idx) => (
          <TouchableOpacity
            key={range.key}
            style={[
              styles.rangeButton,
              timeRange === idx && styles.rangeButtonActive,
            ]}
            onPress={() => setTimeRange(idx)}
          >
            <Text
              style={[
                styles.rangeButtonText,
                timeRange === idx && styles.rangeButtonTextActive,
              ]}
            >
              {range.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Marker selector */}
      <View style={styles.markerSelector}>
        {MARKERS.map((marker, idx) => (
          <TouchableOpacity
            key={marker.key}
            style={[
              styles.markerButton,
              selectedMarker === idx && {
                backgroundColor: marker.color + '30',
                borderColor: marker.color,
              },
            ]}
            onPress={() => setSelectedMarker(idx)}
          >
            <View
              style={[styles.markerDot, { backgroundColor: marker.color }]}
            />
            <Text
              style={[
                styles.markerText,
                selectedMarker === idx && { color: marker.color },
              ]}
            >
              {marker.label}
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* Chart */}
      <View style={styles.chartCard}>
        <Text style={styles.chartTitle}>
          {MARKERS[selectedMarker].label} ({MARKERS[selectedMarker].unit})
        </Text>
        {renderChart()}
      </View>

      {/* Statistics */}
      {stats && (
        <View style={styles.statsCard}>
          <Text style={styles.statsTitle}>Statistics</Text>
          <View style={styles.statsGrid}>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Latest</Text>
              <Text style={styles.statValue}>
                {stats.latest.toFixed(2)}
              </Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Average</Text>
              <Text style={styles.statValue}>{stats.avg.toFixed(2)}</Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Min</Text>
              <Text style={styles.statValue}>{stats.min.toFixed(2)}</Text>
            </View>
            <View style={styles.statItem}>
              <Text style={styles.statLabel}>Max</Text>
              <Text style={styles.statValue}>{stats.max.toFixed(2)}</Text>
            </View>
          </View>
          <Text style={styles.sampleCount}>
            {stats.count} valid samples in range
          </Text>
        </View>
      )}

      {/* Recent samples list */}
      <View style={styles.listCard}>
        <Text style={styles.listTitle}>Sample History</Text>
        {filteredHistory.length === 0 ? (
          <Text style={styles.emptyText}>No samples yet</Text>
        ) : (
          filteredHistory
            .slice(-10)
            .reverse()
            .map((item, idx) => (
              <View key={idx} style={styles.listItem}>
                <View
                  style={[
                    styles.listDot,
                    { backgroundColor: item.stateColor || '#9E9E9E' },
                  ]}
                />
                <View style={styles.listContent}>
                  <Text style={styles.listState}>{item.stateName}</Text>
                  <Text style={styles.listTime}>
                    {formatTimestamp(item.timestamp)}
                  </Text>
                </View>
                <View style={styles.listMetrics}>
                  <Text style={styles.listMetric}>
                    Acet: {item.acetonePpm.toFixed(1)}
                  </Text>
                  <Text style={styles.listMetric}>
                    H₂: {item.h2Ppm.toFixed(1)}
                  </Text>
                </View>
              </View>
            ))
        )}
      </View>
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
  rangeSelector: {
    flexDirection: 'row',
    paddingHorizontal: 16,
    marginBottom: 12,
  },
  rangeButton: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
    backgroundColor: '#1a1a2e',
    marginHorizontal: 2,
    borderRadius: 8,
  },
  rangeButtonActive: {
    backgroundColor: '#2E86AB',
  },
  rangeButtonText: {
    color: '#888',
    fontSize: 13,
  },
  rangeButtonTextActive: {
    color: '#fff',
    fontWeight: 'bold',
  },
  markerSelector: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    paddingHorizontal: 16,
    marginBottom: 16,
  },
  markerButton: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 12,
    paddingVertical: 8,
    backgroundColor: '#1a1a2e',
    margin: 4,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: '#333',
  },
  markerDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    marginRight: 6,
  },
  markerText: {
    color: '#888',
    fontSize: 13,
  },
  chartCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 16,
    margin: 16,
  },
  chartTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 16,
  },
  chartContainer: {
    flexDirection: 'row',
  },
  yAxis: {
    width: 40,
    height: 180,
    justifyContent: 'space-between',
    paddingVertical: 20,
  },
  axisLabel: {
    color: '#666',
    fontSize: 10,
  },
  chartArea: {
    position: 'relative',
    backgroundColor: '#0f0f1e',
    borderRadius: 8,
  },
  gridLine: {
    position: 'absolute',
    left: 0,
    right: 0,
    height: 1,
    backgroundColor: '#222',
    top: 20,
  },
  dataPoint: {
    position: 'absolute',
    width: 6,
    height: 6,
    borderRadius: 3,
  },
  dataLine: {
    position: 'absolute',
    height: 2,
    transformOrigin: '0 0',
  },
  xAxis: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 20,
    marginTop: 8,
    width: width - 64 - 40,
  },
  noDataChart: {
    height: 180,
    justifyContent: 'center',
    alignItems: 'center',
  },
  noDataText: {
    color: '#666',
    fontSize: 16,
  },
  noDataSubtext: {
    color: '#444',
    fontSize: 13,
    marginTop: 8,
  },
  statsCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
  },
  statsTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 16,
  },
  statsGrid: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  statItem: {
    alignItems: 'center',
  },
  statLabel: {
    color: '#888',
    fontSize: 12,
    marginBottom: 4,
  },
  statValue: {
    color: '#fff',
    fontSize: 18,
    fontWeight: 'bold',
  },
  sampleCount: {
    color: '#666',
    fontSize: 12,
    marginTop: 16,
    textAlign: 'center',
  },
  listCard: {
    backgroundColor: '#1a1a2e',
    borderRadius: 16,
    padding: 20,
    margin: 16,
    marginBottom: 40,
  },
  listTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    marginBottom: 12,
  },
  emptyText: {
    color: '#666',
    textAlign: 'center',
    paddingVertical: 20,
  },
  listItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#333',
  },
  listDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 12,
  },
  listContent: {
    flex: 1,
  },
  listState: {
    color: '#fff',
    fontSize: 14,
  },
  listTime: {
    color: '#666',
    fontSize: 12,
  },
  listMetrics: {
    alignItems: 'flex-end',
  },
  listMetric: {
    color: '#aaa',
    fontSize: 12,
  },
});

export default TrendScreen;