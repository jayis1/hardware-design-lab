// SealBeat maintenance screen
// Author: jayis1
const React = require('react');
const { View, Text, StyleSheet } = require('react-native');

function TaskCard(task, index) {
  return React.createElement(
    View,
    { key: `task-${index}`, style: styles.task },
    React.createElement(Text, { style: styles.priority }, task.priority),
    React.createElement(Text, { style: styles.taskTitle }, task.title),
    React.createElement(Text, { style: styles.taskDetail }, task.detail)
  );
}

function MaintenanceScreen(props) {
  return React.createElement(
    View,
    { style: styles.panel },
    React.createElement(Text, { style: styles.title }, 'Maintenance coach'),
    props.tasks.map(TaskCard)
  );
}

const styles = StyleSheet.create({
  panel: { backgroundColor: '#0f1e24', borderRadius: 16, padding: 14, borderWidth: 1, borderColor: '#1b343d' },
  title: { color: '#effcff', fontSize: 20, fontWeight: '700', marginBottom: 10 },
  task: { backgroundColor: '#11262d', borderRadius: 12, padding: 12, marginBottom: 10 },
  priority: { color: '#7cd2ff', fontSize: 12, fontWeight: '700', marginBottom: 6 },
  taskTitle: { color: '#f5ffff', fontSize: 16, fontWeight: '700', marginBottom: 4 },
  taskDetail: { color: '#bad2d8', lineHeight: 20 }
});

module.exports = MaintenanceScreen;
