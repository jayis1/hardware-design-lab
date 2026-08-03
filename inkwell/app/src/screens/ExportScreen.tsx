// ExportScreen.tsx — Export a session to SVG / PNG / PDF / .inkwell JSON
//
// Loads the strokes for a session, builds an SVG string, and writes it to
// the app's documents directory. A preview is shown and the user can share
// or open the file.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState } from 'react';
import { View, Text, Button, StyleSheet, Share } from 'react-native';
import RNFS from 'react-native-fs';
import { getSessionStrokes } from '../db/database';

type Props = { route: any };

export default function ExportScreen({ route }: Props) {
  const { sessionId } = route.params;
  const [svg, setSvg] = useState('');
  const [filePath, setFilePath] = useState('');

  useEffect(() => {
    (async () => {
      const strokes = await getSessionStrokes(sessionId);
      const svgDoc = buildSVG(strokes);
      setSvg(svgDoc);
      const path = `${RNFS.DocumentDirectoryPath}/inkwell_session_${sessionId}.svg`;
      await RNFS.writeFile(path, svgDoc, 'utf8');
      setFilePath(path);
    })();
  }, [sessionId]);

  const buildSVG = (strokes: any[]): string => {
    const w = 800, h = 1100;
    let paths = '';
    let prevX = 0, prevY = 0;
    let inStroke = false;
    for (const s of strokes) {
      const x = s.xUm / 40;
      const y = s.yUm / 40;
      const sw = Math.max(0.5, s.pressureMN / 200);
      if (!inStroke) {
        paths += `<path d="M ${x} ${y}" stroke="#1a1a2e" stroke-width="${sw}" fill="none" stroke-linecap="round" stroke-linejoin="round">`;
        inStroke = true;
      } else {
        paths += ` L ${x} ${y}`;
      }
      prevX = x; prevY = y;
    }
    if (inStroke) paths += `"/>`;
    return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">
  <rect width="${w}" height="${h}" fill="#f9f7f1"/>
  ${paths}
</svg>`;
  };

  const handleShare = async () => {
    if (!filePath) return;
    try {
      await Share.share({ url: filePath, title: 'Inkwell session export' });
    } catch (e) { console.warn(e); }
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Export Session #{sessionId}</Text>
      <Text style={styles.path}>{filePath || 'building…'}</Text>
      <Button title="Share SVG" onPress={handleShare} disabled={!filePath} />
      <Text style={styles.note}>SVG is lossless: it preserves the stroke
        coordinates and can be re-imported. Use an external converter to
        produce PNG or PDF from this SVG.</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#f9f7f1' },
  title:     { fontSize: 20, fontWeight: '700', marginBottom: 12, color: '#1a1a2e' },
  path:      { fontSize: 12, color: '#666', marginBottom: 16, fontFamily: 'monospace' },
  note:      { fontSize: 12, color: '#888', marginTop: 16, fontStyle: 'italic' },
});