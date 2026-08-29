// DrainVeil drain map screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');
const NodeChip = require('../components/NodeChip');

function DrainMapScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Drain Map'),
    React.createElement(Text, { style: styles.caption }, 'Compare branches and identify the true problem node before dispatching service.'),
    React.createElement(
      View,
      { style: styles.list },
      ...props.nodes.map((node, index) => React.createElement(NodeChip, {
        key: `node-${index}`,
        name: node.name,
        risk: node.risk,
        detail: node.detail
      }))
    )
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  caption: { color: '#9fb8c7', lineHeight: 18 },
  list: { gap: 10 }
});

module.exports = DrainMapScreen;
