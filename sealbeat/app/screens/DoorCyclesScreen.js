// SealBeat door cycles screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function bar(heightUnits) {
  return React.createElement(View, { style: [styles.bar, { height: 12 + heightUnits * 8 }] });
}

function DoorCyclesScreen(props) {
  const cycles = props.cycles;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Door cycles'),
    React.createElement(Text, { style: styles.summary }, `Today: ${cycles.totalToday} cycles • Average open: ${cycles.averageOpenSeconds}s • Longest: ${cycles.longestOpenSeconds}s`),
    React.createElement(View, { style: styles.chart }, cycles.pattern.map((value, index) => React.createElement(View, { key: `bar-${index}`, style: styles.column }, bar(value), React.createElement(Text, { style: styles.tick }, `${index + 1}`)))),
    React.createElement(Text, { style: styles.insight }, cycles.insight)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 8 },
  summary: { color: '#9bbac0', marginBottom: 12 },
  chart: { flexDirection: 'row', alignItems: 'flex-end', justifyContent: 'space-between', marginBottom: 12 },
  column: { alignItems: 'center', flex: 1 },
  bar: { width: 16, backgroundColor: '#44d0ff', borderRadius: 99, marginBottom: 6 },
  tick: { color: '#6e9298', fontSize: 10 },
  insight: { color: '#bdd2d7', lineHeight: 20 }
});

module.exports = DoorCyclesScreen;
