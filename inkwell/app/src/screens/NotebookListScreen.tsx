// NotebookListScreen.tsx — List of saved handwriting sessions
//
// Shows a scrolling list of NotebookCards, one per session, sorted by date.
// Tapping a card opens the SessionDetailScreen for that session.
//
// Author: jayis1
// Copyright (C) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useEffect, useState, useCallback } from 'react';
import { View, FlatList, StyleSheet, Text, TouchableOpacity } from 'react-native';
import NotebookCard from '../components/NotebookCard';
import { listSessions, SessionRecord } from '../db/database';

type Props = { navigation: any };

export default function NotebookListScreen({ navigation }: Props) {
  const [sessions, setSessions] = useState<SessionRecord[]>([]);

  const refresh = useCallback(async () => {
    const rows = await listSessions();
    setSessions(rows);
  }, []);

  useEffect(() => {
    const unsub = navigation.addListener('focus', refresh);
    return unsub;
  }, [navigation, refresh]);

  const handleOpen = (id: number) => {
    navigation.navigate('SessionDetail', { sessionId: id });
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Notebooks</Text>
      <FlatList
        data={sessions}
        keyExtractor={item => item.id.toString()}
        renderItem={({ item }) => (
          <NotebookCard session={item} onPress={handleOpen} />
        )}
        ListEmptyComponent={
          <Text style={styles.empty}>No sessions yet. Start writing on the Live Canvas.</Text>
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f9f7f1' },
  title:     { fontSize: 24, fontWeight: '700', color: '#1a1a2e',
               padding: 16, textAlign: 'center' },
  empty:     { textAlign: 'center', color: '#888', marginTop: 40, fontSize: 14 },
});