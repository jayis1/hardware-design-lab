// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function DeviceScreen(props) {
  const device = props.device;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Device details'),
    React.createElement(Text, { style: styles.line }, `Name: ${device.name}`),
    React.createElement(Text, { style: styles.line }, `Author: ${device.author}`),
    React.createElement(Text, { style: styles.line }, `Firmware: ${device.firmware}`),
    React.createElement(Text, { style: styles.line }, `Last calibration: ${device.lastCalibration}`),
    React.createElement(Text, { style: styles.line }, `Flash usage: ${device.flashUsage}`),
    React.createElement(Text, { style: styles.line }, `Matter bridge: ${device.matterBridge}`)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700', marginBottom: 10 },
  line: { color: '#eafef9', lineHeight: 22 }
});

module.exports = DeviceScreen;
