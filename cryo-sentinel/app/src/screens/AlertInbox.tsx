/*
 * src/screens/AlertInbox.tsx — Active alarms requiring acknowledgement.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useEffect, useState, useCallback } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Modal, TextInput, Alert } from 'react-native';
import { Card, IconButton, Button, Badge } from 'react-native-paper';
import { DewarReading, STATE_COLOR } from '../types';
import { getFleet, ackAlarm, openAlarmStream } from '../cloud/Api';

interface ActiveAlarm {
  dewar: DewarReading;
  receivedAt: number;
}

export default function AlertInbox() {
  const [alarms, setAlarms] = useState<ActiveAlarm[]>([]);
  const [ackTarget, setAckTarget] = useState<DewarReading | null>(null);
  const [techId, setTechId] = useState('');
  const [note, setNote] = useState('');

  const load = useCallback(async () => {
    const fleet = await getFleet();
    const active = fleet
      .filter((d) => d.state === 'WARN' || d.state === 'CRITICAL')
      .map((d) => ({ dewar: d, receivedAt: Date.now() }));
    setAlarms(active);
  }, []);

  useEffect(() => {
    load();
    const close = openAlarmStream(() => load());
    return close;
  }, [load]);

  const doAck = async () => {
    if (!ackTarget) return;
    if (!techId) { Alert.alert('Technician ID required'); return; }
    await ackAlarm(ackTarget.dewarId, techId, note);
    setAlarms((prev) => prev.filter((a) => a.dewar.dewarId !== ackTarget.dewarId));
    setAckTarget(null);
    setTechId('');
    setNote('');
  };

  const sorted = [...alarms].sort((a, b) => {
    const ra = a.dewar.state === 'CRITICAL' ? 0 : 1;
    const rb = b.dewar.state === 'CRITICAL' ? 0 : 1;
    return ra - rb;
  });

  return (
    <View style={s.wrap}>
      <View style={s.header}>
        <Text style={s.h2}>{alarms.length} active alarm{alarms.length !== 1 ? 's' : ''}</Text>
        <Badge size={24} style={s.badge}>{alarms.length}</Badge>
      </View>
      <Text style={s.help}>
        Acknowledging an alarm stops the escalation chain for this event.
        The underlying invariant must also return to tolerance before the
        Dewar clears.
      </Text>

      <FlatList
        data={sorted}
        keyExtractor={(a) => a.dewar.dewarId}
        ListEmptyComponent={
          <View style={s.empty}>
            <IconButton icon="check-circle" size={56} color="#16A34A" />
            <Text style={s.emptyText}>No active alarms. All Dewars OK.</Text>
          </View>
        }
        renderItem={({ item }) => (
          <AlarmCard a={item} onAck={() => setAckTarget(item.dewar)} />
        )}
      />

      {/* Acknowledgement modal */}
      <Modal visible={!!ackTarget} transparent animationType="slide"
        onRequestClose={() => setAckTarget(null)}>
        <View style={s.modalWrap}>
          <Card style={s.modalCard}>
            <Card.Title title={`Acknowledge ${ackTarget?.dewarId ?? ''}`}
              subtitle={ackTarget?.dewar.reason.replace(/_/g, ' ')} />
            <Card.Content>
              <TextInput label="Your technician ID" value={techId}
                onChangeText={setTechId} style={s.input} />
              <TextInput label="Note (optional)" value={note}
                onChangeText={setNote} style={s.input}
                multiline numberOfLines={3} />
              <View style={s.modalBtns}>
                <Button mode="outlined" onPress={() => setAckTarget(null)}>Cancel</Button>
                <Button mode="contained" onPress={doAck}>Acknowledge</Button>
              </View>
            </Card.Content>
          </Card>
        </View>
      </Modal>
    </View>
  );
}

function AlarmCard({ a, onAck }: { a: ActiveAlarm; onAck: () => void }) {
  const color = STATE_COLOR[a.dewar.state];
  const mins = Math.round((Date.now() - a.receivedAt) / 60000);
  return (
    <Card style={[s.card, { borderLeftColor: color, borderLeftWidth: 6 }]}>
      <Card.Title
        title={a.dewar.label || a.dewar.dewarId}
        subtitle={`${a.dewar.state} — ${a.dewar.reason.replace(/_/g, ' ')}  •  ${mins} min ago`}
        left={(p) => <IconButton {...p} icon="alert-octagon" color={color} />}
        right={(p) => (
          <TouchableOpacity onPress={onAck}>
            <Button {...p} mode="contained" color={color}>Ack</Button>
          </TouchableOpacity>
        )}
      />
      <Card.Content>
        <View style={s.metricRow}>
          <Text style={s.metric}>LN2: {a.dewar.levelPct.toFixed(1)}%</Text>
          <Text style={s.metric}>Rate: {a.dewar.levelRatePerHour.toFixed(2)}%/h</Text>
          <Text style={s.metric}>Tilt: {a.dewar.tiltDeg.toFixed(1)}°</Text>
        </View>
      </Card.Content>
    </Card>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 16 },
  header: { flexDirection: 'row', alignItems: 'center', marginBottom: 8 },
  h2: { color: '#F1F5F9', fontSize: 20, fontWeight: 'bold', marginRight: 12 },
  badge: { backgroundColor: '#DC2626' },
  help: { color: '#94A3B8', fontSize: 12, marginBottom: 16 },
  card: { backgroundColor: '#1E293B', marginBottom: 10, borderRadius: 8 },
  metricRow: { flexDirection: 'row', justifyContent: 'space-between' },
  metric: { color: '#F1F5F9', fontSize: 13 },
  empty: { alignItems: 'center', marginTop: 80 },
  emptyText: { color: '#16A34A', fontSize: 16, marginTop: 12 },
  modalWrap: { flex: 1, backgroundColor: 'rgba(0,0,0,0.6)',
    justifyContent: 'center', padding: 24 },
  modalCard: { backgroundColor: '#1E293B', borderRadius: 12 },
  input: { backgroundColor: '#0F172A', marginBottom: 12 },
  modalBtns: { flexDirection: 'row', justifyContent: 'flex-end', gap: 12 },
});