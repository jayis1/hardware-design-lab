// utils/protocol.js — HydroFluor BLE sample record parser
// Author: jayis1  License: MIT
//
// Decodes the 24-byte binary sample record (BLE characteristic 0xFF11)
// into a structured JS object. Also encodes commands for the 0xFF02
// control characteristic.

// ---- GATT UUIDs (short, 16-bit) ----
export const UUIDS = {
  SERVICE_CONTROL:    '0000ff01-0000-1000-8000-00805f9b34fb',
  CHAR_CMD:           '0000ff02-0000-1000-8000-00805f9b34fb',
  CHAR_STATUS:        '0000ff03-0000-1000-8000-00805f9b34fb',
  SERVICE_DATA:       '0000ff10-0000-1000-8000-00805f9b34fb',
  CHAR_SAMPLE:        '0000ff11-0000-1000-8000-00805f9b34fb',
  SERVICE_CALIB:      '0000ff20-0000-1000-8000-00805f9b34fb',
  CHAR_REF:           '0000ff21-0000-1000-8000-00805f9b34fb',
  CHAR_RESULT:        '0000ff22-0000-1000-8000-00805f9b34fb',
  SERVICE_INFO:       '0000ff30-0000-1000-8000-00805f9b34fb',
  CHAR_INFO:          '0000ff31-0000-1000-8000-00805f9b34fb',
};

// ---- Sample record binary format (24 bytes, little-endian) ----
// See README §5.2 for the field table.
export function parseSample(buf) {
  // buf is an ArrayBuffer of 24 bytes
  const dv = new DataView(buf);
  if (buf.byteLength < 24) return null;
  return {
    seq:         dv.getUint16(0,  true),
    timestamp:   dv.getUint32(2,  true),
    depthCm:     dv.getUint16(6,  true),
    tempC01:     dv.getInt16(8,   true),
    batteryMv:   dv.getUint16(10, true),
    cdomPpb:     dv.getUint16(12, true),
    chlaUgl:     dv.getUint16(14, true),
    phycUgl:     dv.getUint16(16, true),
    oilPpb:      dv.getUint16(18, true),
    turbNtuX100: dv.getUint16(20, true),
    flags:       dv.getUint16(22, true),
  };
}

// ---- Derived accessors ----
export function tempC(rec)       { return rec.tempC01 / 100; }
export function depthM(rec)      { return rec.depthCm / 100; }
export function turbNtu(rec)     { return rec.turbNtuX100 / 100; }
export function batteryPct(rec)  {
  const v = rec.batteryMv;
  return Math.max(0, Math.min(100, ((v - 2400) / (3600 - 2400)) * 100));
}
export function hasOverrange(rec){ return (rec.flags & 0x1) !== 0; }
export function hasBubble(rec)   { return (rec.flags & 0x2) !== 0; }
export function hasCalWarn(rec)  { return (rec.flags & 0x4) !== 0; }

// ---- Command encoder (writes to CHAR_CMD as ASCII) ----
export const Commands = {
  START:    'START\n',
  STOP:     'STOP\n',
  PUMP_ON:  'PUMP ON\n',
  PUMP_OFF: 'PUMP OFF\n',
  CAL:      (analyte, ref) => `CAL ${analyte} ${ref}\n`,
  PERIOD:   (ms) => `PERIOD ${ms}\n`,
};

export function encodeCommand(cmd) {
  // TextEncoder isn't guaranteed in all RN runtimes; fall back to manual UTF-8.
  if (typeof TextEncoder !== 'undefined') {
    return new TextEncoder().encode(cmd);
  }
  const bytes = [];
  for (let i = 0; i < cmd.length; i++) {
    const c = cmd.charCodeAt(i);
    if (c < 0x80)       bytes.push(c);
    else if (c < 0x800) { bytes.push(0xc0 | (c >> 6), 0x80 | (c & 0x3f)); }
    else                { bytes.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f)); }
  }
  return new Uint8Array(bytes);
}

// ---- Calibration reference pair encoder (CHAR_REF) ----
// Binary frame: [analyte_id:1][ref_value:2 LE][24× raw int16 LE = 48 bytes]
export function encodeCalibRef(analyteId, refValue, raw24) {
  const buf = new ArrayBuffer(3 + 48);
  const dv  = new DataView(buf);
  dv.setUint8(0, analyteId);
  dv.setUint16(1, refValue, true);
  for (let i = 0; i < 24; ++i) {
    dv.setInt16(3 + i * 2, raw24[i], true);
  }
  return buf;
}

// ---- Device info parser (CHAR_INFO: ASCII "fw|serial|caldate") ----
export function parseDeviceInfo(text) {
  const [fw, serial, cal] = (text || '').split('|');
  return { fw: fw || '?', serial: serial || '?', calDate: Number(cal || 0) };
}