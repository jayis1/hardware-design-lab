// screens/SurveyMapScreen.js — GPS-tagged sample map
// Author: jayis1  License: MIT
import React, { useState, useEffect, useRef } from 'react';
import { View, Text, StyleSheet, Alert, Platform } from 'react-native';
import MapView, { Marker, Circle } from 'react-native-maps';
import * as Location from 'expo-location';
import { theme, ANALYTES } from '../utils/theme';
import ble from '../utils/ble';

export default function SurveyMapScreen() {
  const [location, setLocation] = useState(null);
  const [points, setPoints] = useState([]);   // { coords, rec, analyte }
  const [colorBy, setColorBy] = useState('cdom');
  const subRef = useRef(null);

  useEffect(() => {
    (async () => {
      const { status } = await Location.requestForegroundPermissionsAsync();
      if (status !== 'granted') { Alert.alert('Location permission denied'); return; }
      const loc = await Location.getCurrentPositionAsync({ accuracy: Location.Accuracy.High });
      setLocation(loc.coords);
    })();
    subRef.current = ble.subscribeSamples(rec => {
      Location.getCurrentPositionAsync({ accuracy: Location.Accuracy.High })
        .then(l => setPoints(p => [...p, { coords: l.coords, rec }]))
        .catch(() => setPoints(p => [...p, { coords: location, rec }]));
    });
    return () => { if (subRef.current) subRef.current(); };
  }, []);

  const colorFor = (rec) => {
    const meta = ANALYTES.find(a => a.key === colorBy);
    if (!meta || !rec) return '#888';
    const v = valOf(colorBy, rec);
    return v >= meta.alarm ? theme.colors.alarm : meta.color;
  };

  return (
    <View style={styles.container}>
      <View style={styles.bar}>
        <Text style={styles.title}>Survey Map</Text>
        <Text style={styles.sub}>{points.length} points • color by {colorBy.toUpperCase()}</Text>
      </View>
      {location && (
        <MapView
          style={styles.map}
          initialRegion={{
            latitude: location.latitude,
            longitude: location.longitude,
            latitudeDelta: 0.01,
            longitudeDelta: 0.01,
          }}
        >
          {points.map((p, i) => (
            <Circle
              key={i}
              center={{ latitude: p.coords.latitude, longitude: p.coords.longitude }}
              radius={8}
              strokeColor={colorFor(p.rec)}
              fillColor={colorFor(p.rec)}
            />
          ))}
        </MapView>
      )}
      <View style={styles.legend}>
        {ANALYTES.map(a => (
          <Text key={a.key} onPress={() => setColorBy(a.key)}
                style={[styles.legItem, { color: colorBy === a.key ? theme.colors.text : theme.colors.textDim, fontWeight: colorBy === a.key ? '700' : '400' }]}>
            <Text style={{ color: a.color }}>●</Text> {a.label}
          </Text>
        ))}
      </View>
      <Text style={styles.exportHint}>Tap to recolor by analyte. Export from the Deployments screen.</Text>
    </View>
  );
}

function valOf(k, r) {
  switch (k) {
    case 'cdom': return r.cdomPpb;
    case 'chla': return r.chlaUgl;
    case 'phyc': return r.phycUgl;
    case 'oil':  return r.oilPpb;
    case 'turb': return r.turbNtuX100 / 100;
    default: return 0;
  }
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: theme.colors.bg },
  bar: { padding: theme.spacing.md, backgroundColor: theme.colors.surface },
  title: { color: theme.colors.text, fontSize: 20, fontWeight: '700' },
  sub: { color: theme.colors.textDim, fontSize: 12 },
  map: { flex: 1 },
  legend: { flexDirection: 'row', flexWrap: 'wrap', gap: 12, padding: theme.spacing.sm, backgroundColor: theme.colors.surface },
  legItem: { fontSize: 13, fontFamily: theme.fonts.mono },
  exportHint: { color: theme.colors.textDim, fontSize: 11, padding: 8, textAlign: 'center' },
});