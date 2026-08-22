// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function SessionScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.heading }, 'Session Timeline'),
    props.sessions.map((item) => React.createElement(
      View,
      { key: item.id, style: styles.card },
      React.createElement(Text, { style: styles.rowTitle }, item.row + ' · ' + item.risk.toUpperCase()),
      React.createElement(Text, { style: styles.rowMeta }, 'Dew margin ' + item.dewMargin + ' °C · Wetness ' + item.wetness + '% · Spore ' + item.spore),
      React.createElement(Text, { style: styles.note }, item.note)
    ))
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  heading: { color: '#d7f7e6', fontSize: 22, fontWeight: '700' },
  card: { backgroundColor: '#13201a', borderRadius: 12, padding: 12, borderWidth: 1, borderColor: '#294233' },
  rowTitle: { color: '#f4fff8', fontWeight: '700' },
  rowMeta: { color: '#9ec5ae', marginTop: 4 },
  note: { color: '#8fa595', marginTop: 6 }
});

module.exports = SessionScreen;
