/*
 * App.tsx — Cryo-Sentinel companion app root.
 *
 * Author: jayis1
 * License: MIT
 */
import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { Provider as PaperProvider, DefaultTheme } from 'react-native-paper';
import { StatusBar } from 'expo-status-bar';

import Dashboard from './src/screens/Dashboard';
import DewarDetail from './src/screens/DewarDetail';
import Commissioning from './src/screens/Commissioning';
import EscalationChain from './src/screens/EscalationChain';
import AuditLog from './src/screens/AuditLog';
import AlertInbox from './src/screens/AlertInbox';

export type RootStackParamList = {
  Dashboard: undefined;
  DewarDetail: { dewarId: string };
  Commissioning: undefined;
  EscalationChain: { dewarId: string };
  AuditLog: { dewarId: string };
  AlertInbox: undefined;
};

const Stack = createStackNavigator<RootStackParamList>();

const theme = {
  ...DefaultTheme,
  colors: {
    ...DefaultTheme.colors,
    primary: '#0066CC',
    accent: '#00AACC',
    background: '#0F172A',
    surface: '#1E293B',
    text: '#F1F5F9',
  },
};

export default function App() {
  return (
    <PaperProvider theme={theme}>
      <NavigationContainer>
        <StatusBar style="light" />
        <Stack.Navigator
          initialRouteName="Dashboard"
          screenOptions={{
            headerStyle: { backgroundColor: '#1E293B' },
            headerTintColor: '#F1F5F9',
            headerTitleStyle: { fontWeight: 'bold' },
          }}
        >
          <Stack.Screen name="Dashboard" component={Dashboard}
            options={{ title: 'Cryo-Sentinel Fleet' }} />
          <Stack.Screen name="DewarDetail" component={DewarDetail}
            options={({ route }) => ({ title: `Dewar ${route.params.dewarId}` })} />
          <Stack.Screen name="Commissioning" component={Commissioning}
            options={{ title: 'Commission a Dewar' }} />
          <Stack.Screen name="EscalationChain" component={EscalationChain}
            options={({ route }) => ({ title: `Escalation — ${route.params.dewarId}` })} />
          <Stack.Screen name="AuditLog" component={AuditLog}
            options={({ route }) => ({ title: `Audit Log — ${route.params.dewarId}` })} />
          <Stack.Screen name="AlertInbox" component={AlertInbox}
            options={{ title: 'Active Alerts' }} />
        </Stack.Navigator>
      </NavigationContainer>
    </PaperProvider>
  );
}