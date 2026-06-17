/**
 * SensorLayout.js — SonicSight Companion
 * Interactive sensor placement diagram. User marks transducer positions
 * on a photograph of the test object, and the app computes angular
 * coordinates for the tomographic reconstruction.
 * @author jayis1
 */

import React, { useState, useRef } from 'react';
import {
  View, Text, StyleSheet, Image, PanResponder, TouchableOpacity,
} from 'react-native';
import Svg, { Circle, Line, Text as SvgText } from 'react-native-svg';

const MAX_SENSORS = 32;

const SensorLayout = ({ onLayoutChange }) => {
  const [sensors, setSensors] = useState([]);
  const [imageSize, setImageSize] = useState({ w: 300, h: 300 });
  const imageRef = useRef();

  const addSensorAt = (x, y) => {
    if (sensors.length >= MAX_SENSORS) return;
    const idx = sensors.length;
    const angle = Math.atan2(y - imageSize.h / 2, x - imageSize.w / 2);
    const radius = Math.sqrt(
      (x - imageSize.w / 2) ** 2 + (y - imageSize.h / 2) ** 2
    );
    const newSensors = [...sensors, { x, y, angle, radius, idx }];
    setSensors(newSensors);
    if (onLayoutChange) onLayoutChange(newSensors);
  };

  const removeSensor = (idx) => {
    const newSensors = sensors.filter((s) => s.idx !== idx)
      .map((s, i) => ({ ...s, idx: i }));
    setSensors(newSensors);
    if (onLayoutChange) onLayoutChange(newSensors);
  };

  const panResponder = PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onPanResponderRelease: (evt) => {
      const { locationX, locationY } = evt.nativeEvent;
      addSensorAt(locationX, locationY);
    },
  });

  return (
    <View style={styles.container}>
      <Text style={styles.title}>
        Sensor Layout ({sensors.length}/{MAX_SENSORS})
      </Text>
      <View style={styles.svgContainer} {...panResponder.panHandlers}>
        <Svg width={imageSize.w} height={imageSize.h}>
          {/* Circle outline */}
          <Circle cx={imageSize.w / 2} cy={imageSize.h / 2}
                  r={Math.min(imageSize.w, imageSize.h) / 2 - 10}
                  fill="none" stroke="#333" strokeWidth={1} />
          {/* Sensor markers */}
          {sensors.map((s) => (
            <Circle key={s.idx} cx={s.x} cy={s.y} r={6}
                    fill="#00d2ff" stroke="#fff" strokeWidth={1.5} />
          ))}
          {/* Labels */}
          {sensors.map((s) => (
            <SvgText key={`l${s.idx}`} x={s.x + 8} y={s.y + 4}
                     fill="#fff" fontSize={10}>
              {s.idx + 1}
            </SvgText>
          ))}
        </Svg>
      </View>

      {/* Sensor list */}
      <View style={styles.list}>
        {sensors.map((s) => (
          <View key={s.idx} style={styles.sensorRow}>
            <Text style={styles.sensorLabel}>#{s.idx + 1}</Text>
            <Text style={styles.sensorInfo}>
              θ={((s.angle * 180) / Math.PI).toFixed(1)}° r={s.radius.toFixed(0)}px
            </Text>
            <TouchableOpacity onPress={() => removeSensor(s.idx)}>
              <Text style={styles.removeBtn}>✕</Text>
            </TouchableOpacity>
          </View>
        ))}
      </View>

      <TouchableOpacity style={styles.sendBtn}
        onPress={() => {
          /* Send sensor geometry to device over BLE */
          const coords = sensors.map((s) => ({
            angle: s.angle,
            radius: s.radius / Math.min(imageSize.w, imageSize.h),
          }));
          onLayoutChange?.(coords);
        }}>
        <Text style={styles.sendText}>Send to Device</Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { alignItems: 'center', padding: 12 },
  title: { color: '#00d2ff', fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  svgContainer: {
    backgroundColor: '#0a0a1a', borderRadius: 8,
    borderWidth: 1, borderColor: '#333',
    overflow: 'hidden',
  },
  list: { width: '100%', marginTop: 8 },
  sensorRow: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#1a1a2e', padding: 6,
    borderRadius: 4, marginBottom: 4,
  },
  sensorLabel: { color: '#00d2ff', fontSize: 12, fontWeight: 'bold', marginRight: 8 },
  sensorInfo: { color: '#aaa', fontSize: 11, flex: 1 },
  removeBtn: { color: '#f44336', fontSize: 16, padding: 4 },
  sendBtn: {
    backgroundColor: '#00d2ff', borderRadius: 8, padding: 10,
    paddingHorizontal: 24, marginTop: 8,
  },
  sendText: { color: '#000', fontSize: 14, fontWeight: 'bold' },
});

export default SensorLayout;