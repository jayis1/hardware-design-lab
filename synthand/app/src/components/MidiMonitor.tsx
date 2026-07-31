/**
 * MidiMonitor.tsx — Scrolling MIDI event log component.
 *
 * Displays the most recent MIDI events received from the Synthand glove,
 * formatted as human-readable text (e.g., "Note On C4 vel 87").
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useEffect, useRef } from 'react';
import { View, StyleSheet, Text, ScrollView } from 'react-native';
import { MidiEvent, MIDI_STATUS_NOTE_ON, MIDI_STATUS_NOTE_OFF, MIDI_STATUS_CC, MIDI_STATUS_PITCH_BEND, MIDI_STATUS_PROGRAM_CHANGE, MIDI_STATUS_CHANNEL_PRESSURE } from '../ble/protocol';

interface MidiMonitorProps {
  events: MidiEvent[];
  maxLines?: number;
}

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];

function midiNoteToName(note: number): string {
  const octave = Math.floor(note / 12) - 1;
  const name = NOTE_NAMES[note % 12];
  return `${name}${octave}`;
}

function formatMidiEvent(evt: MidiEvent): string {
  const time = (evt.timestamp / 1000).toFixed(2);
  switch (evt.status) {
    case MIDI_STATUS_NOTE_ON:
      return `[${time}s] Note On  ${midiNoteToName(evt.data1)} ch${evt.channel + 1} vel ${evt.data2}`;
    case MIDI_STATUS_NOTE_OFF:
      return `[${time}s] Note Off ${midiNoteToName(evt.data1)} ch${evt.channel + 1}`;
    case MIDI_STATUS_CC:
      return `[${time}s] CC ${evt.data1} = ${evt.data2} ch${evt.channel + 1}`;
    case MIDI_STATUS_PITCH_BEND:
      const pb = (evt.data2 << 7) | evt.data1;
      return `[${time}s] Pitch Bend ${pb - 8192} ch${evt.channel + 1}`;
    case MIDI_STATUS_PROGRAM_CHANGE:
      return `[${time}s] Program ${evt.data1 + 1} ch${evt.channel + 1}`;
    case MIDI_STATUS_CHANNEL_PRESSURE:
      return `[${time}s] Ch Pressure ${evt.data2} ch${evt.channel + 1}`;
    default:
      return `[${time}s] Unknown 0x${evt.status.toString(16)}`;
  }
}

/**
 * MidiMonitor — scrolling log of MIDI events.
 * Author: jayis1
 */
export default function MidiMonitor({ events, maxLines = 50 }: MidiMonitorProps) {
  const scrollViewRef = useRef<ScrollView>(null);
  const recentEvents = events.slice(-maxLines);

  useEffect(() => {
    // Auto-scroll to bottom on new events
    scrollViewRef.current?.scrollToEnd({ animated: true });
  }, [events]);

  return (
    <View style={styles.container}>
      <Text style={styles.title}>MIDI Event Log</Text>
      <ScrollView
        ref={scrollViewRef}
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
      >
        {recentEvents.length === 0 ? (
          <Text style={styles.empty}>No MIDI events yet...</Text>
        ) : (
          recentEvents.map((evt, i) => (
            <Text key={i} style={styles.eventLine}>
              {formatMidiEvent(evt)}
            </Text>
          ))
        )}
      </ScrollView>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#0f0f23',
    borderRadius: 12,
    padding: 12,
    marginVertical: 8,
    maxHeight: 200,
  },
  title: {
    color: '#e94560',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  scroll: {
    flex: 1,
  },
  scrollContent: {
    paddingBottom: 8,
  },
  eventLine: {
    color: '#0f0',
    fontFamily: 'monospace',
    fontSize: 11,
    paddingVertical: 1,
  },
  empty: {
    color: '#555',
    fontSize: 12,
    fontStyle: 'italic',
  },
});