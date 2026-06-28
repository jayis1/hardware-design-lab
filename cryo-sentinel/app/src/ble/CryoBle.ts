/*
 * src/ble/CryoBle.ts — BLE interface for commissioning Cryo-Sentinel.
 *
 * Wraps react-native-ble-plx with the Cryo-Sentinel GATT service contract:
 *   Service:  0000C5A1-0000-1000-8000-00805F9B34FB
 *   Char #1:  0000C5A2-...  (read/notify)  — live reading
 *   Char #2:  0000C5A3-...  (write)         — calibration capture command
 *   Char #3:  0000C5A4-...  (write)         — threshold config
 *   Char #4:  0000C5A5-...  (read)          — audit log tail
 *
 * Author: jayis1
 * License: MIT
 */
import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { DewarReading, CalibrationPoint } from '../types';

export const CS_SERVICE_UUID       = '0000C5A1-0000-1000-8000-00805F9B34FB';
export const CS_CHAR_LIVE_UUID     = '0000C5A2-0000-1000-8000-00805F9B34FB';
export const CS_CHAR_CAL_CMD_UUID  = '0000C5A3-0000-1000-8000-00805F9B34FB';
export const CS_CHAR_THRESH_UUID   = '0000C5A4-0000-1000-8000-00805F9B34FB';
export const CS_CHAR_LOG_TAIL_UUID = '0000C5A5-0000-1000-8000-00805F9B34FB';

const manager = new BleManager();

/* Scan for Cryo-Sentinel devices advertising the CS service UUID. */
export async function scanForSentinels(timeoutMs = 8000): Promise<Device[]> {
  const found: Device[] = [];
  return new Promise((resolve) => {
    manager.startDeviceScan([CS_SERVICE_UUID], null, (err, dev) => {
      if (err) { console.warn('BLE scan err', err); resolve([]); return; }
      if (dev && !found.find((d) => d.id === dev.id)) found.push(dev);
    });
    setTimeout(() => {
      manager.stopDeviceScan();
      resolve(found);
    }, timeoutMs);
  });
}

/* Connect to a device and return a live-reading subscription callback. */
export async function connectAndSubscribe(
  deviceId: string,
  onReading: (r: DewarReading) => void
): Promise<Device> {
  const dev = await manager.connectToDevice(deviceId);
  await dev.discoverAllServicesAndCharacteristics();
  const ch = await dev.readCharacteristicForService(
    CS_SERVICE_UUID, CS_CHAR_LIVE_UUID);
  await dev.setupNotification(CS_CHAR_LIVE_UUID).then((sub) =>
    sub.on('characteristic', (c: Characteristic | null) => {
      if (c?.value) onReading(parseLivePayload(c.value, deviceId));
    })
  );
  // also push the initial read immediately
  if (ch.value) onReading(parseLivePayload(ch.value, deviceId));
  return dev;
}

/* Parse the 32-byte live-reading GATT payload into a DewarReading. */
function parseLivePayload(b64: string, deviceId: string): DewarReading {
  const buf = Buffer.from(b64, 'base64');
  // Layout (matches cs_heartbeat_t in firmware/cs_radio.h):
  //  u32 seq         [0..4]
  //  u32 epoch_min   [4..8]
  //  u8  state       [8]
  //  u8  reason      [9]
  //  f32 level_pct   [10..14]
  //  f32 level_rate  [14..18]
  //  f32 rtd[3]      [18..30]
  //  f32 gradient    [30..34]
  //  f32 tilt        [34..38]
  //  f32 vib         [38..42]
  //  f32 ambient     [42..46]
  //  f32 ambient_rh  [46..50]
  //  f32 batt_pct    [50..54]
  //  u8  flags       [54]   (mains|lid|enc bits)
  const dv = new DataView(buf.buffer);
  const stateIdx = dv.getUint8(8);
  const reasonIdx = dv.getUint8(9);
  const flags = dv.getUint8(54);
  const states = ['OK', 'WATCH', 'WARN', 'CRITICAL'] as const;
  const reasons = [
    'none', 'level_low_warn', 'level_low_crit', 'level_drop_rate',
    'gradient_flat', 'lid_open', 'tilt_warn', 'tilt_crit',
    'ambient_hot', 'mains_lost', 'battery_crit', 'enclosure_open',
    'probe_removed', 'sensor_fault',
  ];
  return {
    dewarId: deviceId,
    label: '',
    state: states[stateIdx] ?? 'OK',
    reason: reasons[reasonIdx] ?? 'none',
    levelPct: dv.getFloat32(10, true),
    levelRatePerHour: dv.getFloat32(14, true),
    rtdTopC: dv.getFloat32(18, true),
    rtdMidC: dv.getFloat32(22, true),
    rtdBotC: dv.getFloat32(26, true),
    gradientSlope: dv.getFloat32(30, true),
    tiltDeg: dv.getFloat32(34, true),
    vibRmsG: dv.getFloat32(38, true),
    ambientC: dv.getFloat32(42, true),
    ambientRh: dv.getFloat32(46, true),
    battPct: dv.getFloat32(50, true),
    mainsPresent: !!(flags & 0x01),
    lidOpen: !!(flags & 0x02),
    enclosureOpen: !!(flags & 0x04),
    lastHeartbeatEpoch: Date.now() / 1000,
    serial: '',
  };
}

/* Send a calibration capture command to the connected device.
 * pointIdx is 0..3 corresponding to the four anchor points. */
export async function sendCalibrationCapture(
  device: Device, pointIdx: number
): Promise<CalibrationPoint> {
  const cmd = Buffer.alloc(2);
  cmd.writeUInt8(0x01, 0);        // capture opcode
  cmd.writeUInt8(pointIdx, 1);
  await device.writeCharacteristicWithResponseForService(
    CS_SERVICE_UUID, CS_CHAR_CAL_CMD_UUID, cmd.toString('base64'));
  // The capture result comes back on the live characteristic; the caller
  // is expected to read it next. For the app flow we just read the live
  // characteristic, which after a capture echoes back a raw code.
  const ch = await device.readCharacteristicForService(
    CS_SERVICE_UUID, CS_CHAR_LIVE_UUID);
  if (!ch.value) throw new Error('no capture response');
  const buf = Buffer.from(ch.value, 'base64');
  const raw = buf.readUInt32LE(0);
  return { index: pointIdx, pct: [0, 25, 75, 100][pointIdx], raw };
}

/* Write threshold configuration (4 floats: levelWarn, levelCrit,
 * levelRateCrit, tiltWarn). */
export async function sendThresholds(
  device: Device,
  levelWarn: number, levelCrit: number, levelRateCrit: number, tiltWarn: number
): Promise<void> {
  const cmd = Buffer.alloc(1 + 16);
  cmd.writeUInt8(0x02, 0);        // threshold opcode
  cmd.writeFloatLE(levelWarn, 1);
  cmd.writeFloatLE(levelCrit, 5);
  cmd.writeFloatLE(levelRateCrit, 9);
  cmd.writeFloatLE(tiltWarn, 13);
  await device.writeCharacteristicWithResponseForService(
    CS_SERVICE_UUID, CS_CHAR_THRESH_UUID, cmd.toString('base64'));
}

/* Acknowledge an active alarm on the connected device. */
export async function sendAck(device: Device, technicianId: string): Promise<void> {
  const cmd = Buffer.alloc(5);
  cmd.writeUInt8(0x03, 0);        // ack opcode
  cmd.writeUInt32LE(parseInt(technicianId, 10) || 0, 1);
  await device.writeCharacteristicWithResponseForService(
    CS_SERVICE_UUID, CS_CHAR_CAL_CMD_UUID, cmd.toString('base64'));
}

export function disconnect(device: Device): Promise<void> {
  return manager.cancelDeviceConnection(device.id);
}