/*
 * App.js — StudGuard companion application
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: MIT
 */

import React, { useMemo, useState } from 'react';
import { SafeAreaView, StatusBar } from 'react-native';
import DashboardScreen from './screens/DashboardScreen.js';
import NodeDetailScreen from './screens/NodeDetailScreen.js';
import SurveyScreen from './screens/SurveyScreen.js';
import SettingsScreen from './screens/SettingsScreen.js';
import { sampleNodes } from './utils/protocol.js';

const h = React.createElement;

export default function App() {
  const [route, setRoute] = useState('dashboard');
  const [selectedNode, setSelectedNode] = useState(sampleNodes[0]);
  const nodes = useMemo(() => sampleNodes, []);

  let screen = h(DashboardScreen, {
    nodes,
    onSelectNode: (node) => {
      setSelectedNode(node);
      setRoute('detail');
    },
    onOpenSurvey: () => setRoute('survey'),
    onOpenSettings: () => setRoute('settings')
  });

  if (route === 'detail') {
    screen = h(NodeDetailScreen, { node: selectedNode, onBack: () => setRoute('dashboard') });
  } else if (route === 'survey') {
    screen = h(SurveyScreen, { onBack: () => setRoute('dashboard') });
  } else if (route === 'settings') {
    screen = h(SettingsScreen, { onBack: () => setRoute('dashboard') });
  }

  return h(
    SafeAreaView,
    { style: { flex: 1, backgroundColor: '#020617' } },
    h(StatusBar, { barStyle: 'light-content' }),
    screen
  );
}
