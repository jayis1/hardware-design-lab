// VentLattice device screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function DeviceScreen({ device }) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.heading }, 'Device details'),
    React.createElement(Text, { style: styles.body }, `Node: ${device.name}`),
    React.createElement(Text, { style: styles.body }, `Author: ${device.author}`),
    React.createElement(Text, { style: styles.body }, `Firmware: ${device.firmware}`),
    React.createElement(Text, { style: styles.body }, `Battery: ${device.battery}`),
    React.createElement(Text, { style: styles.body }, `Mesh status: ${device.mesh}`),
    React.createElement(Text, { style: styles.body }, `Last sync: ${device.lastSync}`)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e23', borderRadius: 18, padding: 16 },
  heading: { color: '#f4ffff', fontWeight: '700', fontSize: 22, marginBottom: 10 },
  body: { color: '#d8f4f4', marginBottom: 6 }
});

module.exports = DeviceScreen;
