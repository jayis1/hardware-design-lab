/*
 * src/screens/AuditLog.tsx — Chronological, filterable audit log for a Dewar.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useEffect, useState, useMemo } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity } from 'react-native';
import { Card, IconButton, SegmentedButtons, Menu, Button } from 'react-native-paper';
import { useRoute } from '@react-navigation/native';
import { AuditRecord } from '../types';
import { getAuditLog } from '../cloud/Api';

const EVENT_LABELS: Record<string, string> = {
  BOOT: 'System boot',
  STATE: 'State transition',
  LID_OPEN: 'Lid opened',
  LID_CLOSE: 'Lid closed',
  ENC_OPEN: 'Enclosure opened',
  ENC_CLOSE: 'Enclosure closed',
  MAINS_LOST: 'Mains power lost',
  MAINS_BACK: 'Mains power restored',
  ALARM_SEND: 'Alarm sent',
  ALARM_ACK: 'Alarm acknowledged',
  CAL_START: 'Calibration started',
  CAL_DONE: 'Calibration completed',
  HEARTBEAT: 'Heartbeat',
  FAULT: 'Fault detected',
};

const EVENT_COLORS: Record<string, string> = {
  ALARM_SEND: '#DC2626',
  ALARM_ACK: '#16A34A',
  MAINS_LOST: '#EAB308',
  ENC_OPEN: '#EA580C',
  LID_OPEN: '#EA580C',
  FAULT: '#DC2626',
  STATE: '#0066CC',
};

export default function AuditLog() {
  const route = useRoute<any>();
  const dewarId = route.params.dewarId;
  const [records, setRecords] = useState<AuditRecord[]>([]);
  const [filter, setFilter] = useState<'all' | 'alarms' | 'power' | 'lid'>('all');
  const [menuOpen, setMenuOpen] = useState(false);

  useEffect(() => {
    getAuditLog(dewarId).then(setRecords).catch(console.warn);
  }, [dewarId]);

  const filtered = useMemo(() => {
    switch (filter) {
      case 'alarms': return records.filter((r) =>
        r.event.startsWith('ALARM') || r.event === 'STATE');
      case 'power':  return records.filter((r) =>
        r.event.startsWith('MAINS'));
      case 'lid':    return records.filter((r) =>
        r.event.startsWith('LID'));
      default:       return records;
    }
  }, [records, filter]);

  return (
    <View style={s.wrap}>
      <Text style={s.h2}>Audit Log — {dewarId}</Text>
      <Text style={s.help}>
        Tamper-evident event log stored in on-board FRAM and mirrored to cloud.
        Required for ISO 20387 and CAP/CLAB audits.
      </Text>

      <SegmentedButtons
        value={filter}
        onValueChange={(v) => setFilter(v as any)}
        buttons={[
          { value: 'all',    label: 'All' },
          { value: 'alarms', label: 'Alarms' },
          { value: 'power',  label: 'Power' },
          { value: 'lid',    label: 'Lid' },
        ]}
        style={s.filter}
      />

      <FlatList
        data={filtered}
        keyExtractor={(r) => String(r.seq)}
        renderItem={({ item }) => <LogRow r={item} />}
      />

      <View style={s.exportRow}>
        <Menu visible={menuOpen} onDismiss={() => setMenuOpen(false)}
          anchor={<Button mode="outlined" icon="download"
            onPress={() => setMenuOpen(true)}>Export</Button>}>
          <Menu.Item onPress={() => exportRecords(filtered, 'csv')}
            title="Export as CSV" />
          <Menu.Item onPress={() => exportRecords(filtered, 'pdf')}
            title="Export as PDF" />
        </Menu>
      </View>
    </View>
  );
}

function LogRow({ r }: { r: AuditRecord }) {
  const label = EVENT_LABELS[r.event] ?? r.event;
  const color = EVENT_COLORS[r.event] ?? '#94A3B8';
  const time = new Date(r.epochMin * 60000).toLocaleString();
  return (
    <Card style={[s.card, { borderLeftColor: color, borderLeftWidth: 4 }]}>
      <Card.Title
        title={label}
        subtitle={`#${r.seq}  •  ${time}  •  aux=0x${r.aux.toString(16)}`}
        left={(p) => <IconButton {...p} icon="calendar-clock" color={color} />}
      />
    </Card>
  );
}

/* Export stubs — in the production app these use expo-sharing + expo-file-system
 * to write a real file and hand off to the OS share sheet. */
function exportRecords(records: AuditRecord[], fmt: 'csv' | 'pdf') {
  console.log(`export ${records.length} records as ${fmt}`);
  // csv: seq,epoch,event,aux joined with \n
  // pdf: a simple table render via react-native-pdf-lib
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 16 },
  h2: { color: '#F1F5F9', fontSize: 20, fontWeight: 'bold', marginBottom: 8 },
  help: { color: '#94A3B8', fontSize: 12, marginBottom: 16 },
  filter: { marginBottom: 12 },
  card: { backgroundColor: '#1E293B', marginBottom: 6, borderRadius: 6 },
  exportRow: { flexDirection: 'row', justifyContent: 'flex-end', marginTop: 8 },
});