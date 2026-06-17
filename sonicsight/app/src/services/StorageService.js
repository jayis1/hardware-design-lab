/**
 * StorageService.js — SonicSight Companion
 * Local storage for scan history, settings, and cached tomograms.
 * Uses AsyncStorage and react-native-fs for file I/O.
 * @author jayis1
 */

import AsyncStorage from '@react-native-async-storage/async-storage';

const KEYS = {
  SCAN_HISTORY: '@sonicsight_scans',
  SETTINGS: '@sonicsight_settings',
  CURRENT_SCAN: '@sonicsight_current',
};

class StorageService {
  /** Save scan record to history */
  async saveScan(scanData) {
    try {
      const history = await this.getScanHistory();
      history.unshift({
        ...scanData,
        id: scanData.id || `S${Date.now()}`,
        savedAt: new Date().toISOString(),
      });
      /* Keep last 100 scans */
      if (history.length > 100) history.length = 100;
      await AsyncStorage.setItem(KEYS.SCAN_HISTORY, JSON.stringify(history));
      return true;
    } catch (e) {
      console.warn('Failed to save scan:', e);
      return false;
    }
  }

  /** Get all scan history */
  async getScanHistory() {
    try {
      const data = await AsyncStorage.getItem(KEYS.SCAN_HISTORY);
      return data ? JSON.parse(data) : [];
    } catch (e) {
      return [];
    }
  }

  /** Delete a scan by ID */
  async deleteScan(scanId) {
    try {
      const history = await this.getScanHistory();
      const filtered = history.filter((s) => s.id !== scanId);
      await AsyncStorage.setItem(KEYS.SCAN_HISTORY, JSON.stringify(filtered));
      return true;
    } catch (e) {
      return false;
    }
  }

  /** Save app settings */
  async saveSettings(settings) {
    try {
      await AsyncStorage.setItem(KEYS.SETTINGS, JSON.stringify(settings));
      return true;
    } catch (e) {
      return false;
    }
  }

  /** Load app settings */
  async loadSettings() {
    try {
      const data = await AsyncStorage.getItem(KEYS.SETTINGS);
      return data ? JSON.parse(data) : {};
    } catch (e) {
      return {};
    }
  }

  /** Cache current scan data */
  async saveCurrentScan(scanData) {
    try {
      await AsyncStorage.setItem(KEYS.CURRENT_SCAN, JSON.stringify(scanData));
    } catch (e) {
      /* silent */
    }
  }

  /** Load cached current scan */
  async loadCurrentScan() {
    try {
      const data = await AsyncStorage.getItem(KEYS.CURRENT_SCAN);
      return data ? JSON.parse(data) : null;
    } catch (e) {
      return null;
    }
  }

  /** Clear all data */
  async clearAll() {
    try {
      await AsyncStorage.multiRemove(Object.values(KEYS));
    } catch (e) {
      /* silent */
    }
  }
}

export default StorageService;