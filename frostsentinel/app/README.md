# FrostSentinel Companion App

**Author:** jayis1  
**Copyright © 2026 jayis1. All rights reserved.**  
**License:** MIT

Companion app for the FrostSentinel radiative frost prediction and ice-nucleation detection mesh.

## Screens

1. **Mesh Dashboard** — overview of all mesh nodes with RFRI, T_wet, ΔT_rad, and AE status
2. **Node Detail** — live time-series plots (24 h) and current readings for a single node
3. **Frost Watch** — active alert view with countdown, mitigation recommendations, and acknowledge
4. **Calibration** — leaf wetness threshold, sky IR offset, wick prime, AE baseline learning
5. **Provisioning** — scan, assign node IDs, set mesh roles, configure AES key, set sample interval
6. **Settings** — units, notifications, data export (CSV), time sync, OTA firmware

## BLE Protocol

Custom GATT service `f5b00001-5b3e-4f6a-9c2d-1e7f8a3b5c4d` with characteristics for:
- Live data (notify, 16-byte payload)
- Command (write)
- Log dump (notify, chunked 24-byte records)
- Status (read)

Framing: `[0xA5] [type] [len] [payload...] [checksum] [0x5A]`

## Building

```bash
npm install
npx expo start
```

## Author

**jayis1** — Copyright © 2026. MIT License.