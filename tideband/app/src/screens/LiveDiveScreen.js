/**
 * @file    LiveDiveScreen.js
 * @brief   Real-time dive screen showing current velocity, depth,
 *          temperature, battery, and a scrolling current rose.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license MIT
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, ActivityIndicator,
  ScrollView,
} from 'react-native';
import Svg, { Circle, Line, Text as SvgText, Polygon } from 'react-native-svg';
import { useTideBand } from '../services/TideBandContext';
import { formatSpeed, formatDepth, headingToDirection } from '../utils/protocol';

export default function LiveDiveScreen() {
  const {
    connected, scanning, status, profileData, diveActive, error,
    startScan, units,
  } = useTideBand();

  const [displayData, setDisplayData] = useState({
    speed: 0,
    heading: 0,
    depth: 0,
    temp: 0,
    battery: 100,
    quality: 0,
  });

  // Update display from latest profile data
  useEffect(() => {
    if (profileData.length > 0) {
      const latest = profileData[profileData.length - 1];
      const speed = Math.sqrt(latest.vx ** 2 + latest.vy ** 2 + latest.vz ** 2);
      const heading = Math.atan2(latest.vy, latest.vx) * 180 / Math.PI;
      setDisplayData({
        speed: speed,
        heading: heading < 0 ? heading + 360 : heading,
        depth: latest.depth,
        temp: latest.temp,
        battery: status?.batteryPct || 100,
        quality: latest.quality,
      });
    }
  }, [profileData, status]);

  // Get speed and depth history for waterfall (last 60 samples)
  const history = profileData.slice(-60);
  const maxDepth = Math.max(...history.map(p => p.depth), 1);
  const maxSpeed = Math.max(...history.map(p =>
    Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2)
  ), 0.5);

  if (!connected && !scanning) {
    return (
      <View style={styles.container}>
        <Text style={styles.title}>TideBand</Text>
        <Text style={styles.subtitle}>Wrist-Worn Current Profiler</Text>
        <TouchableOpacity style={styles.scanButton} onPress={startScan}>
          <Text style={styles.scanButtonText}>Scan for Device</Text>
        </TouchableOpacity>
        {error && <Text style={styles.errorText}>Error: {error}</Text>}
      </View>
    );
  }

  if (scanning) {
    return (
      <View style={styles.container}>
        <ActivityIndicator size="large" color="#0080FF" />
        <Text style={styles.scanningText}>Scanning for TideBand...</Text>
      </View>
    );
  }

  const qualityColors = ['#FF4444', '#FFAA00', '#44AA44', '#00AA00'];
  const qualityLabels = ['Poor', 'Fair', 'Good', 'Excellent'];

  return (
    <ScrollView style={styles.container}>
      {/* Status bar */}
      <View style={styles.statusBar}>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Battery</Text>
          <Text style={styles.statusValue}>{displayData.battery}%</Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Status</Text>
          <Text style={[styles.statusValue,
            { color: diveActive ? '#00AA00' : '#808080' }]}>
            {diveActive ? 'DIVING' : 'SURFACE'}
          </Text>
        </View>
        <View style={styles.statusItem}>
          <Text style={styles.statusLabel}>Quality</Text>
          <Text style={[styles.statusValue,
            { color: qualityColors[displayData.quality] }]}>
            {qualityLabels[displayData.quality]}
          </Text>
        </View>
      </View>

      {/* Current Rose */}
      <View style={styles.roseContainer}>
        <CurrentRose
          speed={displayData.speed}
          heading={displayData.heading}
          maxSpeed={5}
          units={units}
        />
      </View>

      {/* Current data readout */}
      <View style={styles.dataRow}>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Current Speed</Text>
          <Text style={styles.dataValue}>
            {formatSpeed(displayData.speed, units)}
          </Text>
        </View>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Direction</Text>
          <Text style={styles.dataValue}>
            {displayData.heading.toFixed(0)}° {headingToDirection(displayData.heading)}
          </Text>
        </View>
      </View>

      <View style={styles.dataRow}>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Depth</Text>
          <Text style={styles.dataValue}>
            {formatDepth(displayData.depth, units)}
          </Text>
        </View>
        <View style={styles.dataCard}>
          <Text style={styles.dataLabel}>Water Temp</Text>
          <Text style={styles.dataValue}>
            {displayData.temp.toFixed(1)}°C
          </Text>
        </View>
      </View>

      {/* Depth-vs-Speed waterfall plot */}
      <View style={styles.waterfallContainer}>
        <Text style={styles.sectionTitle}>Current Profile (last 60 samples)</Text>
        <WaterfallPlot
          data={history}
          maxDepth={maxDepth}
          maxSpeed={maxSpeed}
          width={300}
          height={120}
        />
      </View>
    </ScrollView>
  );
}

// ---- Current Rose SVG Component ----
function CurrentRose({ speed, heading, maxSpeed, units }) {
  const cx = 90, cy = 90, r = 70;
  const arrowLen = (speed / maxSpeed) * r;
  const angleRad = heading * Math.PI / 180;
  const ex = cx + Math.sin(angleRad) * arrowLen;
  const ey = cy - Math.cos(angleRad) * arrowLen;

  return (
    <Svg width={180} height={180}>
      {/* Outer circle */}
      <Circle cx={cx} cy={cy} r={r} fill="none" stroke="#808080" strokeWidth={2} />

      {/* Cardinal direction marks */}
      <Line x1={cx} y1={cy - r} x2={cx} y2={cy - r + 8} stroke="#808080" strokeWidth={2} />
      <Line x1={cx} y1={cy + r} x2={cx} y2={cy + r - 8} stroke="#808080" strokeWidth={2} />
      <Line x1={cx - r} y1={cy} x2={cx - r + 8} y2={cy} stroke="#808080" strokeWidth={2} />
      <Line x1={cx + r} y1={cy} x2={cx + r - 8} y2={cy} stroke="#808080" strokeWidth={2} />

      {/* Cardinal labels */}
      <SvgText x={cx} y={cy - r - 5} fontSize={12} fill="#808080" textAnchor="middle">N</SvgText>
      <SvgText x={cx} y={cy + r + 15} fontSize={12} fill="#808080" textAnchor="middle">S</SvgText>
      <SvgText x={cx - r - 10} y={cy + 4} fontSize={12} fill="#808080" textAnchor="middle">W</SvgText>
      <SvgText x={cx + r + 10} y={cy + 4} fontSize={12} fill="#808080" textAnchor="middle">E</SvgText>

      {/* Current arrow */}
      {speed > 0.01 && (
        <>
          <Line x1={cx} y1={cy} x2={ex} y2={ey} stroke="#0080FF" strokeWidth={3} />
          <Circle cx={ex} cy={ey} r={4} fill="#0080FF" />
        </>
      )}

      {/* Center dot */}
      <Circle cx={cx} cy={cy} r={3} fill="#444444" />
    </Svg>
  );
}

// ---- Waterfall Plot (depth vs speed over time) ----
function WaterfallPlot({ data, maxDepth, maxSpeed, width, height }) {
  if (data.length === 0) {
    return <Text style={styles.noData}>No data yet</Text>;
  }

  const barWidth = width / 60;
  const speedColors = (speed) => {
    const ratio = Math.min(speed / maxSpeed, 1);
    const r = Math.round(255 * ratio);
    const g = Math.round(255 * (1 - ratio));
    return `rgb(${r}, ${g}, 0)`;
  };

  return (
    <Svg width={width} height={height}>
      {/* Depth axis (left side) */}
      <Line x1={20} y1={0} x2={20} y2={height} stroke="#808080" strokeWidth={1} />
      <SvgText x={5} y={10} fontSize={8} fill="#808080">0m</SvgText>
      <SvgText x={5} y={height - 2} fontSize={8} fill="#808080">
        {maxDepth.toFixed(0)}m
      </SvgText>

      {/* Bars for each sample */}
      {data.map((p, i) => {
        const speed = Math.sqrt(p.vx ** 2 + p.vy ** 2 + p.vz ** 2);
        const x = 20 + i * barWidth;
        const barH = (p.depth / maxDepth) * height;
        return (
          <Rect
            key={i}
            x={x}
            y={height - barH}
            width={barWidth - 1}
            height={barH}
            fill={speedColors(speed)}
            opacity={0.8}
          />
        );
      })}
    </Svg>
  );
}

// ---- Import Rect for WaterfallPlot ----
import { Rect } from 'react-native-svg';

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F0F0F0',
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#0080FF',
    textAlign: 'center',
    marginTop: 80,
  },
  subtitle: {
    fontSize: 16,
    color: '#808080',
    textAlign: 'center',
    marginBottom: 40,
  },
  scanButton: {
    backgroundColor: '#0080FF',
    paddingVertical: 15,
    paddingHorizontal: 40,
    borderRadius: 10,
    alignSelf: 'center',
  },
  scanButtonText: {
    color: '#FFFFFF',
    fontSize: 18,
    fontWeight: 'bold',
  },
  errorText: {
    color: '#FF0000',
    textAlign: 'center',
    marginTop: 20,
  },
  scanningText: {
    fontSize: 16,
    color: '#808080',
    textAlign: 'center',
    marginTop: 20,
  },
  statusBar: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    backgroundColor: '#FFFFFF',
    paddingVertical: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#E0E0E0',
  },
  statusItem: {
    alignItems: 'center',
  },
  statusLabel: {
    fontSize: 12,
    color: '#808080',
  },
  statusValue: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#333333',
  },
  roseContainer: {
    alignItems: 'center',
    paddingVertical: 20,
    backgroundColor: '#FFFFFF',
    marginVertical: 8,
  },
  dataRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingHorizontal: 16,
    marginVertical: 4,
  },
  dataCard: {
    flex: 1,
    backgroundColor: '#FFFFFF',
    borderRadius: 8,
    padding: 12,
    marginHorizontal: 4,
    alignItems: 'center',
  },
  dataLabel: {
    fontSize: 12,
    color: '#808080',
    marginBottom: 4,
  },
  dataValue: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#0080FF',
  },
  waterfallContainer: {
    backgroundColor: '#FFFFFF',
    margin: 16,
    padding: 12,
    borderRadius: 8,
    alignItems: 'center',
  },
  sectionTitle: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#333333',
    marginBottom: 8,
  },
  noData: {
    fontSize: 14,
    color: '#808080',
    textAlign: 'center',
    padding: 20,
  },
});