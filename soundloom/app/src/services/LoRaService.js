/**
 * LoRaService.js — LoRaWAN gateway data ingestion service
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Receives uplinks from SoundLoom pods via a LoRaWAN gateway (TTN/Chirpstack
 * HTTP integration or MQTT). Parses the 51-byte SoundLoom uplink payload
 * and dispatches it to the Redux store.
 */

const UPLINK_PORT = 1;
const UPLINK_LEN = 51;

// Class names matching the firmware classifier
const CLASS_NAMES = [
  'root_growth', 'root_hydraulic', 'earthworm_burrow', 'earthworm_cast',
  'arthropod_click', 'grub_chew', 'microbe_ebullition', 'water_infiltration',
  'compaction_crack', 'noise'
];

// Alert flag bits
const FLAG_PEST = 0x0001;
const FLAG_COMPACTION = 0x0002;
const FLAG_LOW_BATTERY = 0x0004;
const FLAG_TILT = 0x0008;

class LoRaService {
  constructor() {
    this.mqttClient = null;
    this.uplinkCallback = null;
    this.pollTimer = null;
    this.gatewayUrl = null;
    this.apiKey = null;
  }

  async initialise() {
    // In production: load gateway config from Redux/AsyncStorage
    // For now, set up a polling fallback
    return Promise.resolve();
  }

  /**
   * Configure the LoRaWAN gateway connection.
   * @param {string} url - Gateway HTTP integration URL
   * @param {string} apiKey - API key for authentication
   */
  configure(url, apiKey) {
    this.gatewayUrl = url;
    this.apiKey = apiKey;
  }

  /**
   * Start polling the gateway for new uplinks.
   * In production this would use MQTT subscription instead of polling.
   */
  startPolling(intervalMs = 60000) {
    if (this.pollTimer) clearInterval(this.pollTimer);
    this.pollTimer = setInterval(() => this._pollGateway(), intervalMs);
  }

  stopPolling() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
  }

  /**
   * Register a callback for received uplinks.
   */
  onUplink(callback) {
    this.uplinkCallback = callback;
  }

  /**
   * Parse a 51-byte SoundLoom uplink payload.
   * Format defined in firmware/lorawan.c:lora_build_uplink().
   */
  parseUplink(payload) {
    if (!payload || payload.length < UPLINK_LEN) return null;

    const bytes = (payload instanceof Uint8Array) ? payload : new Uint8Array(payload);
    let idx = 0;

    const svi = bytes[idx++];

    const eventCounts = [];
    for (let i = 0; i < 10; i++) {
      eventCounts.push((bytes[idx++] << 8) | bytes[idx++]);
    }

    const moisture = [];
    for (let i = 0; i < 2; i++) {
      moisture.push(((bytes[idx++] << 8) | bytes[idx++]) / 100.0);
    }

    const ec = [];
    for (let i = 0; i < 2; i++) {
      ec.push((bytes[idx++] << 8) | bytes[idx++]);
    }

    const depthTemp = [];
    for (let i = 0; i < 4; i++) {
      const raw = (bytes[idx++] << 8) | bytes[idx++];
      const signed = raw > 32767 ? raw - 65536 : raw;
      depthTemp.push(signed / 100.0);
    }

    const co2ppm = (bytes[idx++] << 8) | bytes[idx++];
    const batteryMv = (bytes[idx++] << 8) | bytes[idx++];

    const flags = (bytes[idx++] << 8) | bytes[idx++];
    const seq = (bytes[idx++] << 8) | bytes[idx++];

    // CRC (last 2 bytes, not verified here)
    idx += 2;

    return {
      svi,
      eventCounts,
      moisture,
      ec,
      depthTemp,
      co2ppm,
      batteryMv,
      flags,
      seq,
      alerts: {
        pest: !!(flags & FLAG_PEST),
        compaction: !!(flags & FLAG_COMPACTION),
        lowBattery: !!(flags & FLAG_LOW_BATTERY),
        tilt: !!(flags & FLAG_TILT),
      },
      eventRates: eventCounts.map((c, i) => ({
        class: CLASS_NAMES[i],
        classId: i,
        count: c,
        isPest: i === 5,
        isCompaction: i === 8,
        isBiological: [0, 1, 2, 3, 4, 6].includes(i),
      })),
    };
  }

  /**
   * Poll the gateway for new uplinks (HTTP integration).
   */
  async _pollGateway() {
    if (!this.gatewayUrl) return;
    try {
      const response = await fetch(`${this.gatewayUrl}/uplinks`, {
        headers: { 'Authorization': `Bearer ${this.apiKey}` },
      });
      const data = await response.json();
      if (data.uplinks) {
        for (const uplink of data.uplinks) {
          if (uplink.port === UPLINK_PORT && uplink.payload) {
            const parsed = this.parseUplink(uplink.payload);
            if (parsed && this.uplinkCallback) {
              this.uplinkCallback({
                podId: uplink.devEUI,
                timestamp: uplink.receivedAt || Date.now(),
                ...parsed,
              });
            }
          }
        }
      }
    } catch (error) {
      console.error('SoundLoom LoRa poll error:', error);
    }
  }

  /**
   * Generate a demo uplink for testing/development.
   */
  generateDemoUplink(podId) {
    const payload = new Uint8Array(UPLINK_LEN);
    let idx = 0;
    payload[idx++] = Math.floor(50 + Math.random() * 40); // SVI 50-90

    // Event counts (10 classes)
    for (let i = 0; i < 10; i++) {
      const count = Math.floor(Math.random() * 50);
      payload[idx++] = (count >> 8) & 0xFF;
      payload[idx++] = count & 0xFF;
    }

    // Moisture ×2
    for (let i = 0; i < 2; i++) {
      const m = Math.floor((20 + Math.random() * 30) * 100);
      payload[idx++] = (m >> 8) & 0xFF;
      payload[idx++] = m & 0xFF;
    }

    // EC ×2
    for (let i = 0; i < 2; i++) {
      const e = Math.floor(50 + Math.random() * 200);
      payload[idx++] = (e >> 8) & 0xFF;
      payload[idx++] = e & 0xFF;
    }

    // Depth temps ×4
    for (let i = 0; i < 4; i++) {
      const t = Math.round((10 + Math.random() * 15 - i * 2) * 100);
      const signed = t < 0 ? 65536 + t : t;
      payload[idx++] = (signed >> 8) & 0xFF;
      payload[idx++] = signed & 0xFF;
    }

    // CO2
    const co2 = Math.floor(400 + Math.random() * 800);
    payload[idx++] = (co2 >> 8) & 0xFF;
    payload[idx++] = co2 & 0xFF;

    // Battery
    const bat = Math.floor(3300 + Math.random() * 370);
    payload[idx++] = (bat >> 8) & 0xFF;
    payload[idx++] = bat & 0xFF;

    // Flags
    payload[idx++] = 0;
    payload[idx++] = 0;

    // Sequence
    payload[idx++] = 0;
    payload[idx++] = Math.floor(Math.random() * 255);

    // CRC
    payload[idx++] = 0;
    payload[idx++] = 0;

    return this.parseUplink(payload);
  }
}

export default new LoRaService();