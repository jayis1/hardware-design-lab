/*
 * src/screens/EscalationChain.tsx — Edit the per-Dewar alarm escalation chain.
 *
 * Author: jayis1
 * License: MIT
 */
import React, { useEffect, useState } from 'react';
import { View, Text, StyleSheet, FlatList, TouchableOpacity, Alert } from 'react-native';
import { Card, IconButton, Button, TextInput, Switch, ActivityIndicator } from 'react-native-paper';
import { useRoute } from '@react-navigation/native';
import { EscalationEntry } from '../types';
import { getEscalation, putEscalation } from '../cloud/Api';

export default function EscalationChain() {
  const route = useRoute<any>();
  const dewarId = route.params.dewarId;
  const [chain, setChain] = useState<EscalationEntry[]>([]);
  const [loading, setLoading] = useState(true);
  const [dirty, setDirty] = useState(false);

  useEffect(() => {
    getEscalation(dewarId).then((c) => { setChain(c); setLoading(false); })
      .catch((e) => { console.warn(e); setLoading(false); });
  }, [dewarId]);

  const update = (idx: number, patch: Partial<EscalationEntry>) => {
    setChain((prev) => prev.map((e, i) => i === idx ? { ...e, ...patch } : e));
    setDirty(true);
  };

  const add = () => {
    setChain((prev) => [...prev, {
      technicianId: '', name: '', order: prev.length + 1,
      push: true, sms: true, email: false, call: false, delayMin: 0,
    }]);
    setDirty(true);
  };

  const remove = (idx: number) => {
    setChain((prev) => prev.filter((_, i) => i !== idx).map((e, i) => ({ ...e, order: i + 1 })));
    setDirty(true);
  };

  const save = async () => {
    try {
      await putEscalation(dewarId, chain);
      setDirty(false);
      Alert.alert('Saved', 'Escalation chain updated.');
    } catch (e: any) {
      Alert.alert('Save failed', e?.message ?? String(e));
    }
  };

  if (loading) return <View style={s.wrap}><ActivityIndicator /></View>;

  return (
    <View style={s.wrap}>
      <Text style={s.h2}>Escalation chain — {dewarId}</Text>
      <Text style={s.help}>
        When an alarm fires, the chain is contacted in order. If nobody
        acknowledges within the per-entry delay, the next entry is contacted.
        Tier-2 (CRITICAL) alarms repeat every 5 min until acked.
      </Text>

      <FlatList
        data={chain}
        keyExtractor={(_, i) => `e${i}`}
        renderItem={({ item, index }) => (
          <Card style={s.card}>
            <Card.Title
              title={`#${index + 1}`}
              left={(p) => <IconButton {...p} icon="account" />}
              right={(p) => <IconButton {...p} icon="close" onPress={() => remove(index)} />}
            />
            <Card.Content>
              <TextInput label="Name" value={item.name}
                onChangeText={(v) => update(index, { name: v })} style={s.input} />
              <TextInput label="Technician ID" value={item.technicianId}
                onChangeText={(v) => update(index, { technicianId: v })} style={s.input} />
              <View style={s.row}>
                <ChannelToggle label="Push"  value={item.push}
                  onToggle={(v) => update(index, { push: v })} />
                <ChannelToggle label="SMS"   value={item.sms}
                  onToggle={(v) => update(index, { sms: v })} />
                <ChannelToggle label="Email" value={item.email}
                  onToggle={(v) => update(index, { email: v })} />
                <ChannelToggle label="Call"  value={item.call}
                  onToggle={(v) => update(index, { call: v })} />
              </View>
              <TextInput label="Delay before next (min)" value={String(item.delayMin)}
                onChangeText={(v) => update(index, { delayMin: parseInt(v, 10) || 0 })}
                style={s.input} keyboardType="numeric" />
            </Card.Content>
          </Card>
        )}
      />
      <View style={s.btnRow}>
        <Button mode="outlined" onPress={add} icon="plus">Add entry</Button>
        <Button mode="contained" onPress={save} disabled={!dirty}>Save</Button>
      </View>
    </View>
  );
}

function ChannelToggle({ label, value, onToggle }: {
  label: string; value: boolean; onToggle: (v: boolean) => void;
}) {
  return (
    <View style={s.toggle}>
      <Text style={s.toggleLabel}>{label}</Text>
      <Switch value={value} onValueChange={onToggle} color="#0066CC" />
    </View>
  );
}

const s = StyleSheet.create({
  wrap: { flex: 1, backgroundColor: '#0F172A', padding: 16 },
  h2: { color: '#F1F5F9', fontSize: 20, fontWeight: 'bold', marginBottom: 8 },
  help: { color: '#94A3B8', fontSize: 12, marginBottom: 16 },
  card: { backgroundColor: '#1E293B', marginBottom: 12, borderRadius: 8 },
  input: { backgroundColor: '#0F172A', marginBottom: 8 },
  row: { flexDirection: 'row', justifyContent: 'space-around', marginVertical: 8 },
  toggle: { alignItems: 'center' },
  toggleLabel: { color: '#F1F5F9', fontSize: 11, marginBottom: 4 },
  btnRow: { flexDirection: 'row', justifyContent: 'space-between', marginVertical: 16 },
});