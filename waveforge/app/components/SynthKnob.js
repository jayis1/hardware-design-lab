/**
 * SynthKnob — Reusable rotary knob component
 * Displays a circular knob with value and label
 */

import React, { useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  PanResponder,
} from 'react-native';

export default function SynthKnob({ label, value, min, max, step, onChange, size = 70 }) {
  const normalizedValue = (value - min) / (max - min);
  const angle = normalizedValue * 270 - 135; // -135° to +135°

  const panResponder = PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderMove: (_, gestureState) => {
      const sensitivity = 0.005;
      const delta = -gestureState.dy * sensitivity;
      const newNorm = Math.max(0, Math.min(1, normalizedValue + delta));
      const newValue = min + newNorm * (max - min);
      const stepped = Math.round(newValue / step) * step;
      onChange(Math.max(min, Math.min(max, stepped)));
    },
  });

  return (
    <View style={styles.container}>
      <View
        style={[styles.knobContainer, { width: size, height: size }]}
        {...panResponder.panHandlers}
      >
        {/* Outer ring */}
        <View style={[styles.knobRing, {
          width: size,
          height: size,
          borderRadius: size / 2,
        }]}>
          {/* Inner knob */}
          <View style={[styles.knobInner, {
            width: size - 8,
            height: size - 8,
            borderRadius: (size - 8) / 2,
          }]}>
            {/* Indicator dot */}
            <View style={[styles.knobDot, {
              transform: [{ rotate: `${angle}deg` }],
            }]} />
          </View>
        </View>

        {/* Arc indicator */}
        <View style={[styles.arcOverlay, { width: size, height: size }]}>
          <Text style={styles.knobValue}>
            {step >= 1 ? value.toFixed(0) : value.toFixed(3)}
          </Text>
        </View>
      </View>
      <Text style={styles.knobLabel}>{label}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    padding: 4,
  },
  knobContainer: {
    justifyContent: 'center',
    alignItems: 'center',
  },
  knobRing: {
    backgroundColor: '#0f3460',
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 2,
    borderColor: '#333',
  },
  knobInner: {
    backgroundColor: '#1a1a2e',
    justifyContent: 'center',
    alignItems: 'center',
    overflow: 'hidden',
  },
  knobDot: {
    position: 'absolute',
    width: 6,
    height: 6,
    borderRadius: 3,
    backgroundColor: '#e94560',
    top: 4,
    left: '50%',
    marginLeft: -3,
  },
  arcOverlay: {
    position: 'absolute',
    justifyContent: 'center',
    alignItems: 'center',
  },
  knobValue: {
    color: '#e0e0e0',
    fontSize: 9,
    fontWeight: 'bold',
  },
  knobLabel: {
    color: '#a0a0a0',
    fontSize: 12,
    marginTop: 4,
    fontWeight: 'bold',
  },
});