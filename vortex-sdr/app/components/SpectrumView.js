/**
 * SpectrumView.js — Custom spectrum/waterfall display component
 * Renders FFT data as spectrum trace and waterfall
 */

import React, { useRef, useEffect, useCallback } from 'react';
import { View, StyleSheet, Dimensions } from 'react-native';
import Svg, { Line, Rect, Text, G, Path, Defs, LinearGradient, Stop } from 'react-native-svg';
import { theme } from './Theme';

const SCREEN_WIDTH = Dimensions.get('window').width;
const SPECTRUM_HEIGHT = 200;
const WATERFALL_HEIGHT = 150;
const MARGIN_LEFT = 40;
const MARGIN_RIGHT = 10;
const MARGIN_TOP = 20;
const MARGIN_BOTTOM = 30;
const PLOT_WIDTH = SCREEN_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
const PLOT_HEIGHT = SPECTRUM_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;

// dBm range for display
const DEFAULT_REF_LEVEL = -20;  // dBm
const DEFAULT_SCALE = 10;       // dB/div
const DIVISIONS_Y = 10;
const DIVISIONS_X = 10;

// Color map for waterfall (cold to hot)
function powerToColor(powerDbm, minDbm, maxDbm) {
  const normalized = (powerDbm - minDbm) / (maxDbm - minDbm);
  const clamped = Math.max(0, Math.min(1, normalized));

  let r, g, b;
  if (clamped < 0.25) {
    // Black to blue
    const t = clamped / 0.25;
    r = 0; g = 0; b = Math.floor(t * 255);
  } else if (clamped < 0.5) {
    // Blue to cyan
    const t = (clamped - 0.25) / 0.25;
    r = 0; g = Math.floor(t * 255); b = 255;
  } else if (clamped < 0.75) {
    // Cyan to yellow
    const t = (clamped - 0.5) / 0.25;
    r = Math.floor(t * 255); g = 255; b = Math.floor(255 * (1 - t));
  } else {
    // Yellow to red
    const t = (clamped - 0.75) / 0.25;
    r = 255; g = Math.floor(255 * (1 - t)); b = 0;
  }

  return `rgb(${r},${g},${b})`;
}

// Format frequency for display
function formatFreq(hz) {
  if (hz >= 1e9) return `${(hz / 1e9).toFixed(3)} GHz`;
  if (hz >= 1e6) return `${(hz / 1e6).toFixed(3)} MHz`;
  if (hz >= 1e3) return `${(hz / 1e3).toFixed(1)} kHz`;
  return `${hz} Hz`;
}

export default function SpectrumView({ spectrumData, deviceStatus, style }) {
  const waterfallHistory = useRef([]);
  const maxWaterfallLines = 100;

  // Update waterfall history when new spectrum data arrives
  useEffect(() => {
    if (!spectrumData || !spectrumData.bins) return;

    const { bins, numBins } = spectrumData;
    const line = new Array(Math.min(numBins, PLOT_WIDTH));

    for (let i = 0; i < PLOT_WIDTH; i++) {
      const binIndex = Math.floor((i / PLOT_WIDTH) * numBins);
      line[i] = bins[binIndex] / 10.0; // Convert from dBm*10 to dBm
    }

    waterfallHistory.current.push(line);
    if (waterfallHistory.current.length > maxWaterfallLines) {
      waterfallHistory.current.shift();
    }
  }, [spectrumData]);

  // Calculate display parameters
  const refLevel = deviceStatus?.refLevel ?? DEFAULT_REF_LEVEL;
  const scale = deviceStatus?.scale ?? DEFAULT_SCALE;
  const startFreq = deviceStatus?.startFreq ?? 88e6;
  const stopFreq = deviceStatus?.stopFreq ?? 108e6;

  const minDbm = refLevel - scale * DIVISIONS_Y;
  const maxDbm = refLevel;

  // Build grid lines
  const gridLinesX = [];
  const gridLabelsX = [];
  for (let i = 0; i <= DIVISIONS_X; i++) {
    const x = MARGIN_LEFT + (i / DIVISIONS_X) * PLOT_WIDTH;
    const freq = startFreq + (i / DIVISIONS_X) * (stopFreq - startFreq);
    gridLinesX.push(x);
    gridLabelsX.push({ x, label: formatFreq(freq) });
  }

  const gridLinesY = [];
  const gridLabelsY = [];
  for (let i = 0; i <= DIVISIONS_Y; i++) {
    const y = MARGIN_TOP + (i / DIVISIONS_Y) * PLOT_HEIGHT;
    const dbm = refLevel - i * scale;
    gridLinesY.push(y);
    gridLabelsY.push({ y, label: `${dbm}` });
  }

  // Build spectrum trace path
  let spectrumPath = '';
  if (spectrumData && spectrumData.bins) {
    const { bins, numBins } = spectrumData;
    let first = true;
    for (let i = 0; i < PLOT_WIDTH; i++) {
      const binIndex = Math.floor((i / PLOT_WIDTH) * numBins);
      if (binIndex >= numBins) continue;
      const power = bins[binIndex] / 10.0; // dBm
      const x = MARGIN_LEFT + i;
      const y = MARGIN_TOP + ((maxDbm - power) / (maxDbm - minDbm)) * PLOT_HEIGHT;
      const clampedY = Math.max(MARGIN_TOP, Math.min(MARGIN_TOP + PLOT_HEIGHT, y));

      if (first) {
        spectrumPath += `M ${x} ${clampedY}`;
        first = false;
      } else {
        spectrumPath += ` L ${x} ${clampedY}`;
      }
    }
  }

  return (
    <View style={[styles.container, style]}>
      {/* Spectrum trace area */}
      <Svg width={SCREEN_WIDTH} height={SPECTRUM_HEIGHT}>
        {/* Background */}
        <Rect
          x={MARGIN_LEFT}
          y={MARGIN_TOP}
          width={PLOT_WIDTH}
          height={PLOT_HEIGHT}
          fill={theme.colors.background}
          stroke={theme.colors.border}
          strokeWidth={1}
        />

        {/* Grid lines - Y axis (dBm) */}
        {gridLinesY.map((y, i) => (
          <G key={`gy${i}`}>
            <Line
              x1={MARGIN_LEFT}
              y1={y}
              x2={MARGIN_LEFT + PLOT_WIDTH}
              y2={y}
              stroke={i % 2 === 0 ? theme.colors.gridLineMajor : theme.colors.gridLine}
              strokeWidth={0.5}
            />
            <Text
              x={MARGIN_LEFT - 4}
              y={y + 4}
              fill={theme.colors.textSecondary}
              fontSize={theme.fontSize.xs}
              textAnchor="end"
            >
              {gridLabelsY[i]?.label}
            </Text>
          </G>
        ))}

        {/* Grid lines - X axis (frequency) */}
        {gridLinesX.map((x, i) => (
          <G key={`gx${i}`}>
            <Line
              x1={x}
              y1={MARGIN_TOP}
              x2={x}
              y2={MARGIN_TOP + PLOT_HEIGHT}
              stroke={i % 2 === 0 ? theme.colors.gridLineMajor : theme.colors.gridLine}
              strokeWidth={0.5}
            />
            {i % 2 === 0 && (
              <Text
                x={x}
                y={MARGIN_TOP + PLOT_HEIGHT + 14}
                fill={theme.colors.textSecondary}
                fontSize={theme.fontSize.xs}
                textAnchor="middle"
              >
                {gridLabelsX[i]?.label}
              </Text>
            )}
          </G>
        ))}

        {/* Spectrum trace */}
        {spectrumPath ? (
          <Path
            d={spectrumPath}
            fill="none"
            stroke={theme.colors.primary}
            strokeWidth={1.5}
          />
        ) : (
          <Text
            x={SCREEN_WIDTH / 2}
            y={SPECTRUM_HEIGHT / 2}
            fill={theme.colors.textDim}
            fontSize={theme.fontSize.lg}
            textAnchor="middle"
          >
            No signal data
          </Text>
        )}

        {/* Y-axis label */}
        <Text
          x={6}
          y={SPECTRUM_HEIGHT / 2}
          fill={theme.colors.textSecondary}
          fontSize={theme.fontSize.xs}
          textAnchor="middle"
          rotation={-90}
        >
          dBm
        </Text>
      </Svg>

      {/* Waterfall area */}
      <View style={styles.waterfallContainer}>
        {waterfallHistory.current.length > 0 ? (
          <Svg width={SCREEN_WIDTH} height={WATERFALL_HEIGHT}>
            <Rect
              x={MARGIN_LEFT}
              y={0}
              width={PLOT_WIDTH}
              height={WATERFALL_HEIGHT}
              fill={theme.colors.background}
            />
            {waterfallHistory.current.map((line, lineIndex) => {
              const y = Math.floor(
                (lineIndex / maxWaterfallLines) * WATERFALL_HEIGHT
              );
              const lineHeight = Math.max(
                1,
                Math.floor(WATERFALL_HEIGHT / maxWaterfallLines)
              );

              return line.map((power, pixelIndex) => {
                const x = MARGIN_LEFT + pixelIndex;
                const color = powerToColor(power, minDbm, maxDbm);
                return (
                  <Rect
                    key={`wf_${lineIndex}_${pixelIndex}`}
                    x={x}
                    y={y}
                    width={1}
                    height={lineHeight}
                    fill={color}
                  />
                );
              });
            })}
          </Svg>
        ) : (
          <View style={styles.waterfallPlaceholder}>
            <Svg width={SCREEN_WIDTH} height={WATERFALL_HEIGHT}>
              <Rect
                x={MARGIN_LEFT}
                y={0}
                width={PLOT_WIDTH}
                height={WATERFALL_HEIGHT}
                fill={theme.colors.background}
                stroke={theme.colors.border}
                strokeWidth={1}
              />
              <Text
                x={SCREEN_WIDTH / 2}
                y={WATERFALL_HEIGHT / 2}
                fill={theme.colors.textDim}
                fontSize={theme.fontSize.md}
                textAnchor="middle"
              >
                Waterfall — waiting for data
              </Text>
            </Svg>
          </View>
        )}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: theme.colors.background,
  },
  waterfallContainer: {
    marginTop: 2,
  },
  waterfallPlaceholder: {
    justifyContent: 'center',
    alignItems: 'center',
  },
});