// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function colorFor(value) {
  if (value < 17) return '#20344a';
  if (value < 18) return '#286c87';
  if (value < 19) return '#31a589';
  if (value < 20) return '#7ec850';
  return '#e5b445';
}

function ThermalGrid(props) {
  const frame = props.frame || [];
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Thermal Canopy View'),
    React.createElement(
      View,
      { style: styles.grid },
      frame.slice(0, 48).map((value, index) => React.createElement(View, {
        key: 'px-' + index,
        style: [styles.pixel, { backgroundColor: colorFor(value) }]
      }))
    )
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 14, borderWidth: 1, borderColor: '#294233' },
  title: { color: '#d7f7e6', fontSize: 16, fontWeight: '700', marginBottom: 10 },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 2 },
  pixel: { width: '10%', aspectRatio: 1, borderRadius: 3 }
});

module.exports = ThermalGrid;
