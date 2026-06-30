/**
 * MapScreen.tsx — Interactive pest pressure heatmap map
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, ActivityIndicator, Picker } from 'react-native';
import { Card, Title, Button, SegmentedButtons } from 'react-native-paper';
import { useApp } from '../context/AppContext';
import { SEVERITY_COLORS, SPECIES_NAMES } from '../types';

export default function MapScreen() {
  const { api, nodes, detections, loading, refreshData } = useApp();
  const [heatmap, setHeatmap] = useState<import('../types').PestPressureGrid | null>(null);
  const [timeRange, setTimeRange] = useState('24h');
  const [selectedSpecies, setSelectedSpecies] = useState('');
  const [mapError, setMapError] = useState<string | null>(null);

  useEffect(() => {
    let mounted = true;
    const loadHeatmap = async () => {
      try {
        setMapError(null);
        const grid = await api.getHeatmap(
          selectedSpecies || undefined,
          new Date()
        );
        if (mounted) setHeatmap(grid);
      } catch (err) {
        if (mounted) {
          setMapError(err instanceof Error ? err.message : 'Failed to load heatmap');
          setHeatmap(null);
        }
      }
    };
    loadHeatmap();
    const interval = setInterval(loadHeatmap, 60000); /* 1 min refresh */
    return () => { mounted = false; clearInterval(interval); };
  }, [api, selectedSpecies, timeRange]);

  /* Unique species from detections for filter dropdown */
  const speciesList = Array.from(
    new Set(detections.map(d => SPECIES_NAMES[d.species_id] || 'Unknown'))
  );

  /* Compute heatmap from detections (fallback if API not available) */
  const computeSimpleHeatmap = () => {
    const cells = new Map<string, { lat: number; lon: number; count: number; species: string }>();
    const gridSize = 0.001; /* ~100m grid */

    detections.forEach(det => {
      const latKey = Math.floor(det.gps.lat / gridSize) * gridSize;
      const lonKey = Math.floor(det.gps.lon / gridSize) * gridSize;
      const key = `${latKey},${lonKey}`;
      const existing = cells.get(key);
      if (existing) {
        existing.count++;
      } else {
        cells.set(key, {
          lat: latKey,
          lon: lonKey,
          count: 1,
          species: SPECIES_NAMES[det.species_id] || 'Unknown',
        });
      }
    });

    return Array.from(cells.values()).map(c => ({
      lat: c.lat,
      lon: c.lon,
      pressure: Math.min(c.count / 10, 1),
      dominant_species: c.species,
      detection_count: c.count,
    }));
  };

  const heatmapCells = heatmap?.cells || computeSimpleHeatmap();

  /* Find bounds for rendering */
  const lats = nodes.map(n => n.gps.lat).filter(l => l !== 0);
  const lons = nodes.map(n => n.gps.lon).filter(l => l !== 0);
  const centerLat = lats.length ? lats.reduce((a, b) => a + b) / lats.length : 45.5;
  const centerLon = lons.length ? lons.reduce((a, b) => a + b) / lons.length : -122.6;

  return (
    <View style={styles.container}>
      {/* Filter controls */}
      <Card style={styles.filterCard}>
        <Card.Content>
          <View style={styles.filterRow}>
            <Text style={styles.filterLabel}>Time Range:</Text>
            <SegmentedButtons
              value={timeRange}
              onValueChange={setTimeRange}
              buttons={[
                { value: '1h', label: '1h' },
                { value: '24h', label: '24h' },
                { value: '7d', label: '7d' },
                { value: '30d', label: '30d' },
              ]}
            />
          </View>
          <View style={styles.filterRow}>
            <Text style={styles.filterLabel}>Species:</Text>
            <Picker
              selectedValue={selectedSpecies}
              style={styles.picker}
              onValueChange={setSelectedSpecies}
            >
              <Picker.Item label="All species" value="" />
              {speciesList.map(species => (
                <Picker.Item key={species} label={species} value={species} />
              ))}
            </Picker>
          </View>
        </Card.Content>
      </Card>

      {/* Map visualization (text-based fallback) */}
      <Card style={styles.mapCard}>
        <Card.Content>
          <Title>Pest Pressure Map</Title>
          {loading && <ActivityIndicator animating />}
          {mapError && <Text style={styles.errorText}>⚠ {mapError}</Text>}

          <Text style={styles.mapInfo}>
            Center: {centerLat.toFixed(4)}°, {centerLon.toFixed(4)}°
          </Text>

          {/* Simple visualization: list grid cells by pressure */}
          <View style={styles.gridContainer}>
            {heatmapCells
              .sort((a, b) => b.pressure - a.pressure)
              .slice(0, 20)
              .map((cell, idx) => {
                const color = cell.pressure > 0.7 ? '#E57373' :
                              cell.pressure > 0.4 ? '#FFB74D' :
                              cell.pressure > 0.1 ? '#FFF176' : '#81C784';
                return (
                  <View key={idx} style={[styles.gridCell, { backgroundColor: color }]}>
                    <Text style={styles.gridCellText}>
                      {cell.lat.toFixed(3)}, {cell.lon.toFixed(3)}
                    </Text>
                    <Text style={styles.gridCellCount}>
                      {cell.detection_count} detections
                    </Text>
                    <Text style={styles.gridCellSpecies}>
                      {cell.dominant_species}
                    </Text>
                  </View>
                );
              })}
          </View>

          {/* Node markers */}
          <View style={styles.nodeMarkersContainer}>
            <Title style={styles.nodeMarkersTitle}>Active Nodes</Title>
            {nodes.map((node, idx) => (
              <View key={idx} style={styles.nodeMarkerRow}>
                <View
                  style={[
                    styles.nodeDot,
                    { backgroundColor: node.battery_pct > 50 ? '#4CAF50' :
                      node.battery_pct > 20 ? '#FFB74D' : '#E57373' }
                  ]}
                />
                <Text style={styles.nodeMarkerText}>
                  {node.label || `Node ${node.node_addr}`} — {node.gps.lat.toFixed(3)}°, {node.gps.lon.toFixed(3)}°
                </Text>
                <Text style={styles.nodeBattery}>{node.battery_pct}%</Text>
              </View>
            ))}
          </View>
        </Card.Content>
      </Card>

      {/* Legend */}
      <Card style={styles.legendCard}>
        <Card.Content>
          <Title style={styles.legendTitle}>Pressure Legend</Title>
          <View style={styles.legendRow}>
            <View style={[styles.legendDot, { backgroundColor: '#81C784' }]} />
            <Text style={styles.legendText}>None (0-10%)</Text>
          </View>
          <View style={styles.legendRow}>
            <View style={[styles.legendDot, { backgroundColor: '#FFF176' }]} />
            <Text style={styles.legendText}>Low (10-40%)</Text>
          </View>
          <View style={styles.legendRow}>
            <View style={[styles.legendDot, { backgroundColor: '#FFB74D' }]} />
            <Text style={styles.legendText}>Medium (40-70%)</Text>
          </View>
          <View style={styles.legendRow}>
            <View style={[styles.legendDot, { backgroundColor: '#E57373' }]} />
            <Text style={styles.legendText}>High (70-100%)</Text>
          </View>
        </Card.Content>
      </Card>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F1F8E9',
    padding: 8,
  },
  filterCard: {
    marginBottom: 8,
  },
  filterRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 8,
  },
  filterLabel: {
    fontSize: 13,
    fontWeight: '500',
    marginRight: 12,
    minWidth: 80,
  },
  picker: {
    flex: 1,
    height: 40,
  },
  mapCard: {
    marginBottom: 8,
  },
  mapInfo: {
    fontSize: 12,
    color: '#757575',
    marginBottom: 8,
  },
  gridContainer: {
    marginTop: 8,
  },
  gridCell: {
    padding: 10,
    marginBottom: 6,
    borderRadius: 8,
  },
  gridCellText: {
    fontSize: 13,
    fontWeight: '600',
    color: '#333',
  },
  gridCellCount: {
    fontSize: 12,
    color: '#555',
  },
  gridCellSpecies: {
    fontSize: 11,
    color: '#666',
    fontStyle: 'italic',
  },
  nodeMarkersContainer: {
    marginTop: 16,
  },
  nodeMarkersTitle: {
    fontSize: 14,
  },
  nodeMarkerRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  nodeDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 8,
  },
  nodeMarkerText: {
    flex: 1,
    fontSize: 12,
  },
  nodeBattery: {
    fontSize: 12,
    color: '#558B2F',
  },
  errorText: {
    color: '#D32F2F',
  },
  legendCard: {
    marginBottom: 8,
  },
  legendTitle: {
    fontSize: 14,
    marginBottom: 8,
  },
  legendRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 3,
  },
  legendDot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 8,
  },
  legendText: {
    fontSize: 12,
  },
});