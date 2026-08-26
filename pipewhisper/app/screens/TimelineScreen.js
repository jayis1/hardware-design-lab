// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const TimelineRow = require('../components/TimelineRow');

function TimelineScreen(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Anomaly timeline'),
    React.createElement(Text, { style: styles.subtitle }, 'Overnight and active-use events'),
    props.timeline.map((item) => React.createElement(TimelineRow, { key: `${item.time}-${item.level}`, item }))
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 10 }
});

module.exports = TimelineScreen;
