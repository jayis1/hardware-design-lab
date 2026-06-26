// src/EventDetail.tsx — triaxial waveform viewer + cross-correlation epicenter
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useMemo } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { Card, Title, Paragraph, Divider, Button } from 'react-native-paper';
import { LineChart } from 'react-native-chart-kit';
import { useRoute } from '@react-navigation/native';

// Synthetic triaxial waveform: 200 SPS, ±5 s window = 2000 samples per axis.
// We downsample to 100 points for display.
function makeWaveform(seed: number, type: string): number[] {
  const pts: number[] = [];
  for (let i = 0; i < 100; i++) {
    const t = i / 100;
    let v = 0;
    if (type === 'Basal slide') {
      v = Math.sin(2 * Math.PI * 4 * t) * Math.exp(-((t - 0.5) ** 2) / 0.05) * 0.8;
    } else if (type === 'Crevasse snap') {
      v = (Math.random() - 0.5) * 2 * Math.exp(-((t - 0.3) ** 2) / 0.005);
    } else if (type === 'Calving thump') {
      v = Math.sin(2 * Math.PI * 2 * t) * (1 - t * 0.5) * 0.9;
    } else {
      v = Math.sin(2 * Math.PI * 6 * t + seed) * 0.5;
    }
    pts.push(Number(v.toFixed(4)));
  }
  return pts;
}

// Mock epicenter estimate via TDOA across array
function estimateEpicenter(eventId: string): { lat: number; lon: number; err: number } {
  // In production: use cross-correlation time-differences + station positions
  // to solve a linear least-squares for (lat, lon, origin_time).
  return { lat: 78.9215, lon: -11.934, err: 35 };
}

export default function EventDetail() {
  const route = useRoute<{ params: { eventId: string } }>();
  const { eventId } = route.params;

  // Mock event metadata keyed by eventId
  const [event] = useState({
    id: eventId,
    type: eventId === 'e1' ? 'Crevasse snap' :
          eventId === 'e2' ? 'Basal slide' :
          eventId === 'e3' ? 'Calving thump' :
          eventId === 'e4' ? 'Subglacial tremor' : 'Icequake (local)',
    magnitude: eventId === 'e3' ? 2.1 : eventId === 'e5' ? 1.4 : 0.8,
    correlScore: 0.85,
    templateId: 1,
    detectingNodes: [3, 7, 12],
    tsUtcMs: Date.now() - 12000,
  });

  const epicenter = useMemo(() => estimateEpicenter(eventId), [eventId]);

  const zWave  = useMemo(() => makeWaveform(1, event.type), [event.type]);
  const nsWave = useMemo(() => makeWaveform(2, event.type), [event.type]);
  const ewWave = useMemo(() => makeWaveform(3, event.type), [event.type]);

  const staLtaTrace = useMemo(() => {
    const t: number[] = [];
    for (let i = 0; i < 100; i++) {
      const v = i < 40 ? 0.5 + i * 0.01 : 3.5 + Math.sin(i * 0.3) * 0.4;
      t.push(Number(v.toFixed(3)));
    }
    return t;
  }, []);

  const chartConfig = {
    backgroundGradientFrom: '#141b2d',
    backgroundGradientTo: '#141b2d',
    color: (opacity = 1) => `rgba(13, 110, 253, ${opacity})`,
    labelColor: () => '#7a8aa8',
    strokeWidth: 1.5,
    propsForDots: { r: '0' },
  };

  const labels = Array.from({ length: 10 }, (_, i) => `${i - 5}s`);

  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <Title>{event.type}</Title>
          <Paragraph style={styles.meta}>
            Event ID: {event.id} · {new Date(event.tsUtcMs).toISOString()}
          </Paragraph>
          <Paragraph style={styles.meta}>
            Magnitude Mv {event.magnitude.toFixed(1)} · xcorr {event.correlScore.toFixed(2)} · template #{event.templateId}
          </Paragraph>
          <Paragraph style={styles.meta}>
            Detecting nodes: {event.detectingNodes.join(', ')}
          </Paragraph>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.h2}>Vertical (Z) waveform</Title>
          <LineChart
            data={{ labels, datasets: [{ data: zWave }] }}
            width={340} height={120} chartConfig={chartConfig}
            bezier style={styles.chart} withDots={false} withInnerLines={false}
          />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.h2}>North-South waveform</Title>
          <LineChart
            data={{ labels, datasets: [{ data: nsWave }] }}
            width={340} height={120} chartConfig={{
              ...chartConfig, color: (o = 1) => `rgba(23, 162, 184, ${o})`,
            }}
            bezier style={styles.chart} withDots={false} withInnerLines={false}
          />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.h2}>East-West waveform</Title>
          <LineChart
            data={{ labels, datasets: [{ data: ewWave }] }}
            width={340} height={120} chartConfig={{
              ...chartConfig, color: (o = 1) => `rgba(255, 193, 7, ${o})`,
            }}
            bezier style={styles.chart} withDots={false} withInnerLines={false}
          />
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.h2}>STA/LTA ratio</Title>
          <LineChart
            data={{
              labels,
              datasets: [
                { data: staLtaTrace, color: () => 'rgba(220, 53, 69, 1)' },
                { data: Array(100).fill(3.0), color: () => 'rgba(255, 193, 7, 0.8)' },
              ],
            }}
            width={340} height={120} chartConfig={chartConfig}
            bezier style={styles.chart} withDots={false} withInnerLines={false}
          />
          <Paragraph style={styles.meta}>
            Yellow line = trigger threshold (3.0).
          </Paragraph>
        </Card.Content>
      </Card>

      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.h2}>Estimated epicenter (TDOA)</Title>
          <Divider style={styles.div} />
          <Paragraph style={styles.meta}>
            Latitude:  {epicenter.lat.toFixed(5)}°{'\n'}
            Longitude: {epicenter.lon.toFixed(5)}°{'\n'}
            Location error: ±{epicenter.err} m
          </Paragraph>
          <Paragraph style={styles.meta}>
            Computed via cross-correlation TDOA across {event.detectingNodes.length} motes,
            least-squares solved on the gateway.
          </Paragraph>
        </Card.Content>
      </Card>

      <View style={styles.row}>
        <Button mode="contained" color="#28a745" onPress={() => {}}>
          Export mini-SEED
        </Button>
        <Button mode="outlined" color="#0d6efd" onPress={() => {}}>
          Upload to archive
        </Button>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0b1220', padding: 8 },
  card: { backgroundColor: '#141b2d', marginBottom: 8 },
  h2: { color: '#e6eaf2', fontSize: 16 },
  meta: { color: '#9fb0d0', fontSize: 13, marginTop: 4 },
  chart: { marginVertical: 8, borderRadius: 8 },
  div: { marginVertical: 8, backgroundColor: '#2a3556' },
  row: { flexDirection: 'row', justifyContent: 'space-around', paddingVertical: 12 },
});