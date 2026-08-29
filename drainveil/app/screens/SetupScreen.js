// DrainVeil setup screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function SetupScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Setup Profile'),
    React.createElement(Text, { style: styles.body }, `Profile: ${props.setup.profile}`),
    React.createElement(Text, { style: styles.body }, `Pipe: ${props.setup.pipe}`),
    React.createElement(Text, { style: styles.body }, `Sync: ${props.setup.sync}`),
    React.createElement(Text, { style: styles.body }, `Thresholds: ${props.setup.thresholds}`),
    React.createElement(Text, { style: styles.note }, props.setup.notes)
  );
}

const styles = StyleSheet.create({
  section: { gap: 10, marginBottom: 32 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  body: { color: '#bed0da', backgroundColor: '#132029', borderRadius: 10, padding: 12 },
  note: { color: '#95b4c5', lineHeight: 20, backgroundColor: '#101a22', borderRadius: 12, padding: 12 }
});

module.exports = SetupScreen;
