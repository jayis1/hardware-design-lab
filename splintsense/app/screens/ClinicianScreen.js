// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function ClinicianScreen(props) {
  const data = props.data;
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Clinician review'),
    React.createElement(Text, { style: styles.line }, `Last review: ${data.lastReview}`),
    React.createElement(Text, { style: styles.line }, `Export ready: ${data.exportReady ? 'Yes' : 'No'}`),
    React.createElement(Text, { style: styles.noteTitle }, 'Recommended action'),
    React.createElement(Text, { style: styles.note }, data.recommendedAction),
    React.createElement(Text, { style: styles.noteTitle }, 'Analyst note'),
    React.createElement(Text, { style: styles.note }, data.note)
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  line: { color: '#b8d7dc', marginBottom: 6 },
  noteTitle: { color: '#d8eef2', fontWeight: '700', marginTop: 10, marginBottom: 4 },
  note: { color: '#9eb5ba', lineHeight: 18 }
});

module.exports = ClinicianScreen;
