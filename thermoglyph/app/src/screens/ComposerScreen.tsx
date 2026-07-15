/**
 * ComposerScreen — Glyph composer screen wrapper
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: MIT
 */

import React from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { GlyphComposer } from '../components/GlyphComposer';
import { ThermalPreview } from '../components/ThermalPreview';
import { useDeviceStore } from '../store/deviceStore';

export const ComposerScreen: React.FC = () => {
  const { thermalFrame, templates, sendGlyph, deleteTemplate } = useDeviceStore();

  return (
    <ScrollView style={styles.screen}>
      <View style={styles.header}>
        <Text style={styles.title}>Glyph Composer</Text>
        <Text style={styles.subtitle}>Design custom thermal patterns</Text>
      </View>

      <ThermalPreview frame={thermalFrame} />
      <GlyphComposer />

      <Text style={styles.sectionTitle}>Saved Templates</Text>
      {templates.map(tpl => (
        <View key={tpl.id} style={styles.templateRow}>
          <TouchableOpacity
            style={styles.templateInfo}
            onPress={() => sendGlyph(tpl.command)}
          >
            <Text style={styles.templateName}>{tpl.name}</Text>
            <Text style={styles.templateMeta}>
              {tpl.command.type} · {tpl.command.polarity} · by {tpl.createdBy}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.deleteBtn}
            onPress={() => deleteTemplate(tpl.id)}
          >
            <Text style={styles.deleteText}>Delete</Text>
          </TouchableOpacity>
        </View>
      ))}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  screen: { flex: 1, backgroundColor: '#0d0d1a' },
  header: { padding: 20, alignItems: 'center' },
  title: { color: '#fff', fontSize: 24, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 13, marginTop: 4 },
  sectionTitle: { color: '#ccc', fontSize: 15, fontWeight: '600', paddingHorizontal: 16, marginTop: 16, marginBottom: 8 },
  templateRow: { flexDirection: 'row', alignItems: 'center', backgroundColor: '#1a1a2e', borderRadius: 10, padding: 14, marginHorizontal: 8, marginVertical: 3 },
  templateInfo: { flex: 1 },
  templateName: { color: '#fff', fontSize: 15, fontWeight: '500' },
  templateMeta: { color: '#888', fontSize: 12, marginTop: 2 },
  deleteBtn: { paddingHorizontal: 12, paddingVertical: 6, backgroundColor: '#422', borderRadius: 6 },
  deleteText: { color: '#a66', fontSize: 12 },
});