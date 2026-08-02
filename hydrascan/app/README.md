# HydraScan Companion App

**Author:** jayis1
**License:** MIT

React Native (Expo) companion app for the HydraScan pocket liquid
fingerprinting instrument.

## Build & run

```bash
yarn            # or: npm install
yarn start      # opens Expo Dev Tools; scan the QR with the Expo Go app
# iOS simulator:    yarn ios
# Android emulator: yarn android
```

Required permissions (added to `app.json` / native manifests in a real
Expo prebuild):

- iOS: `NSBluetoothAlwaysUsageDescription`
- Android: `BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT`, `ACCESS_FINE_LOCATION`

## Screens

| Screen       | Function                                              |
|--------------|-------------------------------------------------------|
| Scan         | BLE pairing, live result card with adulteration flag |
| Fingerprint  | 8-bar optical absorbance chart + EIS Nyquist plot      |
| Library      | Onboard liquid classes; add new class via BLE        |
| History      | Past scans with CSV share                             |
| Settings     | Adulteration thresholds per pair, firmware info, OTA  |

## BLE protocol

Service UUID `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic UART).

- **TX notify** (`…0003-…`): ASCII lines `R,<class_id>,<confidence>,
  <adulterant>,<ratio>,<temp_c>\n` per measurement.
- **RX write** (`…0002-…`): app → device commands `L,ADD,<id>,<name>,
  <n_features>,<…>\n` to extend the liquid library.

## Author

jayis1 — Copyright (C) 2026 jayis1. MIT licensed.