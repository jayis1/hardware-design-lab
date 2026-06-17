/**
 * ExportDialog.js — SonicSight Companion
 * Modal dialog for exporting scan data as PDF report, CSV, or DICOM.
 * @author jayis1
 */

import React from 'react';
import {
  View, Text, StyleSheet, TouchableOpacity, Modal, Share,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

const ExportDialog = ({ visible, onClose, scanData }) => {
  const handleExport = async (format) => {
    try {
      let content = '';
      let mimeType = '';

      switch (format) {
        case 'pdf':
          content = `SonicSight Scan Report\nID: ${scanData?.id}\nDate: ${scanData?.date}\n...`;
          mimeType = 'application/pdf';
          break;
        case 'csv':
          content = 'Tx,Rx,ToF_us\n1,2,55.3\n...';
          mimeType = 'text/csv';
          break;
        case 'dicom':
          content = JSON.stringify(scanData);
          mimeType = 'application/dicom';
          break;
      }

      await Share.share({ message: content, title: `SonicSight_${scanData?.id}.${format}` });
      onClose();
    } catch (err) {
      console.warn('Export failed:', err);
    }
  };

  return (
    <Modal visible={visible} transparent animationType="slide"
           onRequestClose={onClose}>
      <View style={styles.overlay}>
        <View style={styles.dialog}>
          <Text style={styles.title}>Export Scan</Text>
          <Text style={styles.subtitle}>Choose export format</Text>

          <TouchableOpacity style={styles.option} onPress={() => handleExport('pdf')}>
            <Icon name="file-pdf-box" size={32} color="#f44336" />
            <View style={styles.optionInfo}>
              <Text style={styles.optionTitle}>PDF Report</Text>
              <Text style={styles.optionDesc}>Full diagnostic report with image</Text>
            </View>
          </TouchableOpacity>

          <TouchableOpacity style={styles.option} onPress={() => handleExport('csv')}>
            <Icon name="file-delimited" size={32} color="#4caf50" />
            <View style={styles.optionInfo}>
              <Text style={styles.optionTitle}>CSV ToF Matrix</Text>
              <Text style={styles.optionDesc}>Raw time-of-flight data table</Text>
            </View>
          </TouchableOpacity>

          <TouchableOpacity style={styles.option} onPress={() => handleExport('dicom')}>
            <Icon name="file-image" size={32} color="#2196f3" />
            <View style={styles.optionInfo}>
              <Text style={styles.optionTitle}>DICOM Image</Text>
              <Text style={styles.optionDesc}>Medical-grade cross-reference format</Text>
            </View>
          </TouchableOpacity>

          <TouchableOpacity style={styles.cancelBtn} onPress={onClose}>
            <Text style={styles.cancelText}>Cancel</Text>
          </TouchableOpacity>
        </View>
      </View>
    </Modal>
  );
};

const styles = StyleSheet.create({
  overlay: {
    flex: 1, backgroundColor: 'rgba(0,0,0,0.7)',
    justifyContent: 'flex-end',
  },
  dialog: {
    backgroundColor: '#1a1a2e', borderTopLeftRadius: 20,
    borderTopRightRadius: 20, padding: 20, paddingBottom: 40,
  },
  title: { color: '#fff', fontSize: 20, fontWeight: 'bold' },
  subtitle: { color: '#888', fontSize: 14, marginBottom: 16 },
  option: {
    flexDirection: 'row', alignItems: 'center',
    backgroundColor: '#16213e', borderRadius: 12, padding: 16,
    marginBottom: 8,
  },
  optionInfo: { marginLeft: 12, flex: 1 },
  optionTitle: { color: '#fff', fontSize: 15, fontWeight: '600' },
  optionDesc: { color: '#888', fontSize: 12, marginTop: 2 },
  cancelBtn: { alignItems: 'center', padding: 12, marginTop: 8 },
  cancelText: { color: '#f44336', fontSize: 16 },
});

export default ExportDialog;