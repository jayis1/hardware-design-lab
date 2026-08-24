// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function cellColor(value) {
  if (value > 24) return '#ff6a5c';
  if (value > 16) return '#ffb347';
  return '#4fc08d';
}

function PressureMap(props) {
  const zones = props.zones || [];
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Pressure / Moisture region map'),
    React.createElement(
      View,
      { style: styles.grid },
      zones.map((value, index) => React.createElement(
        View,
        { key: `zone-${index}`, style: [styles.cell, { backgroundColor: cellColor(value) }] },
        React.createElement(Text, { style: styles.cellTitle }, `Z${index + 1}`),
        React.createElement(Text, { style: styles.cellValue }, `${value}`)
      ))
    )
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#172427',
    borderRadius: 14,
    padding: 16,
    borderWidth: 1,
    borderColor: '#264248'
  },
  title: { color: '#edf9fa', fontSize: 16, fontWeight: '700', marginBottom: 12 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 10 },
  cell: { width: '22%', minWidth: 62, borderRadius: 12, padding: 10 },
  cellTitle: { color: '#091416', fontWeight: '700' },
  cellValue: { color: '#091416', marginTop: 6, fontSize: 18, fontWeight: '700' }
});

module.exports = PressureMap;
