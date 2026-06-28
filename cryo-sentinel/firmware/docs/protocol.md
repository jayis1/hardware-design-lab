# Cryo-Sentinel cloud protocol
Author: jayis1 · License: MIT

The backend is a thin FastAPI service over Postgres + TimescaleDB. The
device speaks to it over LTE-M/NB-IoT using MQTT (BG770A AT commands) with
the following topic / payload contract. Anyone can reimplement the backend.

## MQTT topics

- `cryo/<dewar_serial>/heartbeat`   — published every 15 min by the device
- `cryo/<dewar_serial>/alarm`       — published on alarm, repeated until ack
- `cryo/<dewar_serial>/ack`         — published by the cloud on ack, subscribed by device
- `cryo/<dewar_serial>/log`         — published on each log record (best-effort)
- `cryo/<dewar_serial>/cmd`         — subscribed by device (thresholds, cal, reboot)

## Heartbeat payload (JSON)

```json
{
  "seq": 1234,
  "epoch_min": 29341234,
  "state": "OK|WATCH|WARN|CRITICAL",
  "reason": "none|level_low_warn|...",
  "level_pct": 78.2,
  "level_rate_ph": 0.12,
  "rtd_top_c": -120.0,
  "rtd_mid_c": -170.0,
  "rtd_bot_c": -192.0,
  "gradient_slope": 72.0,
  "tilt_deg": 1.2,
  "vib_rms_g": 0.012,
  "ambient_c": 21.0,
  "ambient_rh": 44.0,
  "batt_pct": 95.0,
  "mains_present": true,
  "lid_open": false,
  "enclosure_open": false
}
```

## Alarm payload

```json
{
  "seq": 1235,
  "epoch_min": 29341235,
  "tier": 1,
  "state": "WARN",
  "reason": "lid_open",
  "level_pct": 41.0,
  "aux": 0.0,
  "dewar_serial": "TW-XC-3492"
}
```

## Ack payload (cloud → device)

```json
{ "seq": 1235, "technician_id": 1234 }
```

## REST endpoints (for the app)

- `GET  /v1/dewars`                            — fleet snapshot
- `GET  /v1/dewars/{id}/history?hours=N`       — TimescaleDB time-series
- `GET  /v1/dewars/{id}/log`                   — audit log
- `GET  /v1/dewars/{id}/escalation`            — escalation chain
- `PUT  /v1/dewars/{id}/escalation`            — update chain
- `POST /v1/dewars/{id}/ack`                   — acknowledge alarm

## WebSocket

`wss://api/cryo/alarms` — pushes `{dewarId, state, reason}` on each alarm.

— jayis1