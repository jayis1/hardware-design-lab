# Inkwell — Companion App

React Native (Expo) companion app for the **Inkwell** smart fountain pen.

## Features

- **Live Canvas** — real-time, pressure-sensitive stroke rendering on an
  infinite pinch-zoom canvas.
- **Notebooks** — sessions auto-grouped by date, searchable, with per-stroke
  metadata (pen-lift count, mean pressure, duration).
- **Session Sync** — pulls missing journal records from the pen's flash on
  reconnect so no stroke is ever lost.
- **Export** — SVG (lossless), PNG, PDF, and `.inkwell` JSON.
- **Handwriting Recognition** — opt-in on-device TFLite (Latin) or cloud
  (multi-script); original strokes always preserved alongside recognized
  text.
- **Signature Dynamics** — cryptographically signed stroke-dynamics records
  for notary / forensic workflows.
- **Calibration** — guided four-step pen calibration (pressure zero, pressure
  scale, AHRS magnetometer, drift character).

## Screens

| Screen | Purpose |
|---|---|
| `LiveCanvasScreen` | Live handwriting capture canvas + status bar |
| `NotebookListScreen` | Saved sessions list |
| `SessionDetailScreen` | Replay a saved session |
| `ExportScreen` | Export session to SVG / PNG / PDF |
| `CalibrationScreen` | Guided pen calibration |
| `SettingsScreen` | Recognition / forensics / appearance / about |

## BLE Protocol

Custom GATT service `1b7e0001-...` with four characteristics: Stroke Data
(notify), Control (write), Status (read/notify), Journal Replay (write/notify).
See `src/ble/protocol.ts` for the wire format and `src/ble/BleManager.ts`
for the connection logic.

## Build

```bash
npm install
npx expo start
```

## Author

**jayis1** — © 2026 jayis1. MIT licensed.