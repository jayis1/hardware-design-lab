// VentLattice balance screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');
const RoomChip = require('../components/RoomChip');

function BalanceScreen({ rooms }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Whole-home balance snapshot'),
    React.createElement(Text, { style: styles.note }, 'Compare service quality across nearby rooms to tune branch dampers and register openings.'),
    React.createElement(
      View,
      { style: styles.row },
      rooms.map((room, index) => React.createElement(RoomChip, { key: `${room.name}-${index}`, room: room.name, status: `${room.serviceScore}/100 · ${room.status}` }))
    )
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 6 },
  note: { color: '#89b7b7', marginBottom: 12 },
  row: { flexDirection: 'row', flexWrap: 'wrap' }
});

module.exports = BalanceScreen;
