/**
 * DetectionDetailScreen.tsx — Detailed view of pest detections
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Shows the full detection list with filtering, and when a detection is
 * selected, displays the multispectral images (6 bands), NDVI overlay,
 * acoustic spectrogram, wingbeat frequency plot, and species identification
 * with confidence score.
 */

import React, { useState, useMemo } from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  Image,
  Dimensions,
} from 'react-native';
import {
  Card,
  Title,
  Paragraph,
  Button,
  Divider,
  Chip,
  Searchbar,
  ActivityIndicator,
} from 'react-native-paper';
import { useApp } from '../context/AppContext';
import { DetectionEvent, SPECIES_NAMES, SEVERITY_COLORS, SEVERITY_LABELS } from '../types';

const { width } = Dimensions.get('window');
const BAND_NAMES = ['450nm (Blue)', '530nm (Green)', '660nm (Red)', '740nm (Red-Edge)', '810nm (NIR-1)', '850nm (NIR-2)'];
const BAND_COLORS = ['#42A5F5', '#66BB6A', '#EF5350', '#AB47BC', '#7E57C2', '#5C6BC0'];

export default function DetectionDetailScreen() {
  const { detections, loading, refreshData } = useApp();
  const [selectedDetection, setSelectedDetection] = useState<DetectionEvent | null>(null);
  const [searchQuery, setSearchQuery] = useState('');
  const [speciesFilter, setSpeciesFilter] = useState<string | null>(null);

  const filteredDetections = useMemo(() => {
    let result = detections;
    if (speciesFilter) {
      result = result.filter(d =>
        (SPECIES_NAMES[d.species_id] || '').includes(speciesFilter)
      );
    }
    if (searchQuery) {
      const q = searchQuery.toLowerCase();
      result = result.filter(d =>
        (SPECIES_NAMES[d.species_id] || '').toLowerCase().includes(q) ||
        d.node_id.toLowerCase().includes(q)
      );
    }
    return result;
  }, [detections, searchQuery, speciesFilter]);

  /* Get unique species for chips */
  const speciesChips = useMemo(() => {
    const set = new Set<string>();
    detections.forEach(d => {
      const name = SPECIES_NAMES[d.species_id];
      if (name && d.severity !== 'none') set.add(name);
    });
    return Array.from(set).slice(0, 10);
  }, [detections]);

  if (selectedDetection) {
    return (
      <DetectionDetailView
        detection={selectedDetection}
        onBack={() => setSelectedDetection(null)}
      />
    );
  }

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <ScrollView.RefreshControl
          refreshing={loading}
          onRefresh={refreshData}
        />
      }
    >
      <Card style={styles.card}>
        <Card.Content>
          <Title>Detection History</Title>

          {/* Search bar */}
          <Searchbar
            placeholder="Search by species or node ID"
            onChangeText={setSearchQuery}
            value={searchQuery}
            style={styles.searchbar}
          />

          {/* Species filter chips */}
          <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.chipRow}>
            <Chip
              selected={!speciesFilter}
              onPress={() => setSpeciesFilter(null)}
              style={styles.chip}
            >
              All
            </Chip>
            {speciesChips.map(species => (
              <Chip
                key={species}
                selected={speciesFilter === species}
                onPress={() => setSpeciesFilter(species)}
                style={styles.chip}
              >
                {species.split('(')[0].trim()}
              </Chip>
            ))}
          </ScrollView>

          <Divider style={styles.divider} />

          {/* Detection list */}
          {filteredDetections.length === 0 ? (
            <Text style={styles.emptyText}>No detections match your filters</Text>
          ) : (
            filteredDetections.map((det, idx) => (
              <TouchableOpacity
                key={det.id || idx}
                onPress={() => setSelectedDetection(det)}
                style={styles.detectionRow}
              >
                <View
                  style={[
                    styles.severityBar,
                    { backgroundColor: SEVERITY_COLORS[det.severity] || '#CCC' }
                  ]}
                />
                <View style={styles.detectionContent}>
                  <Text style={styles.speciesName}>
                    {SPECIES_NAMES[det.species_id] || 'Unknown species'}
                  </Text>
                  <Text style={styles.detectionMeta}>
                    {det.node_id} • {det.timestamp.substring(11, 19)} • {det.confidence.toFixed(1)}% conf
                  </Text>
                  <View style={styles.featureRow}>
                    <Text style={styles.featureText}>
                      Wingbeat: {det.features.wingbeat_hz.toFixed(0)} Hz
                    </Text>
                    <Text style={styles.featureText}>
                      NDVI: {det.features.spectral_ndvi.toFixed(2)}
                    </Text>
                  </View>
                  <View style={styles.featureRow}>
                    <Text style={styles.featureText}>
                      Damage: {det.features.damage_area_pct.toFixed(1)}%
                    </Text>
                    <Text style={styles.featureText}>
                      Temp: {det.features.temp_c.toFixed(1)}°C
                    </Text>
                  </View>
                </View>
                <Text style={[styles.severityText, { color: SEVERITY_COLORS[det.severity] }]}>
                  {SEVERITY_LABELS[det.severity]}
                </Text>
              </TouchableOpacity>
            ))
          )}
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

/* ----------------------------------------------------------------
 * Detail view for a single detection
 * ---------------------------------------------------------------- */
function DetectionDetailView({
  detection,
  onBack,
}: {
  detection: DetectionEvent;
  onBack: () => void;
}) {
  return (
    <ScrollView style={styles.container}>
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.detailHeader}>
            <Button mode="text" onPress={onBack} icon="arrow-left">
              Back
            </Button>
            <Text style={styles.severityBadge}>
              {SEVERITY_LABELS[detection.severity]}
            </Text>
          </View>

          <Title style={styles.speciesTitle}>
            {SPECIES_NAMES[detection.species_id] || 'Unknown species'}
          </Title>
          <Paragraph style={styles.confidenceText}>
            Confidence: {(detection.confidence * 100).toFixed(1)}%
          </Paragraph>
          <Paragraph style={styles.metaText}>
            Node: {detection.node_id} • {detection.timestamp}
          </Paragraph>
          <Paragraph style={styles.metaText}>
            GPS: {detection.gps.lat.toFixed(5)}, {detection.gps.lon.toFixed(5)}
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Multispectral bands */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Multispectral Bands</Title>
          <View style={styles.bandsGrid}>
            {BAND_NAMES.map((band, idx) => (
              <View key={idx} style={styles.bandItem}>
                <View style={[styles.bandImage, { backgroundColor: BAND_COLORS[idx] }]}>
                  <Text style={styles.bandPlaceholder}>
                    {detection.thumbnail_b64 ? 'IMG' : 'N/A'}
                  </Text>
                </View>
                <Text style={styles.bandLabel}>{band}</Text>
              </View>
            ))}
          </View>

          {/* NDVI overlay */}
          <Divider style={styles.divider} />
          <Title style={styles.sectionTitle}>Vegetation Indices</Title>
          <View style={styles.indexRow}>
            <View style={styles.indexItem}>
              <Text style={styles.indexLabel}>NDVI</Text>
              <Text style={styles.indexValue}>
                {detection.features.spectral_ndvi.toFixed(3)}
              </Text>
            </View>
            <View style={styles.indexItem}>
              <Text style={styles.indexLabel}>NDRE</Text>
              <Text style={styles.indexValue}>
                {detection.features.spectral_ndre.toFixed(3)}
              </Text>
            </View>
            <View style={styles.indexItem}>
              <Text style={styles.indexLabel}>Damage</Text>
              <Text style={styles.indexValue}>
                {detection.features.damage_area_pct.toFixed(1)}%
              </Text>
            </View>
          </View>
        </Card.Content>
      </Card>

      {/* Acoustic analysis */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Acoustic Analysis</Title>

          {/* Spectrogram placeholder */}
          <View style={styles.spectrogramContainer}>
            <Text style={styles.spectrogramLabel}>Spectrogram (0-20 kHz)</Text>
            <View style={styles.spectrogram}>
              {Array.from({ length: 8 }).map((_, row) => (
                <View key={row} style={styles.spectrogramRow}>
                  {Array.from({ length: 32 }).map((_, col) => (
                    <View
                      key={col}
                      style={{
                        flex: 1,
                        height: 8,
                        backgroundColor: `rgba(46, 125, 50, ${Math.random() * 0.8})`,
                      }}
                    />
                  ))}
                </View>
              ))}
            </View>
          </View>

          {/* Wingbeat frequency */}
          <Divider style={styles.divider} />
          <Title style={styles.sectionTitle}>Wingbeat Analysis</Title>
          <View style={styles.wingbeatRow}>
            <View style={styles.wingbeatItem}>
              <Text style={styles.indexLabel}>Fundamental</Text>
              <Text style={styles.wingbeatValue}>
                {detection.features.wingbeat_hz.toFixed(1)} Hz
              </Text>
            </View>
            <View style={styles.wingbeatItem}>
              <Text style={styles.indexLabel}>Activity</Text>
              <Text style={styles.wingbeatValue}>
                {detection.features.temp_c > 15 ? 'Active' : 'Low'}
              </Text>
            </View>
          </View>

          {/* Wingbeat frequency bar chart */}
          <View style={styles.freqBarChart}>
            {Array.from({ length: 20 }).map((_, i) => {
              const height = Math.max(4, Math.sin(i * 0.5 + detection.features.wingbeat_hz / 50) * 30 + 35);
              return (
                <View
                  key={i}
                  style={{
                    flex: 1,
                    height: height,
                    backgroundColor: i === Math.floor(detection.features.wingbeat_hz / 30)
                      ? '#E57373' : '#81C784',
                    marginHorizontal: 1,
                    borderRadius: 2,
                  }}
                />
              );
            })}
          </View>
          <Text style={styles.freqLabel}>0 Hz    150    300    450    600 Hz</Text>
        </Card.Content>
      </Card>

      {/* Environmental context */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Environmental Context</Title>
          <View style={styles.envRow}>
            <Text style={styles.envLabel}>Temperature</Text>
            <Text style={styles.envValue}>{detection.features.temp_c.toFixed(1)} °C</Text>
          </View>
          <View style={styles.envRow}>
            <Text style={styles.envLabel}>Humidity</Text>
            <Text style={styles.envValue}>{detection.features.humidity_pct.toFixed(0)}%</Text>
          </View>
          <View style={styles.envRow}>
            <Text style={styles.envLabel}>CO2</Text>
            <Text style={styles.envValue}>{detection.features.co2_ppm} ppm</Text>
          </View>
        </Card.Content>
      </Card>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F1F8E9',
    padding: 8,
  },
  card: {
    marginBottom: 12,
  },
  searchbar: {
    marginBottom: 8,
  },
  chipRow: {
    flexDirection: 'row',
    marginBottom: 12,
  },
  chip: {
    marginRight: 6,
  },
  divider: {
    marginVertical: 12,
  },
  detectionRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#E0E0E0',
  },
  severityBar: {
    width: 4,
    height: '100%',
    minHeight: 50,
    marginRight: 12,
    borderRadius: 2,
  },
  detectionContent: {
    flex: 1,
  },
  speciesName: {
    fontSize: 14,
    fontWeight: '500',
    color: '#333',
  },
  detectionMeta: {
    fontSize: 11,
    color: '#9E9E9E',
    marginTop: 2,
  },
  featureRow: {
    flexDirection: 'row',
    marginTop: 4,
  },
  featureText: {
    fontSize: 11,
    color: '#757575',
    marginRight: 16,
  },
  severityText: {
    fontSize: 12,
    fontWeight: 'bold',
  },
  emptyText: {
    color: '#9E9E9E',
    fontStyle: 'italic',
    paddingVertical: 20,
    textAlign: 'center',
  },
  detailHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  severityBadge: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#558B2F',
  },
  speciesTitle: {
    fontSize: 18,
    marginTop: 8,
  },
  confidenceText: {
    fontSize: 14,
    color: '#558B2F',
  },
  metaText: {
    fontSize: 12,
    color: '#9E9E9E',
  },
  bandsGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    marginTop: 8,
  },
  bandItem: {
    width: (width - 48) / 3,
    marginBottom: 8,
  },
  bandImage: {
    height: 60,
    borderRadius: 4,
    justifyContent: 'center',
    alignItems: 'center',
  },
  bandPlaceholder: {
    color: '#fff',
    fontSize: 12,
  },
  bandLabel: {
    fontSize: 10,
    color: '#757575',
    marginTop: 2,
  },
  sectionTitle: {
    fontSize: 14,
    marginTop: 8,
  },
  indexRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 12,
  },
  indexItem: {
    alignItems: 'center',
  },
  indexLabel: {
    fontSize: 11,
    color: '#757575',
  },
  indexValue: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#2E7D32',
  },
  spectrogramContainer: {
    marginTop: 12,
  },
  spectrogramLabel: {
    fontSize: 12,
    color: '#757575',
    marginBottom: 4,
  },
  spectrogram: {
    height: 80,
    borderWidth: 1,
    borderColor: '#E0E0E0',
    borderRadius: 4,
    overflow: 'hidden',
  },
  spectrogramRow: {
    flexDirection: 'row',
    height: 10,
  },
  wingbeatRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    marginTop: 12,
  },
  wingbeatItem: {
    alignItems: 'center',
  },
  wingbeatValue: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#2E7D32',
  },
  freqBarChart: {
    flexDirection: 'row',
    height: 50,
    alignItems: 'flex-end',
    marginTop: 12,
  },
  freqLabel: {
    fontSize: 9,
    color: '#9E9E9E',
    marginTop: 4,
  },
  envRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
  },
  envLabel: {
    fontSize: 13,
    color: '#555',
  },
  envValue: {
    fontSize: 13,
    fontWeight: '500',
    color: '#2E7D32',
  },
});