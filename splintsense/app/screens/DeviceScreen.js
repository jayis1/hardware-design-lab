// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const PressureMap = require('../components/PressureMap');

function DeviceScreen(props) {
  const device = props.device;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Device and liner'),
    React.createElement(Text, { style: styles.line }, `Name: ${device.name}`),
    React.createElement(Text, { style: styles.line }, `Firmware: ${device.firmware}`),
    React.createElement(Text, { style: styles.line }, `Liner profile: ${device.linerProfile}`),
    React.createElement(Text, { style: styles.line }, `Calibration age: ${device.calibrationAgeDays} days`),
    React.createElement(Text, { style: styles.line }, `Flash usage: ${device.flashUsage}`),
    React.createElement(Text, { style: styles.line }, `Author: ${device.author}`),
    React.createElement(PressureMap, { zones: [26, 24, 18, 14, 13, 17, 19, 22] })
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  line: { color: '#b8d7dc', marginBottom: 6 }
});

module.exports = DeviceScreen;
