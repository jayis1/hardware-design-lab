/**
 * SettingsScreen.tsx — App settings, alert configuration, and data export
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

import React, { useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert, Share } from 'react-native';
import {
  Card,
  Title,
  Paragraph,
  Button,
  TextInput,
  Switch,
  Divider,
  List,
  Dialog,
  Portal,
  RadioButton,
  Chip,
} from 'react-native-paper';
import { useApp } from '../context/AppContext';
import { SPECIES_NAMES, AlertConfig } from '../types';

export default function SettingsScreen() {
  const { api, gatewayUrl, setGatewayUrl, alerts, refreshData } = useApp();
  const [newAlertDialog, setNewAlertDialog] = useState(false);
  const [exportFormat, setExportFormat] = useState<'csv' | 'geojson' | 'isobus'>('csv');
  const [exportDialog, setExportDialog] = useState(false);
  const [pushNotifications, setPushNotifications] = useState(true);
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [newAlert, setNewAlert] = useState<Partial<AlertConfig>>({
    species: '',
    severity_threshold: 'med',
    proximity_m: 100,
    enabled: true,
  });

  const handleCreateAlert = async () => {
    try {
      await api.createAlert(newAlert as Omit<AlertConfig, 'id' | 'created_at'>);
      setNewAlertDialog(false);
      Alert.alert('Success', 'Alert created successfully');
      refreshData();
    } catch (err) {
      Alert.alert('Error', err instanceof Error ? err.message : 'Failed to create alert');
    }
  };

  const handleDeleteAlert = async (alertId: string) => {
    Alert.alert(
      'Delete Alert',
      'Are you sure you want to delete this alert?',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Delete',
          style: 'destructive',
          onPress: async () => {
            try {
              await api.deleteAlert(alertId);
              refreshData();
            } catch (err) {
              Alert.alert('Error', 'Failed to delete alert');
            }
          },
        },
      ]
    );
  };

  const handleExport = async () => {
    try {
      const data = await api.exportData(exportFormat);
      setExportDialog(false);
      await Share.share({
        message: data,
        title: `SpectraPest Export (${exportFormat.toUpperCase()})`,
      });
    } catch (err) {
      Alert.alert('Export Error', err instanceof Error ? err.message : 'Failed to export');
    }
  };

  /* Species options for alert creation */
  const pestSpecies = Object.entries(SPECIES_NAMES)
    .filter(([id, name]) => parseInt(id) < 60)
    .map(([id, name]) => ({ id: parseInt(id), name }));

  return (
    <ScrollView style={styles.container}>
      {/* Connection settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Gateway Connection</Title>
          <Text style={styles.label}>Gateway URL (WiFi)</Text>
          <TextInput
            value={gatewayUrl}
            onChangeText={setGatewayUrl}
            placeholder="http://192.168.4.1"
            style={styles.input}
            autoCapitalize="none"
            autoCorrect={false}
          />
          <Paragraph style={styles.helpText}>
            Connect to the SpectraPest gateway node's WiFi AP. Default: http://192.168.4.1
          </Paragraph>
        </Card.Content>
      </Card>

      {/* Notification settings */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Notifications</Title>
          <List.Item
            title="Push notifications"
            description="Receive alerts when pest thresholds are exceeded"
            right={() => (
              <Switch value={pushNotifications} onValueChange={setPushNotifications} />
            )}
          />
          <Divider />
          <List.Item
            title="Auto-refresh"
            description="Automatically refresh data every 30 seconds"
            right={() => (
              <Switch value={autoRefresh} onValueChange={setAutoRefresh} />
            )}
          />
        </Card.Content>
      </Card>

      {/* Alert configuration */}
      <Card style={styles.card}>
        <Card.Content>
          <View style={styles.row}>
            <Title>Pest Alerts</Title>
            <Button
              mode="contained"
              onPress={() => setNewAlertDialog(true)}
              icon="plus"
              compact
            >
              Add
            </Button>
          </View>

          {alerts.length === 0 ? (
            <Text style={styles.emptyText}>
              No alerts configured. Tap "Add" to create a pest alert.
            </Text>
          ) : (
            alerts.map((alert, idx) => (
              <View key={alert.id || idx} style={styles.alertRow}>
                <View style={styles.alertInfo}>
                  <Text style={styles.alertSpecies}>{alert.species}</Text>
                  <Text style={styles.alertMeta}>
                    Severity ≥ {alert.severity_threshold} • Proximity ≤ {alert.proximity_m}m
                  </Text>
                </View>
                <Switch
                  value={alert.enabled}
                  onValueChange={async (val) => {
                    try {
                      await api.createAlert({ ...alert, enabled: val });
                      refreshData();
                    } catch (err) { /* ignore */ }
                  }}
                />
                <Button
                  mode="text"
                  onPress={() => handleDeleteAlert(alert.id)}
                  icon="delete"
                  textColor="#E57373"
                >
                  Delete
                </Button>
              </View>
            ))
          )}
        </Card.Content>
      </Card>

      {/* Data export */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>Data Export</Title>
          <Paragraph style={styles.helpText}>
            Export detection data for integration with precision agriculture equipment.
          </Paragraph>
          <View style={styles.formatRow}>
            <Chip
              selected={exportFormat === 'csv'}
              onPress={() => setExportFormat('csv')}
              style={styles.formatChip}
            >
              CSV
            </Chip>
            <Chip
              selected={exportFormat === 'geojson'}
              onPress={() => setExportFormat('geojson')}
              style={styles.formatChip}
            >
              GeoJSON
            </Chip>
            <Chip
              selected={exportFormat === 'isobus'}
              onPress={() => setExportFormat('isobus')}
              style={styles.formatChip}
            >
              ISOBUS XML
            </Chip>
          </View>
          <Button mode="contained" onPress={handleExport} icon="download" style={styles.exportButton}>
            Export as {exportFormat.toUpperCase()}
          </Button>
        </Card.Content>
      </Card>

      {/* About */}
      <Card style={styles.card}>
        <Card.Content>
          <Title>About</Title>
          <Text style={styles.aboutText}>
            SpectraPest Field v1.0
          </Text>
          <Text style={styles.aboutText}>
            Solar-Powered Multispectral & Acoustic Pest Identifier
          </Text>
          <Text style={styles.aboutText}>
            Author: jayis1
          </Text>
          <Text style={styles.aboutText}>
            © 2026 jayis1. All rights reserved.
          </Text>
          <Text style={styles.aboutText}>
            License: MIT
          </Text>
          <Text style={styles.aboutText}>
            Hardware: CERN-OHL-S v2 • Firmware: GPL-2.0
          </Text>
        </Card.Content>
      </Card>

      {/* New alert dialog */}
      <Portal>
        <Dialog visible={newAlertDialog} onDismiss={() => setNewAlertDialog(false)}>
          <Dialog.Title>New Pest Alert</Dialog.Title>
          <Dialog.Content>
            <Text style={styles.dialogLabel}>Species</Text>
            <ScrollView style={styles.speciesPicker} horizontal>
              {pestSpecies.slice(0, 20).map(species => (
                <Chip
                  key={species.id}
                  selected={newAlert.species === species.name}
                  onPress={() => setNewAlert({ ...newAlert, species: species.name })}
                  style={styles.speciesChip}
                >
                  {species.name.split('(')[0].trim()}
                </Chip>
              ))}
            </ScrollView>

            <Text style={styles.dialogLabel}>Severity threshold</Text>
            <RadioButton.Group
              onValueChange={val => setNewAlert({ ...newAlert, severity_threshold: val as any })}
              value={newAlert.severity_threshold || 'med'}
            >
              <View style={styles.radioRow}>
                <RadioButton value="low" />
                <Text>Low</Text>
              </View>
              <View style={styles.radioRow}>
                <RadioButton value="med" />
                <Text>Medium</Text>
              </View>
              <View style={styles.radioRow}>
                <RadioButton value="high" />
                <Text>High</Text>
              </View>
            </RadioButton.Group>

            <Text style={styles.dialogLabel}>Proximity (meters)</Text>
            <TextInput
              keyboardType="numeric"
              value={(newAlert.proximity_m || 100).toString()}
              onChangeText={val => setNewAlert({ ...newAlert, proximity_m: parseInt(val) || 100 })}
              style={styles.dialogInput}
            />
          </Dialog.Content>
          <Dialog.Actions>
            <Button onPress={() => setNewAlertDialog(false)}>Cancel</Button>
            <Button onPress={handleCreateAlert}>Create</Button>
          </Dialog.Actions>
        </Dialog>
      </Portal>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F1F8E9',
    padding: 8,
  },
  card: {
    marginBottom: 12,
  },
  label: {
    fontSize: 13,
    color: '#757575',
    marginBottom: 4,
  },
  input: {
    marginBottom: 8,
  },
  helpText: {
    fontSize: 11,
    color: '#9E9E9E',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  emptyText: {
    color: '#9E9E9E',
    fontStyle: 'italic',
    paddingVertical: 16,
    textAlign: 'center',
  },
  alertRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 12,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: '#E0E0E0',
  },
  alertInfo: {
    flex: 1,
  },
  alertSpecies: {
    fontSize: 14,
    fontWeight: '500',
    color: '#333',
  },
  alertMeta: {
    fontSize: 11,
    color: '#9E9E9E',
    marginTop: 2,
  },
  formatRow: {
    flexDirection: 'row',
    marginVertical: 12,
  },
  formatChip: {
    marginRight: 8,
  },
  exportButton: {
    marginTop: 8,
  },
  aboutText: {
    fontSize: 12,
    color: '#555',
    marginBottom: 4,
  },
  dialogLabel: {
    fontSize: 12,
    color: '#757575',
    marginTop: 12,
    marginBottom: 4,
  },
  speciesPicker: {
    flexDirection: 'row',
    maxHeight: 40,
  },
  speciesChip: {
    marginRight: 6,
    marginBottom: 4,
  },
  radioRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 4,
  },
  dialogInput: {
    marginTop: 4,
  },
});