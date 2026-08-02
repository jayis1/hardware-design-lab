/*
 * LibraryScreen.tsx — onboard liquid class library management
 * Author: jayis1
 */
import React, { useState } from 'react';
import {
  View, Text, FlatList, TouchableOpacity, TextInput, StyleSheet, Alert,
} from 'react-native';
import { hydra } from '../ble';

interface LiquidClass { id: number; name: string; samples: number; }

// Seed with the shipped default library (24 classes).
const SEED: LiquidClass[] = [
  { id: 0,  name: 'Distilled water',  samples: 12 },
  { id: 1,  name: 'Whole milk',       samples: 14 },
  { id: 2,  name: 'Skim milk',       samples: 12 },
  { id: 3,  name: 'Tap water',        samples: 18 },
  { id: 4,  name: 'Whisky 40% ABV',   samples: 16 },
  { id: 5,  name: 'Vodka 40%',        samples: 12 },
  { id: 6,  name: 'Methanol',         samples: 20 },
  { id: 7,  name: 'Isopropanol',      samples: 10 },
  { id: 8,  name: 'White wine',       samples: 12 },
  { id: 9,  name: 'Honey (pure)',     samples: 14 },
  { id: 10, name: 'Glucose syrup',    samples: 10 },
  { id: 11, name: 'Petrol',           samples: 8  },
  { id: 12, name: 'Kerosene',         samples: 8  },
  { id: 13, name: 'Distilled (calib)', samples: 12 },
  { id: 14, name: '0.9% Saline',      samples: 12 },
  { id: 15, name: 'Olive oil',        samples: 10 },
  { id: 16, name: 'Sunflower oil',    samples: 10 },
  { id: 17, name: '5% Glucose IV',    samples: 8  },
  { id: 18, name: 'Vinegar',          samples: 8  },
  { id: 19, name: 'Cola',             samples: 8  },
  { id: 20, name: 'Black coffee',     samples: 8  },
  { id: 21, name: 'Mineral water',    samples: 10 },
  { id: 22, name: 'Red wine',         samples: 10 },
  { id: 23, name: 'Diesel',           samples: 8  },
];

export default function LibraryScreen() {
  const [classes, setClasses] = useState(SEED);
  const [name, setName] = useState('');

  const addClass = async () => {
    if (!name.trim()) { Alert.alert('Enter a class name'); return; }
    const id = Math.max(...classes.map(c => c.id), 0) + 1;
    // In a real flow the user captures N calibration shots which the
    // app uploads to the device; here we just register the slot.
    const cmd = `L,ADD,${id},${name.trim()},16,` +
      Array(32).fill('0.0').join(',');
    await hydra.sendLibraryAdd(cmd);
    setClasses([...classes, { id, name: name.trim(), samples: 0 }]);
    setName('');
  };

  const renderItem = ({ item }: { item: LiquidClass }) => (
    <View style={styles.row}>
      <Text style={styles.id}>{item.id}</Text>
      <Text style={styles.name}>{item.name}</Text>
      <Text style={styles.samples}>{item.samples} shots</Text>
    </View>
  );

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Liquid Library</Text>
      <FlatList data={classes} keyExtractor={c => String(c.id)}
        renderItem={renderItem} contentContainerStyle={{ paddingBottom: 80 }} />

      <View style={styles.add}>
        <TextInput style={styles.input} placeholder="New class name…"
          value={name} onChangeText={setName} />
        <TouchableOpacity style={styles.addBtn} onPress={addClass}>
          <Text style={styles.addBtnText}>Add</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16, backgroundColor: '#fafafa' },
  title:    { fontSize: 22, fontWeight: '700', marginVertical: 12, color: '#1e88e5' },
  row:      { flexDirection: 'row', padding: 12, backgroundColor: '#fff', marginBottom: 4, borderRadius: 6 },
  id:       { width: 30, color: '#999' },
  name:     { flex: 1, fontSize: 15, fontWeight: '500' },
  samples:  { fontSize: 13, color: '#777' },
  add:      { position: 'absolute', bottom: 16, left: 16, right: 16, flexDirection: 'row' },
  input:    { flex: 1, backgroundColor: '#fff', borderWidth: 1, borderColor: '#ddd',
              borderRadius: 8, paddingHorizontal: 12, paddingVertical: 10 },
  addBtn:   { backgroundColor: '#1e88e5', paddingHorizontal: 18, justifyContent: 'center',
              borderRadius: 8, marginLeft: 8 },
  addBtnText: { color: '#fff', fontWeight: '600' },
});