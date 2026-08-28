// VentLattice room chip
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function RoomChip({ room, status }) {
  return React.createElement(
    View,
    { style: styles.chip },
    React.createElement(Text, { style: styles.room }, room),
    React.createElement(Text, { style: styles.status }, status)
  );
}

const styles = StyleSheet.create({
  chip: { backgroundColor: '#173038', borderRadius: 999, paddingHorizontal: 12, paddingVertical: 8, marginRight: 8, marginBottom: 8 },
  room: { color: '#f3ffff', fontWeight: '700' },
  status: { color: '#86c5c5', fontSize: 12 }
});

module.exports = RoomChip;
