/**
 * TrendScreen.js — Historical Trend Charts
 *
 * Displays time-series charts of all sensor channels over the batch
 * lifetime. Supports selecting which channels to display and the
 * time window (1h, 6h, 24h, full batch).
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView, Dimensions } from 'react-native';
import { Card, Title, Paragraph, Button, SegmentedButtons, Checkbox } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { FermenTiqContext } from '../utils/ble';
import { LineChart } from 'react-native-chart-kit';

const SCREEN_WIDTH = Dimensions.get('window').width;

export default function TrendScreen() {
  const { connectionState } = useContext(FermenTiqContext);
  const [history, setHistory] = useState([]);
  const [timeWindow, setTimeWindow] = useState('24h');
  const [showTemp, setShowTemp] = useState(true);
  const [showPh, setShowPh] = useState(true);
  const [showCo2, setShowCo2] = useState(true);
  const [showAbv, setShowAbv] = useState(false);
  const [showBiomass, setShowBiomass] = useState(false);

  // Accumulate live data points into history
  useEffect(() => {
    if (connectionState.liveData) {
      setHistory(prev => {
        const point = {
          timestamp: Date.now(),
          temp: connectionState.liveData.temperature.liquidC,
          ph: connectionState.liveData.ph.value,
          co2: connectionState.liveData.co2.ppm,
          abv: connectionState.liveData.fusion.abv,
          cellDensity: connectionState.liveData.impedance.cellDensity / 1e6,
          bubbleRate: connectionState.liveData.acoustic.bubbleRate,
        };
        const updated = [...prev, point];
        // Keep last 500 points (~15 min at 2s intervals)
        return updated.slice(-500);
      });
    }
  }, [connectionState.liveData]);

  const windowMs = {
    '1h': 3600000,
    '6h': 21600000,
    '24h': 86400000,
    'all': Infinity,
  }[timeWindow];

  const now = Date.now();
  const visibleData = history.filter(p => now - p.timestamp <= windowMs);

  const labels = visibleData.length > 0
    ? visibleData.filter((_, i) => i % Math.max(1, Math.floor(visibleData.length / 6)) === 0)
        .map(p => {
          const d = new Date(p.timestamp);
          return `${d.getHours()}:${String(d.getMinutes()).padStart(2, '0')}`;
        })
    : [];

  const chartConfig = {
    backgroundColor: '#16213e',
    backgroundGradientFrom: '#16213e',
    backgroundGradientTo: '#16213e',
    color: (opacity = 1) => `rgba(212, 160, 23, ${opacity})`,
    labelColor: (opacity = 1) => `rgba(224, 224, 224, ${opacity})`,
    strokeWidth: 2,
    propsForDots: { r: 2 },
  };

  const buildChartData = (field, color, unit) => {
    const data = visibleData.map(p => p[field]);
    return {
      labels: labels,
      datasets: [{
        data: data.length > 0 ? data : [0],
        color: (opacity = 1) => color,
        strokeWidth: 2,
      }],
    };
  };

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>Trend Analysis</Title>
          <SegmentedButtons
            value={timeWindow}
            onValueChange={setTimeWindow}
            buttons={[
              { value: '1h', label: '1H' },
              { value: '6h', label: '6H' },
              { value: '24h', label: '24H' },
              { value: 'all', label: 'All' },
            ]}
            style={styles.segmented}
          />

          {/* Channel selectors */}
          <View style={styles.channelRow}>
            <View style={styles.channelItem}>
              <Checkbox status={showTemp ? 'checked' : 'unchecked'} onPress={() => setShowTemp(!showTemp)} color="#42A5F5" />
              <Text style={styles.channelLabel}>Temperature</Text>
            </View>
            <View style={styles.channelItem}>
              <Checkbox status={showPh ? 'checked' : 'unchecked'} onPress={() => setShowPh(!showPh)} color="#9C27B0" />
              <Text style={styles.channelLabel}>pH</Text>
            </View>
            <View style={styles.channelItem}>
              <Checkbox status={showCo2 ? 'checked' : 'unchecked'} onPress={() => setShowCo2(!showCo2)} color="#FF7043" />
              <Text style={styles.channelLabel}>CO₂</Text>
            </View>
          </View>
          <View style={styles.channelRow}>
            <View style={styles.channelItem}>
              <Checkbox status={showAbv ? 'checked' : 'unchecked'} onPress={() => setShowAbv(!showAbv)} color="#D4A017" />
              <Text style={styles.channelLabel}>ABV</Text>
            </View>
            <View style={styles.channelItem}>
              <Checkbox status={showBiomass ? 'checked' : 'unchecked'} onPress={() => setShowBiomass(!showBiomass)} color="#66BB6A" />
              <Text style={styles.channelLabel}>Biomass</Text>
            </View>
          </View>
        </Card.Content>
      </Card>

      {visibleData.length === 0 ? (
        <View style={styles.emptyContainer}>
          <Icon name="chart-line-variant" size={48} color="#555" />
          <Text style={styles.emptyText}>No data yet for selected period</Text>
        </View>
      ) : (
        <>
          {showTemp && (
            <Card style={styles.card}>
              <Card.Content>
                <Title style={styles.chartTitle}>
                  <Icon name="thermometer" size={20} color="#42A5F5" /> Temperature (°C)
                </Title>
                <LineChart
                  data={buildChartData('temp', 'rgba(66, 165, 245, 1)', '°C')}
                  width={SCREEN_WIDTH - 50}
                  height={180}
                  chartConfig={chartConfig}
                  bezier
                  style={styles.chart}
                />
              </Card.Content>
            </Card>
          )}

          {showPh && (
            <Card style={styles.card}>
              <Card.Content>
                <Title style={styles.chartTitle}>
                  <Icon name="water" size={20} color="#9C27B0" /> pH
                </Title>
                <LineChart
                  data={buildChartData('ph', 'rgba(156, 39, 176, 1)', '')}
                  width={SCREEN_WIDTH - 50}
                  height={180}
                  chartConfig={chartConfig}
                  bezier
                  style={styles.chart}
                />
              </Card.Content>
            </Card>
          )}

          {showCo2 && (
            <Card style={styles.card}>
              <Card.Content>
                <Title style={styles.chartTitle}>
                  <Icon name="molecule-co2" size={20} color="#FF7043" /> CO₂ (ppm)
                </Title>
                <LineChart
                  data={buildChartData('co2', 'rgba(255, 112, 67, 1)', 'ppm')}
                  width={SCREEN_WIDTH - 50}
                  height={180}
                  chartConfig={chartConfig}
                  bezier
                  style={styles.chart}
                />
              </Card.Content>
            </Card>
          )}

          {showAbv && (
            <Card style={styles.card}>
              <Card.Content>
                <Title style={styles.chartTitle}>
                  <Icon name="glass-wine" size={20} color="#D4A017" /> ABV Estimate (%)
                </Title>
                <LineChart
                  data={buildChartData('abv', 'rgba(212, 160, 23, 1)', '%')}
                  width={SCREEN_WIDTH - 50}
                  height={180}
                  chartConfig={chartConfig}
                  bezier
                  style={styles.chart}
                />
              </Card.Content>
            </Card>
          )}

          {showBiomass && (
            <Card style={styles.card}>
              <Card.Content>
                <Title style={styles.chartTitle}>
                  <Icon name="bacteria" size={20} color="#66BB6A" /> Cell Density (M/mL)
                </Title>
                <LineChart
                  data={buildChartData('cellDensity', 'rgba(102, 187, 106, 1)', 'M/mL')}
                  width={SCREEN_WIDTH - 50}
                  height={180}
                  chartConfig={chartConfig}
                  bezier
                  style={styles.chart}
                />
              </Card.Content>
            </Card>
          )}
        </>
      )}

      <View style={styles.footer}>
        <Text style={styles.footerText}>FermenTiq • Author: jayis1</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e' },
  card: { margin: 12, backgroundColor: '#16213e' },
  title: { color: '#e0e0e0', fontSize: 20, marginBottom: 12 },
  chartTitle: { color: '#e0e0e0', fontSize: 16, marginBottom: 8, flexDirection: 'row', alignItems: 'center' },
  segmented: { marginBottom: 12 },
  channelRow: { flexDirection: 'row', justifyContent: 'space-around', marginVertical: 6 },
  channelItem: { flexDirection: 'row', alignItems: 'center' },
  channelLabel: { color: '#e0e0e0', fontSize: 13, marginLeft: 4 },
  chart: { marginVertical: 8, borderRadius: 8 },
  emptyContainer: { alignItems: 'center', padding: 40 },
  emptyText: { color: '#666', fontSize: 16, marginTop: 12 },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 12 },
});