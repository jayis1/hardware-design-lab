/*
 * EpisodeModel.js — Tremor episode data model
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

export default class Episode {
  constructor(data = {}) {
    this.id = data.id || Date.now();
    this.timestamp = data.timestamp || 0;
    this.duration = data.duration || 0;          // milliseconds
    this.frequency = data.frequency || 0;        // Hz
    this.amplitude = data.amplitude || 0;        // g (RMS)
    this.tremorClass = data.tremorClass || 0;     // 0-4
    this.context = data.context || 0;            // 0=rest, 1=postural, 2=action
    this.confidence = data.confidence || 0;      // 0.0-1.0
    this.samples = data.samples || [];           // raw IMU samples
  }

  get className() {
    const names = ['Parkinsonian', 'Essential', 'Cerebellar', 'Physiological', 'Drug-Induced'];
    return names[this.tremorClass] || 'Unknown';
  }

  get contextName() {
    return ['Rest', 'Postural', 'Action'][this.context] || 'Unknown';
  }

  get durationMinutes() {
    return this.duration / 60000;
  }

  get severity() {
    if (this.amplitude < 0.01) return 'minimal';
    if (this.amplitude < 0.05) return 'mild';
    if (this.amplitude < 0.1) return 'moderate';
    return 'severe';
  }

  toJSON() {
    return {
      id: this.id,
      timestamp: this.timestamp,
      duration: this.duration,
      frequency: this.frequency,
      amplitude: this.amplitude,
      tremorClass: this.tremorClass,
      context: this.context,
      confidence: this.confidence,
    };
  }

  static fromBinary(data) {
    // Parse binary episode record from BLE/flash
    if (data.length < 26) return null;

    const view = new DataView(data.buffer || data);
    const magic = view.getUint32(0, true);
    if (magic !== 0x54530301) return null;

    return new Episode({
      timestamp: view.getUint32(4, true) * 1000,
      duration: view.getUint16(8, true),
      frequency: view.getFloat32(12, true),
      amplitude: view.getFloat32(16, true),
      tremorClass: data[20],
      context: data[21],
      confidence: data[22] / 100.0,
    });
  }
}