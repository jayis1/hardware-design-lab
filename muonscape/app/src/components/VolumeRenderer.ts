// VolumeRenderer.ts — 3-D volume renderer using Three.js for the density view
// Author: jayis1
// Copyright (c) 2026 jayis1. All rights reserved.
// License: MIT
//
// Builds a Three.js scene from a downloaded 3-D density volume (float32
// array of VOX_X * VOX_Y * VOX_Z voxels). Renders as a colored isosurface
// with a slider for density threshold and slice planes.

import * as THREE from 'three';

export const VOX_X = 96;
export const VOX_Y = 96;
export const VOX_Z = 32;
export const VOX_SIZE_CM = 3.0;

export interface RenderOptions {
  threshold: number;     /* density threshold 0..1 */
  sliceZ: number;        /* -1 = no slice, else 0..VOX_Z-1 */
  colorMap: 'viridis' | 'grayscale' | 'hot';
}

/** Build a THREE.BufferGeometry from the volume above the threshold. */
export function buildIsosurfaceGeometry(volume: Float32Array,
                                         opts: RenderOptions): THREE.BufferGeometry {
  const positions: number[] = [];
  const colors: number[] = [];
  const indices: number[] = [];

  /* March through voxels and create a small cube for each voxel above
   * threshold. This is a point-cloud-of-cubes approach; a proper
   * marching-cubes implementation would be smoother but larger. */
  const t = opts.threshold;
  let vidx = 0;
  for (let k = 0; k < VOX_Z; ++k) {
    if (opts.sliceZ >= 0 && k !== opts.sliceZ) continue;
    for (let j = 0; j < VOX_Y; ++j) {
      for (let i = 0; i < VOX_X; ++i) {
        const idx = i * VOX_Y * VOX_Z + j * VOX_Z + k;
        const v = volume[idx];
        const nv = Math.max(0, Math.min(1, v));
        if (nv < t) continue;
        const x = (i - VOX_X / 2) * VOX_SIZE_CM;
        const y = (j - VOX_Y / 2) * VOX_SIZE_CM;
        const z = (k - VOX_Z / 2) * VOX_SIZE_CM;
        const s = VOX_SIZE_CM / 2;
        /* 8 corners of a cube */
        const corners = [
          [x - s, y - s, z - s], [x + s, y - s, z - s],
          [x + s, y + s, z - s], [x - s, y + s, z - s],
          [x - s, y - s, z + s], [x + s, y - s, z + s],
          [x + s, y + s, z + s], [x - s, y + s, z + s],
        ];
        for (const c of corners) {
          positions.push(c[0], c[1], c[2]);
          const [r, g, b] = colorMap(nv, opts.colorMap);
          colors.push(r, g, b);
        }
        /* 12 triangles (2 per face) */
        const faces = [
          [0,1,2],[0,2,3], [4,5,6],[4,6,7],
          [0,1,5],[0,5,4], [2,3,7],[2,7,6],
          [0,3,7],[0,7,4], [1,2,6],[1,6,5],
        ];
        for (const f of faces) {
          indices.push(vidx + f[0], vidx + f[1], vidx + f[2]);
        }
        vidx += 8;
      }
    }
  }

  const geom = new THREE.BufferGeometry();
  geom.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
  geom.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
  geom.setIndex(indices);
  geom.computeVertexNormals();
  return geom;
}

function colorMap(v: number, map: string): [number, number, number] {
  if (map === 'grayscale') {
    return [v, v, v];
  }
  if (map === 'hot') {
    const r = Math.min(1, v * 2);
    const g = Math.max(0, Math.min(1, v * 2 - 0.5));
    const b = Math.max(0, v * 2 - 1.5);
    return [r, g, b];
  }
  /* viridis (approximate) */
  const r = 0.267 * (1 - v) + 0.993 * v;
  const g = 0.005 * (1 - v) + 0.906 * v;
  const b = 0.329 * (1 - v) + 0.143 * v;
  return [r, g, b];
}

/** Parse a binary blob (from device download) into a Float32Array. */
export function parseVolume(data: Uint8Array): Float32Array {
  const n = VOX_X * VOX_Y * VOX_Z;
  const buf = new ArrayBuffer(n * 4);
  const view = new DataView(buf);
  for (let i = 0; i < n && i * 4 + 3 < data.length; ++i) {
    view.setFloat32(i * 4, data[i * 4] | (data[i * 4 + 1] << 8) |
      (data[i * 4 + 2] << 16) | (data[i * 4 + 3] << 24), true);
  }
  return new Float32Array(buf);
}