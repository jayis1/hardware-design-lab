/*
 * SiteMapScreen.js — Map of deployed Lichenwatch nodes with state colors.
 *
 * Author: jayis1
 * License: MIT
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Platform } from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { STATE_NAMES, STATE_COLORS } from '../utils/lichen';

const SITES_KEY = 'lichenwatch_sites';

/* A few example field sites (a real app would fetch from a gateway API). */
const DEFAULT_SITES = [
  {
    id: 'LW-0001',
    name: 'Cairn Ridge north face',
    lat: 56.123,
    lon: -4.789,
    state: 0,
    species: 'Rhizocarpon geographicum',
  },
  {
    id: 'LW-0002',
    name: 'Glen Burn acid bog rock',
    lat: 56.198,
    lon: -4.612,
    state: 1,
    species: 'Cladonia portentosa',
  },
  {
    id: 'LW-0003',
    name: 'Industrial downwind transect A',
    lat: 55.870,
    lon: -4.250,
    state: 2,
    species: 'Xanthoria parietina',
  },
  {
    id: 'LW-0004',
    name: 'Tundra polygon refugium',
    lat: 68.421,
    lon: -133.733,
    state: 4,
    species: 'Flavocetraria nivalis',
  },
];

export default function SiteMapScreen() {
  const [sites, setSites] = useState(DEFAULT_SITES);
  const [selected, setSelected] = useState(null);

  const loadSites = async () => {
    const raw = await AsyncStorage.getItem(SITES_KEY);
    if (raw) setSites(JSON.parse(raw));
    else await AsyncStorage.setItem(SITES_KEY, JSON.stringify(DEFAULT_SITES));
  };

  useEffect(() => { loadSites(); }, []);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Deployed Nodes</Text>
      <Text style={styles.subtitle}>
        {sites.length} node(s) across {new Set(sites.map((s) => Math.round(s.lat))).size} region(s)
      </Text>

      {/* Map: we use a list-based representation because react-native-maps
          requires native build. The structure mirrors a MapView with markers. */}
      <View style={styles.mapPlaceholder}>
        <Icon name="map-outline" size={64} color="#1b3a1b" />
        <Text style={styles.mapText}>
          {Platform.OS === 'web'
            ? 'Map view requires native build (react-native-maps).'
            : 'Tap a node below to inspect. A real build renders pins on a MapView here.'}
        </Text>
      </View>

      {/* Legend */}
      <View style={styles.legend}>
        {Object.entries(STATE_NAMES).map(([k, name]) => (
          <View key={k} style={styles.legendItem}>
            <View
              style={[styles.legendDot, { backgroundColor: STATE_COLORS[k] }]}
            />
            <Text style={styles.legendText}>{name}</Text>
          </View>
        ))}
      </View>

      {/* Site list */}
      <View style={styles.siteList}>
        {sites.map((site) => (
          <TouchableOpacity
            key={site.id}
            style={[
              styles.siteItem,
              selected === site.id && styles.siteItemSelected,
              { borderLeftColor: STATE_COLORS[site.state] },
            ]}
            onPress={() => setSelected(site.id)}
          >
            <View
              style={[styles.pin, { backgroundColor: STATE_COLORS[site.state] }]}
            />
            <View style={styles.siteInfo}>
              <Text style={styles.siteName}>{site.name}</Text>
              <Text style={styles.siteMeta}>
                {site.id} · {site.species}
              </Text>
              <Text style={styles.siteCoords}>
                {site.lat.toFixed(4)}, {site.lon.toFixed(4)}
              </Text>
            </View>
            <Text style={[styles.stateTag, { color: STATE_COLORS[site.state] }]}>
              {STATE_NAMES[site.state]}
            </Text>
          </TouchableOpacity>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#fafafa', padding: 12 },
  title: { fontSize: 20, fontWeight: 'bold', color: '#1b3a1b' },
  subtitle: { fontSize: 12, color: '#666', marginBottom: 8 },
  mapPlaceholder: {
    height: 160,
    backgroundColor: '#e8f5e9',
    borderRadius: 10,
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 8,
  },
  mapText: { color: '#555', fontSize: 12, marginTop: 8, textAlign: 'center', padding: 12 },
  legend: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    paddingVertical: 8,
  },
  legendItem: { flexDirection: 'row', alignItems: 'center', marginRight: 12, marginVertical: 4 },
  legendDot: { width: 12, height: 12, borderRadius: 6, marginRight: 4 },
  legendText: { fontSize: 11, color: '#333' },
  siteList: { marginTop: 8 },
  siteItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    backgroundColor: '#fff',
    borderRadius: 8,
    marginVertical: 4,
    borderLeftWidth: 4,
  },
  siteItemSelected: { backgroundColor: '#e8f5e9' },
  pin: { width: 14, height: 14, borderRadius: 7, marginRight: 10 },
  siteInfo: { flex: 1 },
  siteName: { fontSize: 14, fontWeight: 'bold', color: '#1b3a1b' },
  siteMeta: { fontSize: 11, color: '#666' },
  siteCoords: { fontSize: 10, color: '#aaa' },
  stateTag: { fontSize: 12, fontWeight: 'bold' },
});