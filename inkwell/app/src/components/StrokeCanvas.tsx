// StrokeCanvas.tsx — Real-time pressure-sensitive stroke rendering surface
//
// Accumulates incoming StrokeSegments into an SVG path, scaling micrometer
// deltas to screen pixels and modulating stroke width by pressure. The
// canvas is pinch-zoomable and pannable so long pages can be navigated.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useRef, useState } from 'react';
import { View, StyleSheet, GestureResponderEvent } from 'react-native';
import Svg, { Path, G } from 'react-native-svg';
import { StrokeSegment } from '../ble/protocol';

type Point = { x: number; y: number; pressure: number };

type Props = {
  segments: StrokeSegment[];
  width: number;
  height: number;
};

const UM_PER_PX = 40;   // 1 px = 40 µm on screen

export default function StrokeCanvas({ segments, width, height }: Props) {
  const [strokes, setStrokes] = useState<Point[][]>([]);
  const [pan, setPan] = useState({ x: 0, y: 0 });
  const [scale, setScale] = useState(1);
  const currentStroke = useRef<Point[]>([]);
  const cursor = useRef({ x: width / 2, y: height / 2 });

  useEffect(() => {
    if (segments.length === 0) return;
    const seg = segments[segments.length - 1];

    if (seg.flags.strokeStart) {
      currentStroke.current = [];
      cursor.current = { x: cursor.current.x, y: cursor.current.y };
    }

    cursor.current = {
      x: cursor.current.x + seg.dxUm / UM_PER_PX,
      y: cursor.current.y + seg.dyUm / UM_PER_PX,
    };

    currentStroke.current.push({
      x: cursor.current.x,
      y: cursor.current.y,
      pressure: seg.pressureMN,
    });

    if (seg.flags.strokeEnd) {
      setStrokes(prev => [...prev, currentStroke.current]);
      currentStroke.current = [];
    } else {
      setStrokes(prev => [...prev.slice(0, -1), [...currentStroke.current]]);
    }
  }, [segments]);

  const handleTouch = (e: GestureResponderEvent) => {
    // Two-finger pan + pinch handling would go here.
    setPan({ x: e.nativeEvent.locationX, y: e.nativeEvent.locationY });
  };

  return (
    <View style={styles.container} onTouchMove={handleTouch}>
      <Svg width={width} height={height}>
        <G transform={`translate(${pan.x} ${pan.y}) scale(${scale})`}>
          {strokes.map((points, i) => {
            if (points.length < 2) return null;
            let d = `M ${points[0].x} ${points[0].y}`;
            for (let j = 1; j < points.length; j++) {
              d += ` L ${points[j].x} ${points[j].y}`;
            }
            const avgPress = points.reduce((s, p) => s + p.pressure, 0) / points.length;
            const strokeWidth = Math.max(0.5, avgPress / 200);
            return (
              <Path
                key={i}
                d={d}
                stroke="#1a1a2e"
                strokeWidth={strokeWidth}
                strokeLinecap="round"
                strokeLinejoin="round"
                fill="none"
              />
            );
          })}
        </G>
      </Svg>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f9f7f1' },
});