# Cryo-Sentinel

**Author:** jayis1
**License:** MIT
**Version:** 1.0.0

> A cryogenic Dewar sentinel for biobanks, IVF clinics, and stem-cell repositories.
> Continuously monitors liquid-nitrogen level, multi-zone temperature, lid state,
> tilt/vibration, and mains power — and routes cellular (LTE-M / NB-IoT) alarms
> before biological samples are lost.

---

## 1. Purpose and Overview

Cryogenic preservation underpins modern medicine and biology. Human gametes,
embryos, stem-cell lines, cord blood, tissue biobanks, and irreplaceable
research cultures are stored in liquid-nitrogen (LN2) Dewars at −196 °C. The
integrity of these collections depends on a single fragile invariant: **the
Dewar must always contain enough liquid nitrogen**.

In practice this invariant fails in mundane ways:

- A pressurized auto-fill solenoid sticks open and floods the room with N2 gas.
- An auto-fill line ices up and the Dewar silently boils dry over a weekend.
- A building chiller warms the room and boil-off accelerates 3×.
- A janitor unplugs the auto-fill controller to use the outlet for a vacuum.
- A caster fails and the Dewar tips, spilling 160 L of LN2 across a floor.
- An auditor accidentally leaves the lid ajar overnight.

Each of these scenarios can destroy decades of curated biological material in
minutes to hours, and the first sign that something is wrong is usually a
warm sample tube — long after the damage is done. Existing commercial
telemetry solutions are expensive, closed, single-vendor, and frequently tied
to a specific Dewar brand. They also tend to alarm *locally* — useless when
the on-call technician is at home at 02:00.

**Cryo-Sentinel** is an open, vendor-agnostic, battery-backed sentinel that
mounts magnetically to the side of any LN2 Dewar (or freezer tank) and
provides independent, layered monitoring with off-site cellular alarming. It
is not a replacement for the auto-fill controller; it is a **second,
independent witness** that rings a human on a phone when the primary system
fails. This separation of concerns — primary control vs. independent witness
— is the central design principle of the device.

### Design principles

1. **Independence.** Cryo-Sentinel shares no power, no sensors, and no
   controller with the Dewar's auto-fill system. It has its own battery, its
   own level probe, its own temperature probes, and its own radio. A single
   failure in the primary loop cannot blind it.
2. **Defense in depth.** LN2 level is the primary invariant, but it is a
   lagging indicator. Cryo-Sentinel also watches boil-off *rate*, vapor-column
   temperature gradient, lid state, room ambient, and Dewar tilt — so a
   failing Dewar is flagged hours before the level crosses a red line.
3. **Off-site alarming.** Local buzzers are useless at 02:00. Alarms are
   pushed over LTE-M / NB-IoT cellular to a configurable escalation chain
   (push notification → SMS → email → phone call), with acknowledgement and
   per-technician escalation timing.
4. **Tamper evidence.** Lid-open events, magnet-mount removal, and enclosure
   opening are all logged with timestamps to internal FRAM for audit. The
   device cannot be silently disabled without leaving a trail.
5. **Long autonomy.** On internal battery alone, Cryo-Sentinel runs for ≥ 72 h
   of full monitoring plus ≥ 24 h of alarm reporting. A week-long power
   outage must not silence it.

---

## 2. Hardware Specifications

### 2.1 MCU

- **Nordic nRF5340** dual-core SoC
  - Application core: Cortex-M33 @ 128 MHz, 1 MB flash, 512 KB RAM
  - Network core: Cortex-M33 @ 64 MHz, 256 KB flash, 64 KB RAM (runs the
    Thread/BLE radio stack)
- Chosen for: ultra-low-power sleep (≈ 1.2 µA in SYSTEM-OFF with RAM
  retention), integrated BLE 5 / Thread radio, and the dual-core split that
  lets the network core own the radio while the app core sleeps.

### 2.2 Sensors

| Sensor | Part | Purpose | Interface |
|---|---|---|---|
| LN2 level | Custom capacitive strip probe (Cryo-Sentinel design, §3.2) | Primary invariant | I²C CDC (FDC2214) |
| Multi-zone temp | 3× PT1000 RTD, top/mid/bottom of vapor column | Gradient + boil-off rate | I²C MAX31865 (3 channels via mux) |
| Lid state | Reed switch + magnet on lid hinge | Lid-open detection | GPIO + debouncer |
| Tilt / vibration | TDK ICM-45686 6-axis IMU | Tip detection, boil-off vibration, mount removal | I²C |
| Room ambient | Bosch BME280 | Ambient T/RH/P, room-side boil-off accel detection | I²C |
| Mains presence | Opto-isolated AC detector on a wall-wart tap | Detect facility power loss | GPIO |
| Enclosure open | Hall switch on enclosure lid | Tamper evidence | GPIO |

### 2.3 Connectivity

- **BLE 5.3** — on-device commissioning, local technician pairing, and
  live sensor preview from a phone within range.
- **LTE-M / NB-IoT** — primary off-site alarming channel via a Quectel
  BG770A-GLS module (dual-mode LTE-M/NB-IoT, fallback GPRS). The cellular
  link is duty-cycled: it sleeps and is only woken on alarm, on a 15-minute
  heartbeat, or on a forced check-in.
- **Thread 1.3** (optional) — for fleet topologies where several Dewars share
  one cellular gateway; reduces per-node SIM cost.
- **USB-C** — charging, firmware DFU, and a serial console for bring-up.

### 2.4 Power

- **External:** 5 V USB-C from a wall adapter (the "mains presence" tap is on
  this same adapter, so loss of facility power is detected independently of
  the device's own battery).
- **Internal:** 2× 18650 Li-ion in series (7.4 V, ~3500 mAh) with a
  TI BQ25895 charger and a MAX17055 fuel gauge.
- **Autonomy budget:** full monitoring at ~12 mA average + 15-min cellular
  heartbeat → ~72 h on battery alone with the cellular link active, and
  > 7 days in a low-power "mains-lost, monitor-only" mode that suspends
  heartbeats and only transmits on alarm.
- **Hot-swap:** the battery is field-replaceable without dropping monitoring,
  via a dual-diode OR'd power path.

### 2.5 Form Factor

- **Enclosure:** 95 × 65 × 32 mm anodized aluminium puck with a magnetic
  back (N42 NdFeB array) that clings to the steel skirt of standard
  Cryotherm / Taylor-Wharton / Worthington 35–180 L Dewars.
- **Probe harness:** 1.5 m PTFE-jacketed ribbon carrying the capacitive
  strip and three RTDs, with a vacuum-rated feed-through at the Dewar
  neck. The capacitive strip adheres to the **outer wall** of the inner
  vessel — no penetration of the vacuum space, no contact with LN2.
- **Weight:** 240 g (excluding harness).
- **Environmental:** rated −40 to +60 °C ambient (the device lives outside
  the Dewar's cold zone), IP54.

---

## 3. Architecture and Block Diagram

```
                ┌──────────────────────────────────────────────┐
                │                  nRF5340                      │
                │   ┌────────────┐        ┌──────────────┐      │
                │   │  App Core  │───────▶│  Net Core    │      │
  USB-C /       │   │  (monitor, │  IPC   │  (BLE/Thread │      │
  5V in ◀───────┼──▶│  alarm,    │        │   radio)     │      │
  BQ25895       │   │  state)    │        │              │      │
  charger       │   └─────┬──────┘        └──────┬───────┘      │
  2×18650 ◀─────┤         │ I²C / SPI / GPIO     │ UART         │
  MAX17055 ◀────┤         │                      │              │
                │         │                      │              │
                └─────────┼──────────────────────┼──────────────┘
                          │                      │
        ┌─────────────────┼──────────────────────┼─────────────┐
        │                 │                      │             │
   ┌────▼────┐  ┌─────────┴────────┐  ┌──────────▼────────┐  ┌─▼──────────┐
   │ FDC2214 │  │ MAX31865 x3 via  │  │  Quectel BG770A   │  │  Antennas  │
   │ capaci- │  │ TCA9548A mux     │  │  LTE-M / NB-IoT   │  │ PCB trace  │
   │ tive    │  │  PT1000 x3       │  │  + eSIM           │  │ + U.FL     │
   │ LN2     │  │  top/mid/bottom  │  └───────────────────┘  └────────────┘
   │ strip   │  └──────────────────┘
   └─────────┘
        │
   ┌────▼────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
   │ICM-45686│  │  BME280  │  │ Reed:    │  │ Hall:    │  │ Opto:    │
   │ IMU     │  │ ambient  │  │ lid      │  │ enclosure│  │ mains    │
   └─────────┘  └──────────┘  └──────────┘  └──────────┘  └──────────┘
```

### 3.1 Why external-wall capacitive sensing?

The classic approach to LN2 level is a resistive probe immersed in the bath
(or a float). Both require a feed-through into the cold/vacuum space, which
is a leak path and a contamination path, and both are brand-specific.
Cryo-Sentinel instead uses a **capacitive strip adhered to the outer wall of
the inner vessel** (between the inner and outer walls, in the vacuum-jacket
space, or — for Dewars where that space is not accessible — on the *outer*
outer wall with a calibrated offset). The dielectric constant of LN2 (εᵣ ≈
1.43) differs enough from N2 vapor (εᵣ ≈ 1.00) that the inter-electrode
capacitance changes measurably as the liquid/vapor boundary moves past the
strip. The FDC2214 measures this capacitance at 30 kHz with 28-bit
resolution, and the firmware converts raw capacitance to a level percentage
via a per-Dewar calibration table that is captured during commissioning.

This means Cryo-Sentinel works with **any** Dewar brand, requires no
penetration of the vacuum space, and can be retrofitted in under five
minutes.

### 3.2 Capacitive probe construction

The probe is a 1.2 m flexible PCB with two interleaved comb electrodes on
0.2 mm FR-4, jacketed in PTFE. It is self-adhesive (3M VHB 4910) and applied
vertically along the Dewar sidewall. A four-point calibration (empty, 25 %,
75 %, full) is stored in on-board FRAM (Fujitsu MB85RC04) and tagged with the
Dewar serial number. The calibration survives firmware updates and battery
removal.

### 3.3 Multi-zone RTD rationale

A single level number is a lagging indicator. Cryo-Sentinel places three
PT1000 RTDs at three heights in the vapor column (above the expected liquid
line). In steady state the vapor column shows a steep gradient: near the
liquid surface it is ≈ −190 °C, and near the neck it approaches ambient.
When boil-off accelerates (room warmed, vacuum jacket degraded, lid ajar),
the gradient *flattens* — the upper RTD drops sharply while the lower one
barely moves. The firmware computes the gradient slope on a rolling
15-minute window and flags a **boil-off anomaly** before the level itself
has moved appreciably. This typically buys 2–6 hours of advance warning.

---

## 4. Firmware Details and Design Decisions

The firmware lives in `firmware/` and is written in C99 against the nRF
Connect SDK (NCS) Zephyr port. The key design decisions are:

### 4.1 Dual-core split

- The **network core** runs Nordic's serialized Bluetooth LE + Thread radio
  stack and owns the BG770A UART. It exposes a small IPC RPC to the app
  core: `radio_send_alarm()`, `radio_send_heartbeat()`, `radio_ble_pair()`.
  This keeps all radio IRQs off the app core so the monitor loop has
  deterministic timing.
- The **app core** runs the sensor scheduler, the alarm state machine, and
  the FRAM audit log. It can sleep for whole seconds at a time and only
  wakes the network core when it has something to say.

### 4.2 Sensor schedule

A 5-second tick drives all sensing. On each tick:

1. Capacitive LN2 level is sampled 16× and median-filtered.
2. The three RTDs are read via the MAX31865 bank (one-shot conversion).
3. The IMU is read for tilt and for boil-off vibration RMS.
4. BME280 is read (forced mode) for ambient.
5. GPIO states (lid, enclosure, mains) are sampled and debounced.

Every 15 minutes, the app core computes the boil-off gradient, the level
rate-of-change, and the rolling tilt, and pushes a heartbeat to the cloud
over the cellular link.

### 4.3 Alarm state machine

The firmware implements a small, explicit state machine
(`enum cs_alarm_state`) with the following states:

- `CS_STATE_OK` — all invariants within tolerance.
- `CS_STATE_WATCH` — one soft invariant violated (e.g. gradient flattening,
  room warmer than 26 °C, mains absent but battery healthy). No alarm yet;
  a `watch` event is logged and surfaced on the dashboard.
- `CS_STATE_WARN` — a hard invariant violated but not yet catastrophic
  (level < 35 %, lid open > 10 min, tilt > 8°). Triggers a **Tier-1
  alarm**: push + SMS to the primary on-call.
- `CS_STATE_CRITICAL` — a catastrophic invariant (level < 20 %, level
  dropping > 2 %/h, tilt > 15 °, enclosure open while in WARN). Triggers a
  **Tier-2 alarm**: push + SMS + email + voice call to the full escalation
  chain, repeating every 5 minutes until acknowledged.

Transitions are monotone-ish: a CRITICAL cannot be cleared without an
explicit `ack` from the app, and clearing requires the underlying invariant
to be back in tolerance *and* a technician to have acknowledged.

### 4.4 Calibration

Commissioning is a four-point procedure driven from the companion app over
BLE: the technician records raw capacitance at (1) empty, (2) 25 %, (3)
75 %, (4) full. The firmware fits a piecewise-linear map and stores it in
FRAM along with the Dewar serial, the install date, and a checksum. A
single tap in the app re-runs calibration. The level reported to the cloud
is always the calibrated percentage plus the raw capacitance, so the cloud
can re-derive level if the probe is ever re-seated.

### 4.5 Audit log

Every state transition, every lid-open/close, every enclosure-open, every
mains loss/return, every alarm send, every acknowledgement, and every
calibration is appended to a ring buffer in the 4 KB FRAM with a monotonic
32-bit sequence number and a 32-bit minute timestamp (epoch minutes). The
log is mirrored to the cloud on every heartbeat. This gives a tamper-evid
audit trail that survives power loss and is required by ISO 20387
(biobanking) and CAP/CLAB audits.

### 4.6 Power management

The app core spends most of its life in `k_sleep` between 5 s ticks. The
cellular link is fully powered down between heartbeats (the BG770A draws
~1 mA in sleep, ~700 mA Tx; we avoid even the sleep current by gating its
VINT via a load switch). The BQ25895 charger is left in its standalone
mode; the firmware only talks to it to read fault flags. Mains loss is
detected within 1 s and triggers an immediate `WATCH` entry plus a
heartbeat so the cloud knows the node is now on battery.

---

## 5. Application / Software Interface

The companion app is a React Native application (see `app/`) targeting iOS
and Android. It speaks to Cryo-Sentinel over two channels:

- **BLE** during commissioning, calibration, and live local preview.
- **Cloud REST + WebSocket** for fleet monitoring and alarming.

Key screens:

1. **Fleet dashboard** — cards per Dewar showing level %, gradient slope,
   state badge (OK / WATCH / WARN / CRITICAL), last heartbeat, and battery
   state. Tapping a card drills into the per-Dewar detail view.
2. **Dewar detail** — live charts of level %, boil-off rate, vapor-column
   gradient, ambient, and tilt over the last 24 h / 7 d / 30 d.
3. **Commissioning** — BLE scan → connect → assign Dewar serial → run the
   four-point calibration wizard → set alarm thresholds → set escalation
   chain.
4. **Escalation chain editor** — ordered list of technicians with per-tier
   contact methods (push / SMS / email / call) and per-tier delays.
5. **Audit log** — chronological, filterable view of all events for a
   Dewar, exportable to CSV/PDF for audit submissions.
6. **Alert inbox** — active alarms requiring acknowledgement, with a
   one-tap ack that also records the acknowledger's identity and a free
   note.

The cloud backend (not included in this repo) is a thin FastAPI service
over Postgres + TimescaleDB; the device protocol is documented in
`firmware/docs/protocol.md` so the backend can be reimplemented by anyone.

---

## 6. Use Cases and Target Audience

### 6.1 Primary

- **IVF and fertility clinics** — cryopreserved embryos and gametes are
  irreplaceable and litigious. Cryo-Sentinel gives clinics an independent
  witness with off-site alarming, satisfying the "redundant monitoring"
  requirement of most national fertility regulators.
- **Stem-cell and cord-blood banks** — large Dewar fleets, often unattended
  overnight, audited under ISO 20387. The audit log + fleet dashboard map
  directly onto ISO 20387 §7.6 (monitoring of storage conditions).
- **Research biobanks** — university core facilities with mixed Dewar
  brands and no IT budget for vendor lock-in. Cryo-Sentinel is
  vendor-agnostic and open-source.

### 6.2 Secondary

- **Veterinary / animal genetics** — cattle and equine semen storage, where
  a single Dewar can represent a season's genetics value.
- **Industrial / materials** — HTS superconductor storage, quantum-computing
  dilution-refrigerator pre-cool Dewars, laser-gain-cell cryostats.
- **Disaster-prep / field labs** — the battery-backed, cellular-alarming
  design works equally well on a mobile field cryostat with no Wi-Fi.

### 6.3 Non-cryo adaptations

Because the level probe is external and the firmware is parameterized, the
same hardware can monitor any cryogenic or refrigerated liquid vessel
(LOX, liquid argon, dry-ice/acetone baths) by swapping the calibration
table and the thresholds. The README for each adaptation is a single
markdown file in the `profiles/` directory of the firmware tree.

---

## 7. Repository Layout

```
cryo-sentinel/
├── README.md                 ← this file
├── firmware/
│   ├── main.c                ← app-core main + scheduler
│   ├── board.h               ← pin map and board constants
│   ├── registers.h           ← register definitions for external ICs
│   ├── cs_alarm.c            ← alarm state machine
│   ├── cs_alarm.h
│   ├── cs_sensor.c           ← sensor drivers (FDC2214, MAX31865, IMU, BME280)
│   ├── cs_sensor.h
│   ├── cs_radio.c            ← network-core IPC + cellular/BLE dispatch
│   ├── cs_radio.h
│   ├── cs_log.c              ← FRAM audit log
│   ├── cs_log.h
│   ├── cs_power.c            ← charger + fuel gauge + mains detect
│   ├── cs_power.h
│   ├── cs_calibration.c      ← 4-point level calibration
│   ├── cs_calibration.h
│   └── Makefile              ← build with nRF Connect SDK / Zephyr
├── kicad/
│   ├── cryo-sentinel.kicad_sch
│   ├── cryo-sentinel.kicad_pcb
│   └── cryo-sentinel.kicad_pro
└── app/
    ├── App.tsx
    ├── package.json
    ├── src/screens/Dashboard.tsx
    ├── src/screens/DewarDetail.tsx
    ├── src/screens/Commissioning.tsx
    ├── src/screens/EscalationChain.tsx
    ├── src/screens/AuditLog.tsx
    ├── src/screens/AlertInbox.tsx
    ├── src/ble/CryoBle.ts
    ├── src/cloud/Api.ts
    └── src/types.ts
```

---

## 8. Safety Notes

- Cryo-Sentinel **does not control** the Dewar's auto-fill valve. It is a
  monitor and alarm only. This is deliberate: a monitor that can also fill
  is a control system, and control systems demand a vastly higher safety
  integrity level. Keeping the safety loop open means Cryo-Sentinel can be
  deployed without a SIL-3 certification effort.
- The capacitive probe is **non-invasive**: it never contacts LN2 and never
  penetrates the vacuum jacket. It cannot, by construction, cause a
  Dewar failure.
- The internal battery is a protected 18650 pair with a real BQ25895
  charge controller and a PTC on each cell. The enclosure is vented.

---

## 9. Licensing and Credit

All firmware, schematics, PCB layouts, and the companion app are released
under the MIT license. All authorship and copyright notices credit
**jayis1**. See the header of each source file.

— *jayis1*