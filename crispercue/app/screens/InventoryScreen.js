// CrisperCue inventory screen
// Author: jayis1
import React from 'react';
import { StyleSheet, Text, TouchableOpacity, View } from 'react-native';

export default function InventoryScreen({ bins, selectedBin, setSelectedBin }) {
  return (
    <View>
      <Text style={styles.heading}>Drawer inventory</Text>
      {bins.map((bin) => (
        <TouchableOpacity key={bin.id} style={[styles.row, selectedBin === bin.id && styles.activeRow]} onPress={() => setSelectedBin(bin.id)}>
          <View>
            <Text style={styles.name}>{bin.name}</Text>
            <Text style={styles.produce}>{bin.produce}</Text>
          </View>
          <View style={styles.trailing}>
            <Text style={styles.freshness}>{bin.freshness}%</Text>
            <Text style={styles.mass}>{bin.massG} g</Text>
          </View>
        </TouchableOpacity>
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  heading: { color: '#F6FBFF', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  row: {
    backgroundColor: '#10212D',
    borderRadius: 16,
    padding: 14,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#1A3342',
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  activeRow: { borderColor: '#56D3A2', backgroundColor: '#133242' },
  name: { color: '#F6FBFF', fontWeight: '700', fontSize: 16 },
  produce: { color: '#9EC5D1', marginTop: 4 },
  trailing: { alignItems: 'flex-end' },
  freshness: { color: '#56D3A2', fontSize: 22, fontWeight: '700' },
  mass: { color: '#D8E8EE', marginTop: 4 },
});
