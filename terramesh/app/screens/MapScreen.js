/**
 * Terramesh — Map Screen
 * Author: jayis1
 * Copyright © 2026 jayis1
 * License: MIT
 *
 * Map view showing all deployed Terramesh nodes with color-coded
 * classification status markers. Supports pinch-to-zoom and tap
 * for node details.
 */

import React, { useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Dimensions,
  TouchableOpacity,
} from 'react-native';
import { useDevices } from '../components/DeviceContext';
import { protocol } from '../utils/protocol';

const { width, height } = Dimensions.get('window');

// Simplified map view (in production, uses react-native-maps)
const MapScreen = ({ navigation }) => {
  const { nodes } = useDevices();
  const [selectedNode, setSelectedNode] = useState(null);

  const nodeList = Object.values(nodes);

  // Generate grid positions for demo (in production, uses real GPS coords)
  const getNodePosition = (index, total) => {
    const cols = Math.ceil(Math.sqrt(total));
    const rows = Math.ceil(total / cols);
    const x = (index % cols) * (width - 40) / cols + 20;
    const y = Math.floor(index / cols) * (height - 200) / rows + 100;
    return { x, y };
  };

  return (
    <View style={styles.container}>
      {/* Map area (simplified — uses react-native-maps in production) */}
      <View style={styles.mapArea}>
        <Text style={styles.mapPlaceholder}>Site Map</Text>
        <Text style={styles.mapSubtext}>
          {nodeList.length} nodes deployed
        </Text>

        {/* Node markers */}
        {nodeList.map((node, index) => {
          const pos = getNodePosition(index, nodeList.length);
          const clsColor = protocol.classificationColor(node.classification);

          return (
            <TouchableOpacity
              key={node.id}
              style={[styles.marker, {
                left: pos.x - 15,
                top: pos.y - 15,
                backgroundColor: clsColor,
              }]}
              onPress={() => setSelectedNode(
                selectedNode?.id === node.id ? null : node
              )}
            >
              <Text style={styles.markerText}>
                {node.id.split('-')[1]}
              </Text>
            </TouchableOpacity>
          );
        })}

        {/* Gateway marker */}
        <View style={[styles.marker, styles.gatewayMarker, {
          left: width / 2 - 15,
          top: 50,
        }]}>
          <Text style={styles.markerText}>GW</Text>
        </View>
      </View>

      {/* Selected node info panel */}
      {selectedNode && (
        <View style={styles.infoPanel}>
          <View style={styles.infoHeader}>
            <Text style={styles.infoTitle}>{selectedNode.id}</Text>
            <View style={[styles.statusBadge, {
              backgroundColor: protocol.classificationColor(selectedNode.classification)
            }]}>
              <Text style={styles.statusText}>
                {protocol.classificationToString(selectedNode.classification)}
              </Text>
            </View>
          </View>

          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Tilt X/Y</Text>
            <Text style={styles.infoValue}>
              {selectedNode.sensors.tiltX.toFixed(3)}° / {selectedNode.sensors.tiltY.toFixed(3)}°
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Pore Pressure</Text>
            <Text style={styles.infoValue}>
              S: {selectedNode.sensors.porePressureShallow.toFixed(1)} kPa
              {'  '}D: {selectedNode.sensors.porePressureDeep.toFixed(1)} kPa
            </Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Moisture</Text>
            <Text style={styles.infoValue}>{selectedNode.sensors.moisture.toFixed(1)}%</Text>
          </View>
          <View style={styles.infoRow}>
            <Text style={styles.infoLabel}>Battery</Text>
            <Text style={styles.infoValue}>
              {Math.min(100, Math.round((selectedNode.battery / 7200) * 100))}%
            </Text>
          </View>

          <TouchableOpacity
            style={styles.detailButton}
            onPress={() => navigation.navigate('Dashboard', {
              screen: 'NodeDetail',
              params: { nodeId: selectedNode.id }
            })}
          >
            <Text style={styles.detailButtonText}>View Details</Text>
          </TouchableOpacity>
        </View>
      )}

      {/* Legend */}
      <View style={styles.legend}>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#00d4aa' }]} />
          <Text style={styles.legendText}>Normal</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#ffa500' }]} />
          <Text style={styles.legendText}>Warning</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#ff4444' }]} />
          <Text style={styles.legendText}>Critical</Text>
        </View>
        <View style={styles.legendItem}>
          <View style={[styles.legendDot, { backgroundColor: '#4fc3f7' }]} />
          <Text style={styles.legendText}>Gateway</Text>
        </View>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f1a',
  },
  mapArea: {
    flex: 1,
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    overflow: 'hidden',
  },
  mapPlaceholder: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#e0e0e0',
    textAlign: 'center',
    paddingTop: 20,
  },
  mapSubtext: {
    fontSize: 14,
    color: '#6c6c80',
    textAlign: 'center',
  },
  marker: {
    position: 'absolute',
    width: 30,
    height: 30,
    borderRadius: 15,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 2,
    borderColor: '#ffffff',
    elevation: 5,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.3,
    shadowRadius: 4,
  },
  gatewayMarker: {
    backgroundColor: '#4fc3f7',
    width: 36,
    height: 36,
    borderRadius: 18,
  },
  markerText: {
    fontSize: 9,
    fontWeight: 'bold',
    color: '#ffffff',
  },
  infoPanel: {
    backgroundColor: '#1a1a2e',
    margin: 10,
    borderRadius: 12,
    padding: 16,
    borderWidth: 1,
    borderColor: '#2a2a3e',
  },
  infoHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 12,
  },
  infoTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#e0e0e0',
  },
  statusBadge: {
    paddingHorizontal: 10,
    paddingVertical: 4,
    borderRadius: 12,
  },
  statusText: {
    fontSize: 12,
    fontWeight: '600',
    color: '#ffffff',
  },
  infoRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#2a2a3e',
  },
  infoLabel: {
    fontSize: 13,
    color: '#6c6c80',
  },
  infoValue: {
    fontSize: 13,
    color: '#e0e0e0',
    fontWeight: '500',
  },
  detailButton: {
    backgroundColor: '#00d4aa',
    borderRadius: 8,
    padding: 12,
    marginTop: 12,
    alignItems: 'center',
  },
  detailButtonText: {
    fontSize: 14,
    fontWeight: 'bold',
    color: '#0f0f1a',
  },
  legend: {
    flexDirection: 'row',
    justifyContent: 'center',
    padding: 10,
    gap: 16,
  },
  legendItem: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  legendDot: {
    width: 10,
    height: 10,
    borderRadius: 5,
    marginRight: 4,
  },
  legendText: {
    fontSize: 11,
    color: '#6c6c80',
  },
});

export default MapScreen;
