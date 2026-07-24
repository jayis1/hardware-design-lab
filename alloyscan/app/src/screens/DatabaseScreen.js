/**
 * DatabaseScreen.js — Browse the alloy reference database
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

import React, { useState, useMemo } from 'react';
import {
  View, Text, StyleSheet, FlatList, TextInput, TouchableOpacity,
} from 'react-native';
import { alloyDatabase } from '../data/alloyDatabase';
import Colors from '../constants/Colors';

const familyNames = [
  'All', 'Carbon Steel', 'Alloy Steel', 'SS Austenitic',
  'SS Ferritic', 'SS Duplex', 'Aluminum', 'Copper',
  'Titanium', 'Nickel', 'Zn/Mg/Other', 'Other',
];

const familyColors = {
  'Carbon Steel': Colors.family.Carbon,
  'Alloy Steel': Colors.family.Alloy,
  'SS Austenitic': Colors.family.Austenitic,
  'SS Ferritic': Colors.family.Ferritic,
  'SS Duplex': Colors.family.Duplex,
  'Aluminum': Colors.family.Aluminum,
  'Copper': Colors.family.Copper,
  'Titanium': Colors.family.Titanium,
  'Nickel': Colors.family.Nickel,
  'Zn/Mg/Other': Colors.family.Zinc,
  'Other': Colors.family.Other,
};

const DatabaseScreen = () => {
  const [search, setSearch] = useState('');
  const [familyFilter, setFamilyFilter] = useState('All');
  const [selectedAlloy, setSelectedAlloy] = useState(null);

  const filteredAlloys = useMemo(() => {
    let result = alloyDatabase;

    if (familyFilter !== 'All') {
      result = result.filter(a => a.family === familyFilter);
    }

    if (search.trim()) {
      const q = search.toLowerCase().trim();
      result = result.filter(a =>
        a.name.toLowerCase().includes(q) ||
        a.family.toLowerCase().includes(q)
      );
    }

    return result;
  }, [search, familyFilter]);

  const renderAlloyItem = ({ item }) => (
    <TouchableOpacity
      style={styles.alloyItem}
      onPress={() => setSelectedAlloy(item)}
    >
      <View style={[styles.familyIndicator, { backgroundColor: familyColors[item.family] || Colors.gray }]} />
      <View style={styles.alloyInfo}>
        <Text style={styles.alloyName}>{item.name}</Text>
        <Text style={styles.alloyDetail}>
          {item.conductivity_iacs}% IACS · μᵣ={item.permeability_rel} · {item.density_gcm3} g/cm³
        </Text>
      </View>
      <Text style={styles.familyText}>{item.family}</Text>
    </TouchableOpacity>
  );

  if (selectedAlloy) {
    return (
      <View style={styles.container}>
        <View style={styles.detailHeader}>
          <TouchableOpacity onPress={() => setSelectedAlloy(null)}>
            <Text style={styles.backButton}>← Back</Text>
          </TouchableOpacity>
          <Text style={styles.detailTitle}>{selectedAlloy.name}</Text>
        </View>

        <View style={styles.detailContent}>
          <View style={[styles.detailBadge, { backgroundColor: familyColors[selectedAlloy.family] || Colors.gray }]}>
            <Text style={styles.detailBadgeText}>{selectedAlloy.family}</Text>
          </View>

          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Conductivity</Text>
            <Text style={styles.detailValue}>{selectedAlloy.conductivity_iacs}% IACS</Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Permeability</Text>
            <Text style={styles.detailValue}>μᵣ = {selectedAlloy.permeability_rel}</Text>
          </View>
          <View style={styles.detailRow}>
            <Text style={styles.detailLabel}>Density</Text>
            <Text style={styles.detailValue}>{selectedAlloy.density_gcm3} g/cm³</Text>
          </View>

          <View style={styles.featureSection}>
            <Text style={styles.sectionTitle}>Electromagnetic Signature (I/Q at 4 frequencies)</Text>
            <View style={styles.featureGrid}>
              {['1 kHz', '10 kHz', '100 kHz', '500 kHz'].map((freq, i) => (
                <View key={freq} style={styles.featureRow}>
                  <Text style={styles.freqLabel}>{freq}</Text>
                  <Text style={styles.freqValue}>
                    I={selectedAlloy.feature[i*2].toFixed(3)}, Q={selectedAlloy.feature[i*2+1].toFixed(3)}
                  </Text>
                </View>
              ))}
            </View>
          </View>

          <View style={styles.applicationSection}>
            <Text style={styles.sectionTitle}>Common Applications</Text>
            <Text style={styles.appText}>{selectedAlloy.applications || 'General engineering'}</Text>
          </View>

          <View style={styles.misidSection}>
            <Text style={styles.sectionTitle}>Common Misidentifications</Text>
            <Text style={styles.misidText}>
              {selectedAlloy.misid || 'May be confused with similar alloys in the same family. Check conductivity and permeability values for differentiation.'}
            </Text>
          </View>
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      <View style={styles.searchBar}>
        <TextInput
          style={styles.searchInput}
          placeholder="Search alloys..."
          placeholderTextColor={Colors.gray}
          value={search}
          onChangeText={setSearch}
        />
      </View>

      <View style={styles.filterRow}>
        <FlatList
          horizontal
          data={familyNames}
          keyExtractor={item => item}
          renderItem={({ item }) => (
            <TouchableOpacity
              style={[
                styles.filterChip,
                familyFilter === item && { backgroundColor: Colors.accent },
              ]}
              onPress={() => setFamilyFilter(item)}
            >
              <Text style={[
                styles.filterChipText,
                familyFilter === item && { color: Colors.white },
              ]}>{item}</Text>
            </TouchableOpacity>
          )}
          showsHorizontalScrollIndicator={false}
        />
      </View>

      <Text style={styles.countText}>{filteredAlloys.length} alloys</Text>

      <FlatList
        data={filteredAlloys}
        keyExtractor={item => item.name}
        renderItem={renderAlloyItem}
        contentContainerStyle={styles.list}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: Colors.dark },
  searchBar: { padding: 12 },
  searchInput: {
    backgroundColor: Colors.darkGray,
    color: Colors.white,
    borderRadius: 10,
    paddingHorizontal: 16,
    paddingVertical: 10,
    fontSize: 16,
  },
  filterRow: { paddingHorizontal: 12, paddingBottom: 8 },
  filterChip: {
    paddingHorizontal: 14,
    paddingVertical: 6,
    borderRadius: 16,
    marginRight: 8,
    backgroundColor: Colors.darkGray,
  },
  filterChipText: { color: Colors.gray, fontSize: 13 },
  countText: { color: Colors.gray, fontSize: 12, paddingHorizontal: 16, paddingBottom: 4 },
  list: { paddingHorizontal: 12, paddingBottom: 20 },
  alloyItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: Colors.darkGray,
    borderRadius: 10,
    padding: 12,
    marginBottom: 6,
  },
  familyIndicator: { width: 4, height: 40, borderRadius: 2, marginRight: 12 },
  alloyInfo: { flex: 1 },
  alloyName: { color: Colors.white, fontSize: 16, fontWeight: 'bold' },
  alloyDetail: { color: Colors.gray, fontSize: 12, marginTop: 2 },
  familyText: { color: Colors.gray, fontSize: 11 },
  detailHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 12,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  backButton: { color: Colors.accent, fontSize: 16, marginRight: 16 },
  detailTitle: { color: Colors.white, fontSize: 20, fontWeight: 'bold' },
  detailContent: { padding: 16 },
  detailBadge: {
    alignSelf: 'flex-start',
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 12,
    marginBottom: 16,
  },
  detailBadgeText: { color: Colors.white, fontWeight: 'bold', fontSize: 13 },
  detailRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: Colors.darkGray,
  },
  detailLabel: { color: Colors.gray, fontSize: 14 },
  detailValue: { color: Colors.white, fontSize: 14, fontWeight: 'bold' },
  featureSection: { marginTop: 20 },
  sectionTitle: { color: Colors.lightGray, fontSize: 14, fontWeight: 'bold', marginBottom: 8 },
  featureGrid: { backgroundColor: Colors.darkGray, borderRadius: 8, padding: 12 },
  featureRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 4,
  },
  freqLabel: { color: Colors.cyan, fontSize: 13 },
  freqValue: { color: Colors.white, fontSize: 13, fontFamily: 'monospace' },
  applicationSection: { marginTop: 20 },
  appText: { color: Colors.lightGray, fontSize: 14, lineHeight: 20 },
  misidSection: { marginTop: 20 },
  misidText: { color: Colors.lightGray, fontSize: 14, lineHeight: 20 },
});

export default DatabaseScreen;