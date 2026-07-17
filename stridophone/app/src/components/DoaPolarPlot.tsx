/**
 * DoaPolarPlot.tsx — polar direction-of-arrival histogram renderer.
 *
 * Author : jayis1
 * License: MIT
 *
 * 16 azimuth bins (22.5° each). We draw radial bars whose length is
 * proportional to the histogram count, normalised to the max.
 */
import React from 'react';
import { View, StyleSheet } from 'react-native';
import Svg, { Line, Circle, Text as SvgText, G } from 'react-native-svg';

const N_BINS = 16;
const R_MAX = 80;
const CX = 100, CY = 100;

export default function DoaPolarPlot({ hist }: { hist: number[] }) {
  const max = Math.max(1, ...hist);
  return (
    <View style={styles.wrap}>
      <Svg width={200} height={200}>
        <Circle cx={CX} cy={CY} r={R_MAX} stroke="#333" fill="none" />
        <Circle cx={CX} cy={CY} r={R_MAX/2} stroke="#222" fill="none" />
        {Array.from({ length: N_BINS }).map((_, i) => {
          const az = (i / N_BINS) * 2 * Math.PI;
          const len = (hist[i] / max) * R_MAX;
          const x2 = CX + Math.cos(az) * len;
          const y2 = CY + Math.sin(az) * len;
          return (
            <Line
              key={i}
              x1={CX} y1={CY} x2={x2} y2={y2}
              stroke="#39d353" strokeWidth={6} strokeLinecap="round"
            />
          );
        })}
        <SvgText x={CX} y={CY+4} fill="#888" fontSize="10" textAnchor="middle">N</SvgText>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  wrap: { alignItems: 'center', marginVertical: 10 },
});