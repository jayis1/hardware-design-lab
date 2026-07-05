// ImageScreen.tsx — 3-D density volume viewer
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT

import React, { useState, useEffect, useRef } from 'react';
import { View, Text, Slider, StyleSheet, Button, ActivityIndicator } from 'react-native';
import { GLView } from 'expo-gl';
import { Renderer, THREE } from 'expo-three';
import DeviceService from '../services/DeviceService';
import { buildIsosurfaceGeometry, parseVolume, VOX_Z } from '../components/VolumeRenderer';

const styles = StyleSheet.create({
  container: { flex: 1, padding: 20, backgroundColor: '#0a0e27' },
  title: { color: '#e0e6ff', fontSize: 22, fontWeight: '700', marginBottom: 12 },
  label: { color: '#7c9eff', fontSize: 14, marginTop: 8 },
  value: { color: '#e0e6ff', fontSize: 14 },
  glView: { flex: 1, minHeight: 300, backgroundColor: '#000' },
  loading: { position: 'absolute', top: 0, left: 0, right: 0, bottom: 0,
             alignItems: 'center', justifyContent: 'center' },
});

export default function ImageScreen({ route }: any) {
  const [threshold, setThreshold] = useState(0.5);
  const [sliceZ, setSliceZ] = useState(-1);
  const [loading, setLoading] = useState(true);
  const [volume, setVolume] = useState<Float32Array | null>(null);
  const [fbpTracks, setFbpTracks] = useState(0);
  const rendererRef = useRef<Renderer | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const meshRef = useRef<THREE.Mesh | null>(null);

  useEffect(() => {
    const unsub = DeviceService.onStatus((s) => {
      setFbpTracks(s.fbp_tracks);
    });
    return () => unsub();
  }, []);

  const fetchVolume = async () => {
    setLoading(true);
    try {
      /* Request the volume over Wi-Fi TCP. The device streams 96*96*32*4 bytes. */
      const data = await DeviceService.downloadFile('volume.bin');
      const vol = parseVolume(data);
      setVolume(vol);
      rebuildScene(vol, threshold, sliceZ);
    } catch (e: any) {
      console.warn('Volume fetch failed:', e);
    }
    setLoading(false);
  };

  useEffect(() => { fetchVolume(); }, []);

  const rebuildScene = (vol: Float32Array, thr: number, slice: number) => {
    if (!sceneRef.current) return;
    if (meshRef.current) {
      sceneRef.current.remove(meshRef.current);
      meshRef.current.geometry.dispose();
    }
    const geom = buildIsosurfaceGeometry(vol, { threshold: thr, sliceZ: slice, colorMap: 'viridis' });
    const mat = new THREE.MeshStandardMaterial({ vertexColors: true,
      transparent: true, opacity: 0.85, roughness: 0.5, metalness: 0.1 });
    meshRef.current = new THREE.Mesh(geom, mat);
    sceneRef.current.add(meshRef.current);
  };

  const onGLContextCreate = async (gl: any) => {
    const renderer = new Renderer({ gl });
    rendererRef.current = renderer;
    const scene = new THREE.Scene();
    sceneRef.current = scene;
    const camera = new THREE.PerspectiveCamera(45, gl.drawingBufferWidth / gl.drawingBufferHeight, 0.1, 1000);
    camera.position.set(150, 150, 200);
    camera.lookAt(0, 0, 0);

    const ambient = new THREE.AmbientLight(0x404060, 1.0);
    scene.add(ambient);
    const dir = new THREE.DirectionalLight(0xffffff, 1.2);
    dir.position.set(100, 100, 200);
    scene.add(dir);

    if (volume) rebuildScene(volume, threshold, sliceZ);

    const render = () => {
      renderer.render(scene, camera);
      gl.endFrameEXP();
      requestAnimationFrame(render);
    };
    render();
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Density Volume</Text>
      <Text style={styles.value}>FBP tracks accumulated: {fbpTracks}</Text>

      <View style={styles.glView}>
        <GLView style={{ flex: 1 }} onContextCreate={onGLContextCreate} />
        {loading && (
          <View style={styles.loading}>
            <ActivityIndicator size="large" color="#7c9eff" />
            <Text style={{ color: '#7c9eff', marginTop: 8 }}>Loading volume...</Text>
          </View>
        )}
      </View>

      <Text style={styles.label}>Density threshold: {threshold.toFixed(2)}</Text>
      <Slider minimumValue={0} maximumValue={1} value={threshold}
        onValueChange={(v) => { setThreshold(v);
          if (volume) rebuildScene(volume, v, sliceZ); }} />

      <Text style={styles.label}>Z-slice: {sliceZ < 0 ? 'all' : sliceZ}</Text>
      <Slider minimumValue={-1} maximumValue={VOX_Z - 1} step={1} value={sliceZ}
        onValueChange={(v) => { setSliceZ(v);
          if (volume) rebuildScene(volume, threshold, v); }} />

      <Button title="Refresh Volume" onPress={fetchVolume} />
    </View>
  );
}