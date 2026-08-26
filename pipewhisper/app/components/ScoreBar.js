// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function ScoreBar(props) {
  const width = `${Math.max(0, Math.min(100, props.value))}%`;
  return React.createElement(
    View,
    { style: styles.wrapper },
    React.createElement(Text, { style: styles.label }, `${props.label}: ${props.value}%`),
    React.createElement(View, { style: styles.track }, React.createElement(View, { style: [styles.fill, { width }] }))
  );
}

const styles = StyleSheet.create({
  wrapper: { marginTop: 10 },
  label: { color: '#d7f8f5', marginBottom: 6 },
  track: { height: 10, borderRadius: 999, backgroundColor: '#1a3036', overflow: 'hidden' },
  fill: { height: 10, backgroundColor: '#42d5c1', borderRadius: 999 }
});

module.exports = ScoreBar;
