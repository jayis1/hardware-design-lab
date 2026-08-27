// SealBeat device screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function row(label, value) {
  return React.createElement(View, { style: styles.row, key: label },
    React.createElement(Text, { style: styles.label }, label),
    React.createElement(Text, { style: styles.value }, value)
  );
}

function DeviceScreen(props) {
  const device = props.device;
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Device'),
    ['profile', 'firmware', 'connectivity', 'batteryDays', 'installQuality', 'author'].map((key) => row(key, `${device[key]}`))
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 10 },
  row: { flexDirection: 'row', justifyContent: 'space-between', paddingVertical: 7, borderBottomWidth: 1, borderBottomColor: '#173039' },
  label: { color: '#8fb2b9', textTransform: 'capitalize' },
  value: { color: '#f2fbff', fontWeight: '600' }
});

module.exports = DeviceScreen;
