// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function FingerprintScreen(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Fixture fingerprints'),
    React.createElement(Text, { style: styles.subtitle }, 'Branch signature classification by jayis1'),
    props.fingerprints.map((item) => React.createElement(View, { key: item.name, style: styles.row },
      React.createElement(Text, { style: styles.name }, item.name),
      React.createElement(Text, { style: styles.value }, `${item.score}% match`),
      React.createElement(Text, { style: styles.caption }, item.note)
    ))
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#102024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#244247' },
  title: { color: '#effefd', fontSize: 20, fontWeight: '700' },
  subtitle: { color: '#8fb8b6', marginTop: 4, marginBottom: 12 },
  row: { paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: '#1c3337' },
  name: { color: '#f4fffd', fontWeight: '700' },
  value: { color: '#44d7c0', marginTop: 4 },
  caption: { color: '#c8e8e6', marginTop: 4, lineHeight: 18 }
});

module.exports = FingerprintScreen;
