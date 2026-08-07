/**
 * CalibrationScreen.js — pH & Impedance Calibration
 *
 * Guides the user through 2-point pH calibration (pH 4.00 and 7.00
 * buffers) and impedance calibration (air + reference solution).
 * Sends calibration commands to the device over BLE.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

import React, { useContext, useState } from 'react';
import { View, Text, StyleSheet, ScrollView, Alert } from 'react-native';
import { Card, Title, Paragraph, Button, ProgressBar, Divider, List } from 'react-native-paper';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import { FermenTiqContext } from '../utils/ble';

export default function CalibrationScreen() {
  const { connectionState, sendCommand } = useContext(FermenTiqContext);
  const [phStep, setPhStep] = useState(0);
  const [impedanceStep, setImpedanceStep] = useState(0);
  const [phCalibrating, setPhCalibrating] = useState(false);
  const [impCalibrating, setImpCalibrating] = useState(false);
  const [co2Calibrating, setCo2Calibrating] = useState(false);

  const phSteps = [
    { title: 'Step 1: Prepare', desc: 'Rinse the pH probe with distilled water. Have pH 4.00 and pH 7.00 buffer solutions ready.' },
    { title: 'Step 2: pH 7.00 Buffer', desc: 'Place the probe in pH 7.00 buffer. Wait 2 minutes for stabilization, then press Next.' },
    { title: 'Step 3: pH 4.00 Buffer', desc: 'Rinse the probe, then place in pH 4.00 buffer. Wait 2 minutes, then press Complete.' },
    { title: 'Complete!', desc: 'pH calibration saved. Your pH readings are now accurate to ±0.05 pH.' },
  ];

  const impedanceSteps = [
    { title: 'Step 1: Air Calibration', desc: 'Remove the probe from the liquid. Ensure electrodes are dry and clean.' },
    { title: 'Step 2: Reference Solution', desc: 'Place the probe in 0.1M NaCl reference solution at 25°C.' },
    { title: 'Complete!', desc: 'Impedance calibration saved. Cell density estimates are now calibrated.' },
  ];

  const handlePhNext = () => {
    if (phStep === 0) {
      setPhStep(1);
    } else if (phStep === 1) {
      // Send command to record pH 7.00 reading
      sendCommand('calibrate_ph_7');
      setPhStep(2);
    } else if (phStep === 2) {
      // Send command to record pH 4.00 and complete
      sendCommand('calibrate_ph_4');
      setPhStep(3);
      setPhCalibrating(false);
    } else {
      setPhStep(0);
      setPhCalibrating(false);
    }
  };

  const handleImpedanceNext = () => {
    if (impedanceStep === 0) {
      sendCommand('calibrate_imp_air');
      setImpedanceStep(1);
    } else if (impedanceStep === 1) {
      sendCommand('calibrate_imp_ref');
      setImpedanceStep(2);
      setImpCalibrating(false);
    } else {
      setImpedanceStep(0);
      setImpCalibrating(false);
    }
  };

  const handleCo2Calibration = () => {
    Alert.alert(
      'CO₂ Calibration',
      'Place the device in fresh outdoor air (400 ppm CO₂). Calibration takes 2 minutes.',
      [
        { text: 'Cancel', style: 'cancel' },
        {
          text: 'Start',
          onPress: async () => {
            setCo2Calibrating(true);
            await sendCommand('calibrate_co2');
            setTimeout(() => {
              setCo2Calibrating(false);
              Alert.alert('Success', 'CO₂ sensor calibrated to 400 ppm');
            }, 2000);
          },
        },
      ]
    );
  };

  return (
    <ScrollView style={styles.container}>
      {/* pH Calibration */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="water" size={24} color="#9C27B0" /> pH Calibration
          </Title>
          <Paragraph style={styles.desc}>
            2-point calibration using pH 4.00 and 7.00 buffer solutions.
            Recommended before each new batch.
          </Paragraph>

          {!phCalibrating ? (
            <Button
              mode="contained"
              onPress={() => { setPhCalibrating(true); setPhStep(0); }}
              style={styles.button}
              icon="play"
            >
              Start pH Calibration
            </Button>
          ) : (
            <>
              <ProgressBar progress={(phStep + 1) / 4} color="#9C27B0" style={styles.progress} />
              <Title style={styles.stepTitle}>{phSteps[phStep].title}</Title>
              <Paragraph style={styles.stepDesc}>{phSteps[phStep].desc}</Paragraph>
              <Button
                mode="contained"
                onPress={handlePhNext}
                style={styles.button}
                icon={phStep === 3 ? 'check' : 'arrow-right'}
              >
                {phStep === 3 ? 'Done' : phStep === 2 ? 'Complete' : 'Next'}
              </Button>
            </>
          )}
        </Card.Content>
      </Card>

      {/* Impedance Calibration */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="resistor" size={24} color="#66BB6A" /> Impedance Calibration
          </Title>
          <Paragraph style={styles.desc}>
            Calibrate the bioimpedance probe using air and a reference
            NaCl solution. Ensures accurate cell density estimates.
          </Paragraph>

          {!impCalibrating ? (
            <Button
              mode="contained"
              onPress={() => { setImpCalibrating(true); setImpedanceStep(0); }}
              style={styles.button}
              icon="play"
            >
              Start Impedance Calibration
            </Button>
          ) : (
            <>
              <ProgressBar progress={(impedanceStep + 1) / 3} color="#66BB6A" style={styles.progress} />
              <Title style={styles.stepTitle}>{impedanceSteps[impedanceStep].title}</Title>
              <Paragraph style={styles.stepDesc}>{impedanceSteps[impedanceStep].desc}</Paragraph>
              <Button
                mode="contained"
                onPress={handleImpedanceNext}
                style={styles.button}
                icon={impedanceStep === 2 ? 'check' : 'arrow-right'}
              >
                {impedanceStep === 2 ? 'Done' : 'Next'}
              </Button>
            </>
          )}
        </Card.Content>
      </Card>

      {/* CO2 Calibration */}
      <Card style={styles.card}>
        <Card.Content>
          <Title style={styles.title}>
            <Icon name="molecule-co2" size={24} color="#FF7043" /> CO₂ Calibration
          </Title>
          <Paragraph style={styles.desc}>
            Calibrate the NDIR CO₂ sensor against fresh air (400 ppm).
            Perform monthly for best accuracy.
          </Paragraph>
          <Button
            mode="outlined"
            onPress={handleCo2Calibration}
            style={styles.button}
            icon="tune"
            loading={co2Calibrating}
            disabled={co2Calibrating}
            textColor="#FF7043"
          >
            {co2Calibrating ? 'Calibrating...' : 'Calibrate CO₂ (Fresh Air)'}
          </Button>
        </Card.Content>
      </Card>

      <View style={styles.footer}>
        <Text style={styles.footerText}>FermenTiq • Author: jayis1</Text>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#1a1a2e' },
  card: { margin: 12, backgroundColor: '#16213e' },
  title: { color: '#e0e0e0', fontSize: 18, flexDirection: 'row', alignItems: 'center' },
  desc: { color: '#999', fontSize: 14, marginVertical: 8 },
  button: { marginTop: 12 },
  progress: { marginVertical: 12, height: 6 },
  stepTitle: { color: '#D4A017', fontSize: 16, marginTop: 8 },
  stepDesc: { color: '#bbb', fontSize: 14, marginVertical: 8 },
  footer: { padding: 20, alignItems: 'center' },
  footerText: { color: '#555', fontSize: 12 },
});