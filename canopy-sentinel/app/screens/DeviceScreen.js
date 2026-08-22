// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function DeviceScreen(props) {
  const device = props.device;
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.heading }, 'Device Console'),
    React.createElement(View, { style: styles.card },
      React.createElement(Text, { style: styles.text }, 'Serial: ' + device.serial),
      React.createElement(Text, { style: styles.text }, 'Battery: ' + device.battery + '%'),
      React.createElement(Text, { style: styles.text }, 'Firmware: ' + device.firmware),
      React.createElement(Text, { style: styles.text }, 'Active crop: ' + device.crop),
      React.createElement(Text, { style: styles.text }, 'Author: jayis1')
    )
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  heading: { color: '#d7f7e6', fontSize: 22, fontWeight: '700' },
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#294233' },
  text: { color: '#c8e7d6', marginBottom: 6 }
});

module.exports = DeviceScreen;
