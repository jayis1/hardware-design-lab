/**
 * SkyPilot — Map Screen
 * Shows drone position on map with trail
 */

import React, { useState, useEffect, useContext } from 'react';
import { View, Text, StyleSheet, Dimensions } from 'react-native';
import MapView, { Marker, Polyline } from 'react-native-maps';
import { ConnectionContext } from '../components/ConnectionContext';
import { Colors, Typography, Spacing } from '../utils/theme';
import { Commands } from '../utils/protocol';

const { width, height } = Dimensions.get('window');

function MapScreen() {
  const { connection, sendMessage } = useContext(ConnectionContext);
  const [dronePosition, setDronePosition] = useState({
    latitude: 37.7749,
    longitude: -122.4194,
    altitude: 0,
    heading: 0,
    speed: 0,
  });
  const [homePosition, setHomePosition] = useState(null);
  const [trail, setTrail] = useState([]);
  const [satellites, setSatellites] = useState(0);

  useEffect(() => {
    if (!connection.connected) return;

    const interval = setInterval(() => {
      sendMessage({
        cmd: Commands.TELEMETRY_REQUEST,
        payload: { fields: ['gnss'] },
      });
    }, 500);

    return () => clearInterval(interval);
  }, [connection.connected]);

  useEffect(() => {
    if (!connection.connected) return;

    const unsubscribe = connection.onMessage((msg) => {
      if (msg.cmd === Commands.GPS_REPORT) {
        const newPos = {
          latitude: msg.payload.lat,
          longitude: msg.payload.lon,
          altitude: msg.payload.alt,
          heading: msg.payload.heading || 0,
          speed: msg.payload.speed || 0,
        };
        setDronePosition(newPos);
        setSatellites(msg.payload.satellites);

        if (msg.payload.fix_type >= 3) {
          setTrail((prev) => [...prev, newPos].slice(-500));

          if (!homePosition) {
            setHomePosition(newPos);
          }
        }
      }
    });

    return unsubscribe;
  }, [connection.connected]);

  return (
    <View style={styles.container}>
      <MapView
        style={styles.map}
        initialRegion={{
          latitude: dronePosition.latitude,
          longitude: dronePosition.longitude,
          latitudeDelta: 0.01,
          longitudeDelta: 0.01,
        }}
        showsUserLocation
        showsCompass
      >
        {homePosition && (
          <Marker
            coordinate={homePosition}
            title="Home"
            description="Takeoff position"
            pinColor="green"
          />
        )}

        <Marker
          coordinate={{
            latitude: dronePosition.latitude,
            longitude: dronePosition.longitude,
          }}
          title="Drone"
          description={`Alt: ${dronePosition.altitude.toFixed(1)}m | Spd: ${dronePosition.speed.toFixed(1)}m/s`}
          pinColor="red"
        />

        {trail.length > 1 && (
          <Polyline
            coordinates={trail.map((p) => ({
              latitude: p.latitude,
              longitude: p.longitude,
            }))}
            strokeColor={Colors.primary}
            strokeWidth={3}
          />
        )}
      </MapView>

      <View style={styles.overlay}>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Lat</Text>
          <Text style={styles.infoValue}>{dronePosition.latitude.toFixed(6)}°</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Lon</Text>
          <Text style={styles.infoValue}>{dronePosition.longitude.toFixed(6)}°</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Alt</Text>
          <Text style={styles.infoValue}>{dronePosition.altitude.toFixed(1)} m</Text>
        </View>
        <View style={styles.infoRow}>
          <Text style={styles.infoLabel}>Sats</Text>
          <Text style={styles.infoValue}>{satellites}</Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  map: {
    width: width,
    height: height - 120,
  },
  overlay: {
    position: 'absolute',
    bottom: 0,
    left: 0,
    right: 0,
    backgroundColor: 'rgba(0,0,0,0.8)',
    padding: Spacing.md,
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 2,
  },
  infoLabel: {
    ...Typography.caption,
    color: Colors.textSecondary,
  },
  infoValue: {
    ...Typography.caption,
    color: '#fff',
    fontFamily: 'monospace',
  },
});

export default MapScreen;