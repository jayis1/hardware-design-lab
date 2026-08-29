// DrainVeil maintenance screen
// Author: jayis1
const React = require('react');
const { StyleSheet, Text, View } = require('react-native');

function MaintenanceScreen(props) {
  return React.createElement(
    View,
    { style: styles.section },
    React.createElement(Text, { style: styles.title }, 'Maintenance Plan'),
    ...props.tasks.map((task, index) => React.createElement(
      View,
      { key: `task-${index}`, style: styles.task },
      React.createElement(Text, { style: styles.taskTitle }, task.title),
      React.createElement(Text, { style: styles.effect }, task.effect)
    ))
  );
}

const styles = StyleSheet.create({
  section: { gap: 10 },
  title: { color: '#f0f8ff', fontSize: 20, fontWeight: '700' },
  task: { backgroundColor: '#132029', borderRadius: 12, padding: 12, gap: 4, borderLeftWidth: 3, borderLeftColor: '#49a7ff' },
  taskTitle: { color: '#f0f8ff', fontWeight: '700' },
  effect: { color: '#b7cdda', lineHeight: 18 }
});

module.exports = MaintenanceScreen;
