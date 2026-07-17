/**
 * DensityHeatmapScreen.tsx — 24h × azimuth polar density heatmap.
 *
 * Author : jayis1
 * License: MIT
 *
 * Fetches the last 24 h of events, buckets them by hour-of-day and
 * azimuth bin, and renders a polar grid where colour intensity encodes
 * event density. Useful for locating the infested joist / grain bay.
 */
import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';
import { useRoute } from '@react-navigation/native';
import { useDevices } from '../context/DeviceContext';
import { StridoEvent } from '../api';
import Svg, { Rect, Text as SvgText } from 'react-native-svg';

const N_AZ = 16;
const HOURS = 24;

function densityColor(n: number, max: number): string {
  if (max === 0) return '#111';
  const t = n / max;
  const r = Math.floor(t * 255);
  const g = Math.floor((1 - t) * 80);
  return `rgb(${r},${g},30)`;
}

export default function DensityHeatmapScreen() {
  const route = useRoute<any>();
  const { client } = useDevices();
  const [grid, setGrid] = useState<number[][]>([]);  // [hour][az]
  const [max, setMax] = useState(0);

  useEffect(() => {
    if (!client) return;
    client.getEvents(0, 2000).then((evs) => {
      const now = Math.floor(Date.now() / 1000);
      const g: number[][] = Array.from({ length: HOURS }, () => new Array(N_AZ).fill(0));
      let m = 0;
      evs.forEach((e) => {
        const ageH = Math.floor((now - e.t) / 3600);
        if (ageH < 0 || ageH >= HOURS) return;
        const az = e.az % N_AZ;
        g[HOURS - 1 - ageH][az]++;
        if (g[HOURS - 1 - ageH][az] > m) m = g[HOURS - 1 - ageH][az];
      });
      setGrid(g); setMax(m);
    }).catch(() => {});
  }, [client]);

  const cellW = 16, cellH = 18;
  const labelW = 28;

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.h1}>24h × Azimuth density</Text>
      <Text style={styles.sub}>Top = 24 h ago · Bottom = now · Left→Right = azimuth bins 0..15</Text>
      <View style={{ flexDirection: 'row' }}>
        <Svg width={labelW + N_AZ * cellW + 10} height={HOURS * cellH + 24}>
          {/* hour labels */}
          {Array.from({ length: HOURS }).map((_, h) => (
            <SvgText key={h} x={2} y={h * cellH + 14} fill="#888" fontSize="9">
              {`-${h}h`}
            </SvgText>
          ))}
          {/* cells */}
          {grid.map((row, h) =>
            row.map((n, a) => (
              <Rect key={`${h}-${a}`}
                x={labelW + a * cellW} y={h * cellH}
                width={cellW - 1} height={cellH - 1}
                fill={densityColor(n, max)} />
            ))
          )}
        </Svg>
      </View>
      <Text style={styles.legend}>Brighter red = higher event density at that bearing/time.</Text>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#0f0f17', padding: 14 },
  h1: { color: '#e0e0e0', fontSize: 20, fontWeight: 'bold' },
  sub: { color: '#888', fontSize: 11, marginVertical: 6 },
  legend: { color: '#aaa', fontSize: 12, marginTop: 10 },
});