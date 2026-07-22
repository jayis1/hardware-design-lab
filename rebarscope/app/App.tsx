/**
 * RebarScope — Companion app entry point (React Native / Expo)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import ConnectScreen from './src/screens/ConnectScreen';
import SurveySetupScreen from './src/screens/SurveySetupScreen';
import LiveSurveyScreen from './src/screens/LiveSurveyScreen';
import HeatmapScreen from './src/screens/HeatmapScreen';
import LprDetailScreen from './src/screens/LprDetailScreen';
import ReportScreen from './src/screens/ReportScreen';

export type RootStackParamList = {
  Connect: undefined;
  SurveySetup: { siteName: string };
  Live: { config: SurveyConfig; siteName: string };
  Heatmap: { surveyId: string };
  Lpr: { surveyId: string };
  Report: { surveyId: string };
};

export interface SurveyConfig {
  gridResMm: number;
  refElectrode: 'CuCuSO4' | 'AgAgCl';
  modalityMask: number;
  siteName: string;
}

const Stack = createStackNavigator<RootStackParamList>();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator
        initialRouteName="Connect"
        screenOptions={{
          headerStyle: { backgroundColor: '#1a1a2e' },
          headerTintColor: '#fff',
          headerTitleStyle: { fontWeight: 'bold' },
        }}
      >
        <Stack.Screen
          name="Connect"
          component={ConnectScreen}
          options={{ title: 'RebarScope — Connect' }}
        />
        <Stack.Screen
          name="SurveySetup"
          component={SurveySetupScreen}
          options={{ title: 'Survey Setup' }}
        />
        <Stack.Screen
          name="Live"
          component={LiveSurveyScreen}
          options={{ title: 'Live Survey' }}
        />
        <Stack.Screen
          name="Heatmap"
          component={HeatmapScreen}
          options={{ title: 'Corrosion Heatmap' }}
        />
        <Stack.Screen
          name="Lpr"
          component={LprDetailScreen}
          options={{ title: 'LPR Corrosion Rate' }}
        />
        <Stack.Screen
          name="Report"
          component={ReportScreen}
          options={{ title: 'Inspection Report' }}
        />
      </Stack.Navigator>
    </NavigationContainer>
  );
}