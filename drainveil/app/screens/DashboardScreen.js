// DrainVeil dashboard screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const MetricCard = require('../components/MetricCard');

function DashboardScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Dashboard'),
    React.createElement(
      View,
      { style: styles.grid },
      ...props.data.cards.map((card, index) => React.createElement(MetricCard, {
        key: `card-${index}`,
        label: card.label,
        value: card.value,
        caption: card.caption
      }))
    )
  );
}

const styles = StyleSheet.create({
  section: { gap: 12 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  grid: { flexDirection: 'row', flexWrap: 'wrap', gap: 12 }
});

module.exports = DashboardScreen;
