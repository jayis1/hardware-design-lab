/**
 * binaryToCsv.js — MycoMesh SD log binary → CSV converter
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Converts the MycoMesh custom binary log format (written by sdlog.c)
 * to CSV files for analysis in Python (neo, pandas), R, or spreadsheet
 * tools.  Also provides a WAV export option for audio-domain analysis
 * of fungal electrical activity.
 *
 * Binary format:
 *   Block 0: Header [magic(4) | version(1) | n_channels(1) | sample_rate(2) | ...]
 *   Block N+: Data blocks:
 *     [magic(4) | version(2) | timestamp(4) | n_channels(2) | sample_rate(2) | n_samples(2)]
 *     [n_channels × n_samples × 2 bytes (int16_t samples)]
 *     [CRC16(2)]
 *   Epoch records: [0xEE marker(1) | epoch_summary_t(24)]
 */

const HEADER_MAGIC = 0x4D434D53;  // "MCMS"
const DATA_MAGIC = 0x4D434D53;
const EPOCH_MARKER = 0xEE;

// CRC16-CCITT (same as firmware).
function crc16(data) {
  let crc = 0;
  for (const byte of data) {
    crc ^= byte;
    for (let b = 0; b < 16; b++) {
      if (crc & 1) crc = (crc >> 1) ^ 0x8408;
      else crc >>= 1;
    }
  }
  return crc & 0xFFFF;
}

function readUInt32LE(data, offset) {
  return ((data[offset] | (data[offset + 1] << 8) |
           (data[offset + 2] << 16) | (data[offset + 3] << 24)) >>> 0);
}

function readUInt16LE(data, offset) {
  return data[offset] | (data[offset + 1] << 8);
}

function readInt16LE(data, offset) {
  const v = data[offset] | (data[offset + 1] << 8);
  return v > 0x7FFF ? v - 0x10000 : v;
}

/**
 * Parse a MycoMesh binary log file.
 * @param {Uint8Array} data - Raw binary file data
 * @returns {Object} Parsed log: { header, blocks: [], epochs: [] }
 */
export function parseLog(data) {
  const result = { header: null, blocks: [], epochs: [] };

  if (data.length < 512) {
    result.error = 'File too small (no header block)';
    return result;
  }

  // Parse header block (block 0).
  const magic = readUInt32LE(data, 0);
  if (magic !== HEADER_MAGIC) {
    result.error = 'Invalid magic number';
    return result;
  }

  result.header = {
    magic,
    version: data[4],
    nChannels: data[5],
    sampleRate: readUInt16LE(data, 6),
    author: String.fromCharCode(...data.slice(8, 14)),
    deviceName: String.fromCharCode(...data.slice(16, 24)),
  };

  // Parse data blocks starting at block 1 (offset 512).
  let offset = 512;
  while (offset + 16 < data.length) {
    // Check for epoch record marker.
    if (data[offset] === EPOCH_MARKER) {
      const epoch = parseEpochRecord(data, offset + 1);
      if (epoch) {
        result.epochs.push(epoch);
        offset += 1 + 24;
        continue;
      }
    }

    // Parse data block header.
    const blockMagic = readUInt32LE(data, offset);
    if (blockMagic !== DATA_MAGIC) {
      // Skip to next 512-byte boundary (padding).
      offset = Math.ceil((offset + 1) / 512) * 512;
      if (offset >= data.length) break;
      continue;
    }

    const version = readUInt16LE(data, offset + 4);
    const timestamp = readUInt32LE(data, offset + 6);
    const nChannels = readUInt16LE(data, offset + 10);
    const sampleRate = readUInt16LE(data, offset + 12);
    const nSamples = readUInt16LE(data, offset + 14);
    const headerSize = 16;
    const dataSize = nChannels * nSamples * 2;
    const crcOffset = offset + headerSize + dataSize;

    if (crcOffset + 2 > data.length) break;

    // Extract samples.
    const samples = [];
    for (let s = 0; s < nSamples; s++) {
      const row = [];
      for (let ch = 0; ch < nChannels; ch++) {
        const sampleOffset = offset + headerSize + (s * nChannels + ch) * 2;
        row.push(readInt16LE(data, sampleOffset));
      }
      samples.push(row);
    }

    result.blocks.push({ version, timestamp, nChannels, sampleRate, nSamples, samples });
    offset = crcOffset + 2;
  }

  return result;
}

// Parse an epoch summary record (24 bytes).
function parseEpochRecord(data, offset) {
  if (offset + 24 > data.length) return null;
  return {
    classLabel: data[offset],
    spikeRate: data[offset + 1],
    propagationCount: data[offset + 2],
    meanAmplitudeUv: readUInt16LE(data, offset + 4),
    isiCvX100: readUInt16LE(data, offset + 6),
    soilMoisture: readInt16LE(data, offset + 8) / 10.0,
    soilTempC: readInt16LE(data, offset + 10) / 10.0,
    co2Ppm: readUInt16LE(data, offset + 12),
    batteryMv: readUInt16LE(data, offset + 14),
    timestamp: readUInt32LE(data, offset + 16),
  };
}

/**
 * Convert parsed log to CSV string.
 * @param {Object} log - Parsed log from parseLog()
 * @returns {string} CSV data
 */
export function logToCsv(log) {
  const lines = [];

  // Header comment.
  lines.push(`# MycoMesh Log - Author: jayis1 - License: GPL-2.0`);
  lines.push(`# Device: ${log.header?.deviceName || 'unknown'}`);
  lines.push(`# Channels: ${log.header?.nChannels || 16}`);
  lines.push(`# Sample Rate: ${log.header?.sampleRate || 1000} Hz`);
  lines.push('');

  // Data blocks.
  const channelNames = Array.from({ length: log.header?.nChannels || 16 },
    (_, i) => `ch${i}`);
  lines.push(['timestamp_ms', ...channelNames].join(','));

  for (const block of log.blocks) {
    for (let s = 0; s < block.nSamples; s++) {
      const ts = block.timestamp + s;
      const vals = block.samples[s].join(',');
      lines.push([ts, vals].join(','));
    }
  }

  // Epoch summaries.
  lines.push('');
  lines.push('# Epoch Summaries');
  lines.push('timestamp,class_label,spike_rate,prop_count,mean_amp_uv,isi_cv,moisture_pct,temp_c,co2_ppm,battery_mv');

  for (const epoch of log.epochs) {
    const labels = ['IDLE', 'FORAGE', 'TRANSPORT', 'STRESS', 'COMPETE'];
    const label = labels[epoch.classLabel] || 'UNKNOWN';
    lines.push([
      epoch.timestamp,
      label,
      epoch.spikeRate,
      epoch.propagationCount,
      epoch.meanAmplitudeUv,
      (epoch.isiCvX100 / 100).toFixed(2),
      epoch.soilMoisture.toFixed(1),
      epoch.soilTempC.toFixed(1),
      epoch.co2Ppm,
      epoch.batteryMv,
    ].join(','));
  }

  return lines.join('\n');
}

/**
 * Convert parsed log to WAV file data (for audio analysis).
 * All channels are interleaved at the original sample rate.
 * @param {Object} log - Parsed log from parseLog()
 * @returns {Uint8Array} WAV file data (16-bit PCM)
 */
export function logToWav(log) {
  const sampleRate = log.header?.sampleRate || 1000;
  const nChannels = log.header?.nChannels || 16;

  // Collect all samples.
  const allSamples = [];
  for (const block of log.blocks) {
    for (const row of block.samples) {
      allSamples.push(...row);
    }
  }

  const nSamples = allSamples.length / nChannels;
  const dataSize = nSamples * nChannels * 2;

  // WAV header (44 bytes).
  const header = new Uint8Array(44);
  const view = new DataView(header.buffer);

  // RIFF header.
  view.setUint32(0, 0x52494646, false);  // "RIFF"
  view.setUint32(4, 36 + dataSize, true);
  view.setUint32(8, 0x57415645, false);  // "WAVE"

  // fmt chunk.
  view.setUint32(12, 0x666d7420, false);  // "fmt "
  view.setUint32(16, 16, true);           // chunk size
  view.setUint16(20, 1, true);            // PCM format
  view.setUint16(22, nChannels, true);
  view.setUint32(24, sampleRate, true);
  view.setUint32(28, sampleRate * nChannels * 2, true);  // byte rate
  view.setUint16(32, nChannels * 2, true);               // block align
  view.setUint16(34, 16, true);                          // bits per sample

  // data chunk.
  view.setUint32(36, 0x64617461, false);  // "data"
  view.setUint32(40, dataSize, true);

  // Write sample data.
  const wav = new Uint8Array(44 + dataSize);
  wav.set(header, 0);
  const dataView = new DataView(wav.buffer, 44);
  for (let i = 0; i < allSamples.length; i++) {
    dataView.setInt16(i * 2, allSamples[i], true);
  }

  return wav;
}