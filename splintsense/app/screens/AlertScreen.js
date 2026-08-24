// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function badgeColor(level) {
  if (level === 'critical') return '#ff6a5c';
  if (level === 'warning') return '#ffb347';
  if (level === 'caution') return '#79c0ff';
  return '#59d19a';
}

function AlertScreen(props) {
  return React.createElement(
    View,
    { style: styles.card },
    React.createElement(Text, { style: styles.title }, 'Alerts and guidance'),
    props.alerts.map((alert, index) => React.createElement(
      View,
      { key: `alert-${index}`, style: styles.row },
      React.createElement(View, { style: [styles.badge, { backgroundColor: badgeColor(alert.level) }] },
        React.createElement(Text, { style: styles.badgeText }, alert.level.toUpperCase())),
      React.createElement(View, { style: styles.body },
        React.createElement(Text, { style: styles.time }, `${alert.time} • ${alert.code}`),
        React.createElement(Text, { style: styles.detail }, alert.detail))
    ))
  );
}

const styles = StyleSheet.create({
  card: { backgroundColor: '#132024', borderRadius: 16, padding: 16, borderWidth: 1, borderColor: '#264248' },
  title: { color: '#effdff', fontSize: 20, fontWeight: '700', marginBottom: 12 },
  row: { flexDirection: 'row', gap: 12, marginBottom: 12, alignItems: 'flex-start' },
  badge: { borderRadius: 999, paddingHorizontal: 10, paddingVertical: 6 },
  badgeText: { color: '#091416', fontWeight: '800', fontSize: 11 },
  body: { flex: 1 },
  time: { color: '#cbe7ec', fontWeight: '600' },
  detail: { color: '#9eb5ba', marginTop: 4, lineHeight: 18 }
});

module.exports = AlertScreen;
