/**
 * BleService.js — SonicSight Companion
 * BLE communication service. Handles scanning, connection, GATT
 * characteristic read/write/notify for SonicSight data streaming.
 * @author jayis1
 */

const SONICSIGHT_SERVICE_UUID = 'ef44e4a4-1a2b-4c3d-8e5f-6a7b8c9d0e1f';
const TOMOGRAM_CHAR_UUID      = 'ef44e4a5-1a2b-4c3d-8e5f-6a7b8c9d0e1f';
const STATUS_CHAR_UUID        = 'ef44e4a6-1a2b-4c3d-8e5f-6a7b8c9d0e1f';
const COMMAND_CHAR_UUID       = 'ef44e4a7-1a2b-4c3d-8e5f-6a7b8c9d0e1f';
const TOF_RAW_CHAR_UUID       = 'ef44e4a8-1a2b-4c3d-8e5f-6a7b8c9d0e1f';
const GAIN_CHAR_UUID          = 'ef44e4a9-1a2b-4c3d-8e5f-6a7b8c9d0e1f';

class BleService {
  constructor(manager) {
    this.manager = manager;
    this.device = null;
    this.service = null;
    this.tomogramSub = null;
    this.statusSub = null;
    this.tofSub = null;
    this.gainSub = null;
    this._tomogramCallback = null;
    this._statusCallback = null;
    this._progressCallback = null;
  }

  /** Start scanning for SonicSight devices */
  startScan(onDiscovered) {
    this.manager.startDeviceScan(
      null,
      { allowDuplicates: false },
      (error, device) => {
        if (error) {
          console.warn('BLE scan error:', error);
          return;
        }
        if (device && device.name && device.name.includes('SonicSight')) {
          onDiscovered(device);
        }
      }
    );
  }

  /** Stop scanning */
  stopScan() {
    this.manager.stopDeviceScan();
  }

  /** Connect to a SonicSight device */
  async connect(deviceId) {
    this.device = await this.manager.connectToDevice(deviceId);
    await this.device.discoverAllServicesAndCharacteristics();

    this.service = await this.device.getService(SONICSIGHT_SERVICE_UUID);
    return this.device;
  }

  /** Disconnect */
  async disconnect() {
    this._unsubscribe();
    if (this.device) {
      await this.device.cancelConnection();
      this.device = null;
    }
  }

  /** Send command to device */
  async sendCommand(cmd) {
    if (!this.device) return;
    const char = await this.service.getCharacteristic(COMMAND_CHAR_UUID);
    await char.writeWithoutResponse(Buffer.from(cmd, 'utf8'));
  }

  /** Read status */
  async readStatus() {
    if (!this.device) return null;
    try {
      const char = await this.service.getCharacteristic(STATUS_CHAR_UUID);
      const data = await char.read();
      const text = Buffer.from(data).toString('utf8');
      /* Parse: "25,24.5,12.4GB" → battery%, temp°C, sdFree */
      const parts = text.split(',');
      return {
        battery: parseInt(parts[0]) || 0,
        temperature: parseFloat(parts[1]) || 0,
        sdFree: parts[2] || '0 GB',
      };
    } catch (e) {
      return null;
    }
  }

  /** Subscribe to tomogram notifications */
  onTomogram(callback) {
    this._tomogramCallback = callback;
    this._subscribeToCharacteristic(TOMOGRAM_CHAR_UUID, (data) => {
      const text = Buffer.from(data).toString('utf8');
      /* Parse compressed tomogram data */
      const image = this._decodeTomogram(text);
      if (image) callback(image);
    });
  }

  /** Subscribe to status notifications */
  onStatus(callback) {
    this._statusCallback = callback;
    this._subscribeToCharacteristic(STATUS_CHAR_UUID, (data) => {
      const text = Buffer.from(data).toString('utf8');
      callback(text);
    });
  }

  /** Subscribe to progress notifications (embedded in status) */
  onProgress(callback) {
    this._progressCallback = callback;
  }

  /** Internal subscribe helper */
  async _subscribeToCharacteristic(uuid, onData) {
    if (!this.service) return;
    try {
      const char = await this.service.getCharacteristic(uuid);
      await char.monitor((error, charData) => {
        if (error) return;
        if (charData?.value) {
          onData(charData.value);
        }
      });
    } catch (e) {
      console.warn(`BLE subscribe to ${uuid} failed:`, e);
    }
  }

  /** Decode compressed tomogram data from device */
  _decodeTomogram(text) {
    try {
      /* Format: "chunk_offset:HEX_ENCODED_PIXELS" */
      const [offsetStr, hexData] = text.split(':');
      const offset = parseInt(offsetStr) || 0;
      const gridSize = 64;

      /* Parse hex string → array of 8-bit velocity indices */
      const pixels = [];
      for (let i = 0; i < hexData.length; i += 2) {
        const byte = parseInt(hexData.substr(i, 2), 16);
        const velocity = 500 + byte * 13.73; /* inverse of firmware mapping */
        pixels.push(velocity);
      }

      /* Assemble into full tomogram buffer */
      const image = new Float32Array(gridSize * gridSize);
      for (let i = 0; i < pixels.length && (offset + i) < image.length; i++) {
        image[offset + i] = pixels[i];
      }

      return { image, velMin: 500, velMax: 4000, gridSize };
    } catch (e) {
      return null;
    }
  }

  /** Clean up subscriptions */
  _unsubscribe() {
    this.tomogramSub = null;
    this.statusSub = null;
    this.tofSub = null;
    this.gainSub = null;
  }
}

export default BleService;