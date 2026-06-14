/**
 * SkyPilot — Attitude Indicator Component
 * Artificial horizon and heading display for flight control
 */

import React from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';

const { width: SCREEN_WIDTH } = Dimensions.get('window');
const INDICATOR_SIZE = Math.min(SCREEN_WIDTH - 40, 280);

function AttitudeIndicator({ roll, pitch, heading }) {
  // Clamp pitch to ±45° for display
  const clampedPitch = Math.max(-45, Math.min(45, pitch));
  const pitchOffset = (clampedPitch / 45) * (INDICATOR_SIZE / 2);

  return (
    <View style={styles.container}>
      {/* Heading display */}
      <View style={styles.headingContainer}>
        <Text style={styles.headingText}>
          HDG {heading.toFixed(0).padStart(3, '0')}°
        </Text>
      </View>

      {/* Attitude ball */}
      <View style={styles.ballContainer}>
        <View
          style={[
            styles.ball,
            {
              transform: [
                { rotate: `${-roll}deg` },
                { translateY: pitchOffset },
              ],
            },
          ]}
        >
          {/* Sky */}
          <View style={styles.sky}>
            <Text style={styles.pitchMark}>
              {clampedPitch > 0 ? `+${clampedPitch.toFixed(0)}` : clampedPitch.toFixed(0)}°
            </Text>
          </View>
          {/* Horizon line */}
          <View style={styles.horizonLine} />
          {/* Ground */}
          <View style={styles.ground} />
        </View>

        {/* Fixed overlay */}
        <View style={styles.overlay}>
          {/* Wings symbol */}
          <View style={styles.wings}>
            <View style={[styles.wingLine, styles.leftWing]} />
            <View style={styles.wingCenter} />
            <View style={[styles.wingLine, styles.rightWing]} />
          </View>
        </View>
      </View>

      {/* Roll indicator */}
      <View style={styles.rollContainer}>
        <Text style={styles.rollText}>ROLL {roll.toFixed(1)}°</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
    paddingVertical: 8,
  },
  headingContainer: {
    backgroundColor: 'rgba(0,0,0,0.7)',
    paddingHorizontal: 16,
    paddingVertical: 4,
    borderRadius: 4,
    marginBottom: 8,
  },
  headingText: {
    color: '#00ff88',
    fontSize: 16,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
  ballContainer: {
    width: INDICATOR_SIZE,
    height: INDICATOR_SIZE,
    borderRadius: INDICATOR_SIZE / 2,
    overflow: 'hidden',
    borderWidth: 2,
    borderColor: '#444',
    position: 'relative',
  },
  ball: {
    width: INDICATOR_SIZE * 2,
    height: INDICATOR_SIZE * 2,
    position: 'absolute',
    left: -INDICATOR_SIZE / 2,
    top: -INDICATOR_SIZE / 2,
  },
  sky: {
    flex: 1,
    backgroundColor: '#1a4a8a',
    justifyContent: 'center',
    alignItems: 'center',
  },
  pitchMark: {
    color: 'rgba(255,255,255,0.6)',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  horizonLine: {
    height: 2,
    backgroundColor: '#fff',
  },
  ground: {
    flex: 1,
    backgroundColor: '#5a3a1a',
  },
  overlay: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    justifyContent: 'center',
    alignItems: 'center',
  },
  wings: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  wingLine: {
    width: 50,
    height: 3,
    backgroundColor: '#ff8800',
  },
  leftWing: {
    marginRight: 4,
  },
  rightWing: {
    marginLeft: 4,
  },
  wingCenter: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: '#ff8800',
  },
  rollContainer: {
    marginTop: 8,
    backgroundColor: 'rgba(0,0,0,0.7)',
    paddingHorizontal: 16,
    paddingVertical: 4,
    borderRadius: 4,
  },
  rollText: {
    color: '#ffaa00',
    fontSize: 14,
    fontFamily: 'monospace',
    fontWeight: 'bold',
  },
});

export { AttitudeIndicator };
export default AttitudeIndicator;