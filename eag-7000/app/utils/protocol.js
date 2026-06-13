/**
 * EAG-7000 Wire Protocol Definition
 *
 * Defines the binary communication protocol between the companion app
 * (React Native) and the EAG-7000 Edge AI Gateway over BLE.
 *
 * This protocol matches the MU mailbox format used by the M4F firmware:
 *   32-bit message: [TYPE(8)][LEN(8)][DATA(16)]
 *
 * Extended frames (for CAN data > 2 bytes) use BLE characteristic
 * notifications with a length-prefixed payload format.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

// ============================================================================
// Message Types — Match M4F firmware board.h MU_MSG_TYPE_*
// ============================================================================

export const MSG_TYPE = {
  HEARTBEAT:    0x01,  // M4→App: Periodic heartbeat (uptime, status)
  CAN_RX:       0x02,  // M4→App: CAN frame received from bus
  CAN_TX:       0x03,  // App→M4: Transmit CAN frame to bus
  MODBUS_RX:    0x04,  // M4→App: Modbus response received
  MODBUS_TX:    0x05,  // App→M4: Send Modbus request
  POWER:        0x06,  // App→M4: Power state command
  STATUS:       0x07,  // M4→App: System status report
  SENSOR_DATA:  0x08,  // M4→App: Sensor reading (I2C mux channel)
  CONFIG:       0x09,  // App→M4: Configuration update
  OTA_START:    0x0A,  // App→M4: Start firmware OTA update
  OTA_DATA:     0x0B,  // App→M4: Firmware OTA data chunk
  OTA_END:      0x0C,  // App→M4: End firmware OTA update
};

// ============================================================================
// CAN Bus IDs — Match M4F firmware board.h CAN_ID_*
// ============================================================================

export const CAN_ID = {
  HEARTBEAT:    0x100,  // M4→A72 heartbeat
  SENSOR_DATA:  0x200,  // CAN sensor data
  MODBUS_CMD:   0x300,  // Modbus-over-CAN command
  ACK:          0x400,  // Acknowledgment
};

// ============================================================================
// Power State Commands
// ============================================================================

export const POWER_CMD = {
  SLEEP:  0x01,  // Request M4F to prepare for sleep
  WAKE:   0x02,  // Wakeup signal
  RESET:  0x03,  // Full system reset
};

// ============================================================================
// CAN Frame Flags — Match M4F firmware CAN_FRAME_*
// ============================================================================

export const CAN_FLAGS = {
  EXT:  0x01,  // Extended ID (29-bit)
  RTR:  0x02,  // Remote Transmission Request
  FDF:  0x04,  // CAN-FD format
  BRS:  0x08,  // Bit Rate Switch
  ESI:  0x10,  // Error State Indicator
};

// ============================================================================
// Sensor Data Types (I2C mux channels)
// ============================================================================

export const SENSOR_TYPE = {
  TEMPERATURE:  0x01,  // Temperature sensor (°C × 100)
  HUMIDITY:     0x02,  // Humidity sensor (%RH × 10)
  PRESSURE:     0x03,  // Pressure sensor (hPa × 10)
  IMU_ACCEL:    0x04,  // IMU accelerometer (mg)
  IMU_GYRO:    0x05,  // IMU gyroscope (mdps)
  LIGHT:        0x06,  // Ambient light sensor (lux)
  VOLTAGE:      0x07,  // Voltage sensor (mV)
  CURRENT:      0x08,  // Current sensor (mA)
};

// ============================================================================
// BLE Service and Characteristic UUIDs
// ============================================================================

export const BLE = {
  // EAG-7000 BLE Service UUID (custom)
  SERVICE_UUID:          '6E400001-B5A3-F393-E0A9-E50E24DCCA9E',

  // TX characteristic (M4→App, notify)
  // Gateway sends data to app via this characteristic
  TX_CHAR_UUID:          '6E400002-B5A3-F393-E0A9-E50E24DCCA9E',

  // RX characteristic (App→M4, write)
  // App sends commands to gateway via this characteristic
  RX_CHAR_UUID:          '6E400003-B5A3-F393-E0A9-E50E24DCCA9E',

  // Device info service (standard)
  DEVICE_INFO_SERVICE:   '180A',
  MODEL_CHAR:            '2A24',
  FIRMWARE_CHAR:         '2A26',
  HARDWARE_CHAR:         '2A27',
};

// ============================================================================
// Protocol Encode/Decode Functions
// ============================================================================

/**
 * Encode a short message (16-bit data payload) into a BLE packet.
 * Format: [TYPE(1)][LEN(1)][DATA_HI(1)][DATA_LO(1)]
 *
 * @param {number} type  Message type (MSG_TYPE.*)
 * @param {number} len   Payload length
 * @param {number} data  16-bit data field
 * @returns {Uint8Array} 4-byte encoded packet
 */
export function encodeMessage(type, len, data) {
  const packet = new Uint8Array(4);
  packet[0] = type & 0xFF;
  packet[1] = len & 0xFF;
  packet[2] = (data >> 8) & 0xFF;
  packet[3] = data & 0xFF;
  return packet;
}

/**
 * Decode a short message from a BLE packet.
 * @param {Uint8Array|ArrayBuffer} data  Raw BLE data
 * @returns {{type: number, len: number, data: number}} Decoded message
 */
export function decodeMessage(data) {
  const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
  return {
    type: bytes[0],
    len:  bytes[1],
    data: (bytes[2] << 8) | bytes[3],
  };
}

/**
 * Encode a CAN frame for transmission over BLE.
 * Extended format for CAN data > 2 bytes.
 * Format:
 *   [TYPE(1) = 0x02/0x03]
 *   [BUS(1)]
 *   [ID_HI(1)][ID_LO(1)]
 *   [DLC(1)]
 *   [FLAGS(1)]
 *   [DATA(0-64)]
 *
 * @param {number} bus    CAN bus number (0 or 1)
 * @param {number} id     CAN identifier (11 or 29 bit)
 * @param {number} dlc    Data Length Code
 * @param {number} flags  CAN frame flags
 * @param {Uint8Array} payload  Frame data
 * @returns {Uint8Array} Encoded packet
 */
export function encodeCANFrame(bus, id, dlc, flags, payload) {
  const headerLen = 6;
  const dataLen = payload ? payload.length : 0;
  const packet = new Uint8Array(headerLen + dataLen);

  packet[0] = MSG_TYPE.CAN_TX;
  packet[1] = bus & 0x01;
  packet[2] = (id >> 8) & 0xFF;
  packet[3] = id & 0xFF;
  packet[4] = dlc & 0x0F;
  packet[5] = flags & 0xFF;

  if (payload) {
    packet.set(payload, headerLen);
  }

  return packet;
}

/**
 * Decode a received CAN frame from BLE notification.
 * @param {Uint8Array|ArrayBuffer} data  Raw BLE data
 * @returns {{bus: number, id: number, dlc: number, flags: number, data: Uint8Array}}
 */
export function decodeCANFrame(data) {
  const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;

  if (bytes.length < 6) {
    return { bus: 0, id: 0, dlc: 0, flags: 0, data: new Uint8Array(0) };
  }

  const bus   = bytes[1];
  const id    = (bytes[2] << 8) | bytes[3];
  const dlc   = bytes[4] & 0x0F;
  const flags = bytes[5];
  const canData = bytes.slice(6, 6 + dlc);

  return { bus, id, dlc, flags, data: canData };
}

/**
 * Encode a sensor data request for a specific I2C mux channel.
 * @param {number} channel  TCA9548A channel (0-7)
 * @param {number} sensorType  SENSOR_TYPE.*
 * @returns {Uint8Array} Encoded packet
 */
export function encodeSensorRequest(channel, sensorType) {
  return encodeMessage(MSG_TYPE.CONFIG, 0x08, (channel << 8) | (sensorType & 0xFF));
}

/**
 * Decode sensor data from a BLE notification.
 * @param {Uint8Array|ArrayBuffer} data  Raw BLE data
 * @returns {{channel: number, type: number, value: number, timestamp: number}}
 */
export function decodeSensorData(data) {
  const bytes = data instanceof ArrayBuffer ? new Uint8Array(data) : data;
  const msg = decodeMessage(bytes);

  // Extended sensor data uses bytes 4-7 for 32-bit value
  if (bytes.length >= 8) {
    return {
      channel: (msg.data >> 8) & 0xFF,
      type: msg.data & 0xFF,
      value: (bytes[4] << 24) | (bytes[5] << 16) | (bytes[6] << 8) | bytes[7],
      timestamp: Date.now(),
    };
  }

  return {
    channel: (msg.data >> 8) & 0xFF,
    type: msg.data & 0xFF,
    value: msg.data,
    timestamp: Date.now(),
  };
}

// ============================================================================
// System Status Flags
// ============================================================================

export const STATUS_FLAGS = {
  M4_RUNNING:      0x0001,  // Cortex-M4F is running
  A72_RUNNING:     0x0002,  // Cortex-A72 Linux is running
  NPU_ACTIVE:      0x0004,  // Hailo-8 NPU is active
  CAN0_ACTIVE:     0x0008,  // CAN-FD bus 0 is active
  CAN1_ACTIVE:     0x0010,  // CAN-FD bus 1 is active
  ETH0_LINK:       0x0020,  // Ethernet port 0 link up
  ETH1_LINK:       0x0040,  // Ethernet port 1 link up
  POE_POWERED:     0x0080,  // PoE+ power detected
  WATCHDOG_OK:     0x0100,  // Watchdog feeding normally
  OVERTEMP:        0x0200,  // Over-temperature warning
  LOW_VOLTAGE:     0x0400,  // Input voltage below threshold
};

/**
 * Parse status flags into human-readable labels.
 * @param {number} flags  16-bit status flags from M4F
 * @returns {string[]} Array of active status labels
 */
export function parseStatusFlags(flags) {
  const active = [];
  for (const [label, mask] of Object.entries(STATUS_FLAGS)) {
    if (flags & mask) {
      active.push(label);
    }
  }
  return active;
}

// ============================================================================
// DLC (Data Length Code) Conversion — CAN-FD
// ============================================================================

/**
 * Convert DLC to actual byte count for CAN-FD.
 * @param {number} dlc  Data Length Code (0-15)
 * @returns {number} Actual number of data bytes
 */
export function dlcToBytes(dlc) {
  const map = [0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64];
  return dlc < 16 ? map[dlc] : 64;
}

/**
 * Convert byte count to DLC for CAN-FD.
 * @param {number} bytes  Number of data bytes
 * @returns {number} Data Length Code
 */
export function bytesToDLC(bytes) {
  if (bytes <= 8) return bytes;
  if (bytes <= 12) return 9;
  if (bytes <= 16) return 10;
  if (bytes <= 20) return 11;
  if (bytes <= 24) return 12;
  if (bytes <= 32) return 13;
  if (bytes <= 48) return 14;
  return 15;
}