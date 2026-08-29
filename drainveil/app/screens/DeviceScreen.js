// DrainVeil device screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function item(label, value) {
  return React.createElement(
    View,
    { style: styles.row, key: label },
    React.createElement(Text, { style: styles.label }, label),
    React.createElement(Text, { style: styles.value }, value)
  );
}

function DeviceScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Device'),
    item('Node', props.device.name),
    item('Firmware', props.device.firmware),
    item('Radio', props.device.radio),
    item('Confidence', props.device.confidence),
    item('Storage', props.device.storage),
    item('Author', props.device.author)
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  row: { flexDirection: 'row', justifyContent: 'space-between', backgroundColor: '#132029', borderRadius: 10, padding: 12 },
  label: { color: '#98b2c1' },
  value: { color: '#eef7ff', fontWeight: '700' }
});

module.exports = DeviceScreen;
