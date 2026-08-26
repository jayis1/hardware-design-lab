// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function TimelineRow(props) {
  return React.createElement(
    View,
    { style: styles.row },
    React.createElement(Text, { style: styles.time }, props.item.time),
    React.createElement(View, { style: styles.content },
      React.createElement(Text, { style: styles.level }, props.item.level.toUpperCase()),
      React.createElement(Text, { style: styles.detail }, props.item.detail)
    )
  );
}

const styles = StyleSheet.create({
  row: { flexDirection: 'row', gap: 12, paddingVertical: 10, borderBottomWidth: 1, borderBottomColor: '#1d3337' },
  time: { color: '#91b3b7', width: 54, fontWeight: '600' },
  content: { flex: 1 },
  level: { color: '#ffd89e', fontSize: 12, fontWeight: '700', marginBottom: 4 },
  detail: { color: '#e8fbf8', lineHeight: 20 }
});

module.exports = TimelineRow;
