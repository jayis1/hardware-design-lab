/*
 * App.tsx — HiveVox companion app entry point
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * React Native (Expo) app for monitoring HiveVox beehive sensors via
 * LoRaWAN (The Things Network) HTTP integration. Provides an apiary
 * dashboard, per-hive detail charts, alerts feed, and settings screen.
 */
import React, { useState, useEffect, useCallback } from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { StatusBar } from 'expo-status-bar';
import { HiveDatabase } from './components/Database';
import ApiaryDashboard from './screens/ApiaryDashboard';
import HiveDetail from './screens/HiveDetail';
import Alerts from './screens/Alerts';
import Settings from './screens/Settings';

export type HiveData = {
  deveui: string;
  name: string;
  state: number;          // colony_state_t enum
  dominantHz: number;
  rHi: number;
  cvLoud: number;
  broodTempC: number;     // °C
  humidity: number;       // %RH
  weightG: number;        // grams
  batteryMv: number;
  flags: number;
  lastSeen: number;       // unix epoch
  rssi: number;
};

export type AlertEntry = {
  id: number;
  deveui: string;
  hiveName: string;
  type: 'queenless' | 'swarm_prep' | 'dead' | 'weight_drop' | 'low_battery' | 'probe_fault';
  message: string;
  timestamp: number;
  acknowledged: boolean;
};

const Tab = createBottomTabNavigator();

const db = new HiveDatabase();

export default function App() {
  const [hives, setHives] = useState<HiveData[]>([]);
  const [alerts, setAlerts] = useState<AlertEntry[]>([]);
  const [selectedHive, setSelectedHive] = useState<string | null>(null);

  // Refresh hive data from local DB (synced from TTN webhook via background task)
  const refreshHives = useCallback(async () => {
    const data = await db.getAllHives();
    setHives(data);
    const al = await db.getUnacknowledgedAlerts();
    setAlerts(al);
  }, []);

  useEffect(() => {
    refreshHives();
    const interval = setInterval(refreshHives, 30000);  // poll every 30s
    return () => clearInterval(interval);
  }, [refreshHives]);

  return (
    <NavigationContainer>
      <StatusBar style="dark" />
      <Tab.Navigator
        screenOptions={{
          tabBarActiveTintColor: '#e8a800',
          tabBarInactiveTintColor: '#888',
          headerStyle: { backgroundColor: '#fff' },
        }}
      >
        <Tab.Screen name="Apiary">
          {(props: any) => (
            <ApiaryDashboard
              {...props}
              hives={hives}
              onSelectHive={(deveui: string) => {
                setSelectedHive(deveui);
                props.navigation.navigate('HiveDetail');
              }}
            />
          )}
        </Tab.Screen>
        <Tab.Screen name="HiveDetail">
          {(props: any) => (
            <HiveDetail
              {...props}
              hive={hives.find((h) => h.deveui === selectedHive) || null}
              history={selectedHive ? db.getHiveHistory(selectedHive) : []}
            />
          )}
        </Tab.Screen>
        <Tab.Screen name="Alerts">
          {(props: any) => (
            <Alerts
              {...props}
              alerts={alerts}
              onAck={(id: number) => {
                db.acknowledgeAlert(id);
                refreshHives();
              }}
            />
          )}
        </Tab.Screen>
        <Tab.Screen name="Settings">
          {(props: any) => (
            <Settings
              {...props}
              onAddHive={(deveui: string, name: string) => {
                db.addHive(deveui, name);
                refreshHives();
              }}
              onSetInterval={(deveui: string, minutes: number) => {
                db.queueDownlink(deveui, [minutes, 7, 0, 0]);
              }}
              hives={hives}
            />
          )}
        </Tab.Screen>
      </Tab.Navigator>
    </NavigationContainer>
  );
}