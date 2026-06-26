# Glaciator-Motes — Solar-Powered Wireless Microseismometer Mesh for Glacier & Ice-Sheet Monitoring

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

![MCU](https://img.shields.io/badge/MCU-STM32WL55JC-orange)
![Radio](https://img.shields.io/badge/RF-LoRa%20SX1262%20Sub--GHz-green)
![ADC](https://img.shields.io/badge/ADC-ADS1256%2024--bit-brightgreen)
![Sensor](https://img.shields.io/badge/Geophone-Triaxial%204.5Hz-blue)
![Power](https://img.shields.io/badge/Power-Solar%20%2B%20LiFePO4%20186050-yellow)
![Mesh](https://cmd.shields.io/badge/Mesh-TDMA%20LoRa-red)

---

## 1. Overview & Purpose

**Glaciator-Motes** is a field-deployable, solar-powered wireless microseismometer
mesh designed to continuously monitor the seismic activity of glaciers, ice
sheets, and alpine permafrost for scientific research and natural-hazard early
warning. Each mote is a self-contained, ruggedized cylindrical pod that is
simply drilled or placed onto the ice surface; motes self-organize into a
multi-hop LoRa mesh and stream event-triggered seismic waveforms to a base
station gateway for months to years of unattended operation.

Glacio-seismology is a rapidly growing field. Basal sliding, crevasse
propagation, iceberg calving, subglacial water flow, and serac collapses all
generate distinctive microseismic signals in the 0.5–80 Hz band. Traditional
seismological campaigns use cabled reftek stations costing $15,000–$40,000 each,
requiring heavy batteries, bulky solar panels, and manual cable trenching across
crevassed terrain — limiting deployments to a handful of stations and a single
ablation season. Wireless sensor networks promised to solve this, but existing
nodes (e.g., generic LoRa environmental motes) sample only slow scalars
(temperature, humidity) and have neither the dynamic range, the timing accuracy,
nor the on-device event-detection capability required for seismology.

Glaciator-Motes closes this gap with a purpose-built architecture:

- **True seismic-grade front end.** Three 4.5 Hz vertical/geophone coils
  (or a single triaxial 3-component geophone) feed a 24-bit delta-sigma ADC
  (ADS1256) sampling at 200 SPS per axis, yielding >120 dB dynamic range —
  enough to resolve both ambient icequakes (Mv -2 to +1) and large calving
  events (Mv +3) on the same sensor without clipping.
- **Dual-band sensing.** A MEMS accelerometer (ICM-42688) samples at 1 kHz
  to capture the high-frequency content of near-field crevasse snaps and
  serac collapses that saturate the geophone, giving broadband 0.5 Hz–1 kHz
  coverage per mote.
- **On-device event detection.** An STM32WL55 runs a short-time / long-time
  average (STA/LTA) trigger and a recursive one-bit cross-correlation
  detector; only triggered windows (±5 s around the event) are transmitted,
  reducing radio duty cycle by 99% versus continuous streaming while still
  retaining continuous waveform on local microSD for post-season analysis.
- **Time-synchronized mesh.** A 100-ppb TCXO plus GPS-PPS discipline (when
  available) keeps nodes within ±50 µs of each other; the LoRa TDMA mesh
  uses a slot-assignment protocol that delivers multi-hop relaying with
  deterministic latency for event correlation across the array.
- **Extreme-environment power.** A 5 W ruggedized solar panel charges a
  2×18650 LiFePO4 pack (3.2 V × 3.3 Ah = ~10.5 Wh) through a BQ25895
  MPPT charger; a TPS62740 buck regulator drops the system rail to 1.8 V,
  and aggressive duty-cycling yields a year-round survival margin even
  through polar night (using a supercapacitor bridge).

The system is fully open: schematics, PCB layout, firmware, and the React
Native companion app are all released under permissive/reciprocal licenses so
that any glaciology group can build, modify, and deploy their own array at
~$220/node BOM versus $15k+ for commercial reftek stations.

---

## 2. How It Works

### 2.1 Seismic Acquisition Chain

Each mote hosts a tri-axial geophone (three 4.5 Hz moving-coil sensors
orthogonally mounted, or a single 3-component geophone puck). Moving-coil
geophones are passive, broadband velocity sensors whose voltage output is
proportional to ground velocity above their natural frequency; they require
no power, which is critical for a solar node. The three analog channels
enter an instrumentation front end (low-noise JFET op-amp, gain = 100,
band-limited to 80 Hz) and are multiplexed into a **TI ADS1256** 24-bit
delta-sigma ADC running at a PGA gain of 32 and 200 SPS per channel (with
auto-scan of all 8 inputs, 3 used for the geophone, 1 for the MEMS
high-rate stream downsampled, 1 for board temperature, 1 for battery
voltage, 1 for solar voltage, 1 for geophone calibration-loop test).

The ADS1256 talks to the STM32WL55 over SPI at 8 MHz; the MCU services it
via a DMA-driven double-buffer so that sample buffers are never lost even
during radio transmit windows. Sample timestamps are stamped at the ISR
with a microsecond counter disciplined by the TCXO and (when sky is
visible) a u-blox NEO-M9N GPS 1-PPS edge.

The ICM-42688 MEMS accelerometer runs in low-power mode at 1 kHz, 2 g
full-scale; its data-ready line interrupts the MCU, which DMA-streams 1 s
windows into a ring buffer and runs a separate high-frequency trigger
(threshold + STA/LTA) for crevasse-snap detection.

### 2.2 On-Device Event Detection

Continuous 200 SPS triaxial waveforms are processed in 0.5 s frames by a
classic STA/LTA trigger:
- STA window = 0.5 s, LTA window = 10 s (recursive implementation, no
  full-window buffering).
- Trigger declared when STA/LTA ratio exceeds a tunable threshold (default
  3.0) on any of the three components.
- A recursive one-bit normalized cross-correlation runs between a small
  library of template events (basal slide, crevasse snap, calving thump)
  and the incoming stream to classify the event type on-device.
- On trigger, ±5 s of triaxial waveform (200 SPS × 3 ch × 10 s × 4 bytes
  = 24 KB) plus the MEMS high-rate window (1 kHz × 3 ch × 2 s × 2 bytes
  = 12 KB) is sealed into an event packet, written to microSD, and queued
  for LoRa transmission.

This on-device triggering is the key innovation: a 50-node array
generating perhaps 500 events/day transmits only ~12 MB/day across the
whole mesh, which fits comfortably in LoRa's duty-cycle budget, whereas
continuous streaming of 200 SPS × 3 ch would require 52 MB/node/day —
impossible over LoRa.

### 2.3 LoRa TDMA Mesh

The radio is the SX1262 sub-GHz LoRa transceiver integrated into the
STM32WL55 (no separate radio chip needed). The mesh uses a lightweight
TDMA protocol on top of LoRa:

- A **gateway mote** (the one with line-of-sight to the base station /
  GSM uplink) broadcasts a 1 s beacon every 60 s carrying the global
  frame counter and slot assignments.
- Each mote is assigned one 2 s uplink slot in a 30 s superframe; 15
  motes fit per beacon group, and the protocol supports up to 4 beacon
  groups (60 motes) per gateway with inter-group relay slots.
- Events are transmitted as compressed (LZ4) waveform packets in the
  mote's slot; multi-hop relaying uses reserved relay slots so that
  distant motes can reach the gateway through 1-2 intermediate nodes.
- LoRa parameters: SF9, BW 125 kHz, CR 4/5, 868 MHz (EU) / 915 MHz (US),
  25 mW ERP. Effective payload ~110 bytes/slot → a 24 KB event packet
  is fragmented over ~220 slots, spread across multiple superframes to
  respect the 1% EU duty cycle.

A simple CSMA/CA backoff handles slot collisions for unscheduled
high-priority events (e.g., a large calving detected simultaneously by
many motes); the gateway deduplicates overlapping events.

### 2.4 Timing & Synchronization

Array seismology requires that all nodes share a common time base to within
a fraction of the dominant period; for 4.5 Hz geophones that means <50 ms,
but for cross-correlation localization of icequakes the tighter the better.
Glaciator-Motes achieves:

- **±2 ppm free-run** from the 100-ppb TCXO (fox F9A) → <0.17 s/day drift.
- **GPS-PPS discipline** when sky is visible: the GPS 1-PPS edge corrects
  a software PLL that steers the local timescale to within ±1 µs of UTC.
  GPS is duty-cycled (on 5 min every hour) to save power.
- **Beacon-based mesh sync:** motes without sky view lock to the gateway's
  beacon timestamp, yielding ±5 ms across the mesh.
- Every ADC sample is stamped with the local 32-bit microsecond counter
  plus the current UTC estimate; event packets carry both so the gateway
  can re-time-correlate.

### 2.5 Power Budget

| Subsystem | Active | Sleep | Duty |
|---|---|---|---|
| STM32WL55 (M0+ radio, M4 app) | 12 mA | 1.2 µA | 5% |
| ADS1256 + geophone FE | 28 mA | 0.4 mA (standby) | 100% (always sampling) |
| ICM-42688 (1 kHz LP) | 0.6 mA | 6 µA | 100% |
| SX1262 TX (25 mW) | 125 mA | — | 0.3% |
| microSD write | 40 mA | 0.3 mA | 1% |
| GPS NEO-M9N | 22 mA | 9 µA | 8% (5 min/hr) |
| TCXO + RTC | 1.5 mA | 1.5 mA | 100% |

Average draw ≈ **2.9 mA at 3.3 V = 9.6 mW**.
A 5 W solar panel yields ~1.5 W-hr/day in polar summer (24 h daylight, low
sun angle) and ~0.4 W-hr/day in alpine winter — both comfortably above the
9.6 mW continuous drain (0.23 W-hr/day). The 10.5 Wh LiFePO4 pack bridges
~45 days of zero generation (polar night / burial), and a 10 F
supercapacitor handles short dark spells and inrush without stressing the
battery. The system is designed to survive a full year unattended in the
ablation zone.

---

## 3. Hardware Specifications

| Parameter | Value |
|---|---|
| MCU | STM32WL55JC — dual-core Cortex-M4 + M0+, 48 MHz, 256 KB Flash, 64 KB SRAM |
| LoRa Radio | SX1262 (integrated in STM32WL55), 150–960 MHz, +22 dBm max, sub-GHz |
| Seismic ADC | TI ADS1256 — 8 ch, 24-bit, 30 kSPS max, PGA 1–64, SPI |
| Geophone | 3× SM-6B 4.5 Hz vertical OR 1× triaxial 3C-4.5Hz puck, 395 Ω coil |
| MEMS accel | TDK ICM-42688 — ±16 g, 1 kHz, SPI, 0.6 mA LP |
| GPS | u-blox NEO-M9N — 72-ch, 1-PPS, cold-start <25 s, 22 mA |
| TCXO | Fox F9A-100 ppb, 32.768 kHz for RTC + 26 MHz for radio |
| Storage | microSD (UHS-I, SPI mode, up to 64 GB) — continuous waveform cache |
| Charger | TI BQ25895 — 5 A buck-boost MPPT, USB-C or solar input |
| Battery | 2× 18650 LiFePO4 in parallel, 3.2 V nominal, 3.3 Ah, protected |
| Regulator | TPS62740 — 1.8 V system rail, 360 nA IQ, 90% efficiency |
| Supercap | 10 F / 5.5 V — bridges short dark spells, handles inrush |
| Solar | 5 W mono-crystalline, 6 V, ruggedized ETFE-laminated, -40..+85 °C |
| Connectors | IP68 M8 bulkhead — geophone, solar; pogo pads for programming |
| Form factor | Cylindrical POM pod, 70 mm Ø × 240 mm, 480 g (w/o battery) |
| Operating temp | -45 °C to +60 °C (geophone damping fluid rated to -40 °C) |
| Depth rating | IP68, 2 m immersion (transient meltwater) |
| Antenna | Custom 868 MHz helical whip, ±2 dBi, omnidirectional |

---

## 4. Architecture & Block Diagram

```
                ┌──────────────────────────────────────────────────────────┐
                │                    GLACIATOR-MOTE                        │
                │                                                          │
   Solar(5W)────│─MPPT─► BQ25895 ──► LiFePO4 2×18650 ──► TPS62740 ──► 1.8V │
                │            │                  │                           │
                │            └─ Vbat monitor ───┘                           │
                │                                                          │
   Geophone ────┤─[3× JFET preamp, G=100]─► MUX ─► ADS1256 24-bit ─SPI─►   │
   (triaxial)   │   0.5-80 Hz BP                200 SPS/ch   │              │
                │                                              │            │
   ICM-42688 ───┤─SPI────────────────────────────────────────►│            │
   (1 kHz)      │   DRDY IRQ                                    │            │
                │                                              ▼            │
   GPS NEO-M9N──┤─UART─────────────────────────────► ┌──────────────────┐  │
   (1-PPS)──────┤─PPS──────────────────────────────► │   STM32WL55JC    │  │
                │                                    │ Cortex-M4 (app)  │  │
   TCXO 26 MHz──┤───────────────────────────────────►│ Cortex-M0+(radio)│  │
   TCXO 32.768k─┤───────────────────────────────────►│ RTC              │  │
                │                                    │                  │  │
   microSD ─────┤─SPI──────────────────────────────► │  Event detect    │  │
   (waveform)   │                                    │  STA/LTA + xcorr │  │
                │                                    │  TDMA mesh MAC   │  │
                │                                    │  LZ4 compress    │  │
                │                                    └────────┬─────────┘  │
                │                                             │            │
                │                                    SX1262 (integrated)  │
                │                                             │            │
                │                                    868 MHz helical whip │
                └──────────────────────────────────────────────────────────┘
```

### 4.1 Firmware Core Partition

The STM32WL55's dual core is exploited cleanly:
- **Cortex-M0+ (radio core):** runs the LoRa MAC, TDMA scheduler, SX1262
  driver, AES-128 CCM encryption of event packets, and the beacon/relay
  state machine. Communicates with the M4 via a 4 KiB shared SRAM mailbox
  and two semaphores (IPC_IT).
- **Cortex-M4 (application core):** runs the ADS1256 + ICM-42688 drivers,
  STA/LTA trigger, template correlator, LZ4 compression, SD-card logger,
  GPS-PPS disciplining PLL, and the power-management FSM. Hands sealed
  event packets to the M0+ via the mailbox for transmission.

This partition means a long LoRa TX window (up to 2 s at SF9) never blocks
sample acquisition or event detection — a critical correctness property
for a seismometer.

---

## 5. Firmware Design

The firmware lives under `firmware/` and is organized as:

```
firmware/
├── Makefile              # GNU ARM toolchain build
├── board.h               # Pin assignments, peripheral mappings
├── registers.h           # ADS1256, ICM-42688, BQ25895 register maps
├── main.c                # Application core entry, FSM, task scheduler
├── adc_driver.c          # ADS1256 SPI driver, DMA double-buffer, calibration
├── adc_driver.h
├── mems_driver.c         # ICM-42688 SPI driver, FIFO streaming
├── mems_driver.h
├── seismology.c          # STA/LTA, one-bit xcorr, event classifier
├── seismology.h
├── lora_mesh.c           # TDMA MAC, beacon, relay, fragmentation, AES
├── lora_mesh.h
├── gps discipline.c      # NEO-M9N UART + 1-PPS PLL
├── gps_discipline.h
├── power.c               # BQ25895, fuel gauge, solar MPPT, FSM
├── power.h
├── sdlog.c               # microSD SPI, FAT-lite, ring buffer
├── sdlog.h
├── lz4.c                 # Embedded LZ4 compressor (BSD-licensed subset)
└── lz4.h
```

Key design decisions:

- **DMA double-buffered ADC:** the ADS1256's DRDY line is wired to an EXTI
  line; on each DRDY the ISR reads the latest conversion via SPI DMA into
  buffer A while buffer B is being processed, then swaps. This guarantees
  zero sample loss across radio TX windows.
- **Recursive STA/LTA:** the long-term average is updated as
  `LTA = LTA + (x - LTA)/N` with N = 2000 (10 s at 200 SPS), so only a
  float per channel is held — no 10 s buffer needed, saving 24 KB SRAM.
- **One-bit cross-correlation:** the incoming stream and each template are
  reduced to ±1 sign bits; the correlation is then a popcount of XORs,
  extremely fast on the M4 (SMLAD instruction). This makes template
  matching real-time at 200 SPS × 3 channels.
- **GPS duty-cycling:** the GPS is powered by a dedicated MOSFET and
  enabled 5 min/hr; on cold start the firmware warms the NEO-M9N's
  assistnow data via the app over BLE before deployment.
- **SD ring buffer:** the SD card stores the last 48 h of continuous
  triaxial waveform in a 4 GB ring; triggered events are additionally
  written as metadata-indexed records so they can be retrieved
  post-season even if the radio missed them.
- **Brownout survival:** below 3.0 V battery the mote enters "polar
  survival" mode: GPS off, MEMS off, ADC down to 50 SPS, LoRa beacon-only
  once per 10 min, SD writes suspended — drawing <0.5 mA until solar
  returns.

---

## 6. Companion Application

The React Native companion app (`app/`) runs on the field glaciologist's
phone or tablet and provides:

- **Deployment wizard:** scan a mote's QR code, set node ID, mesh group,
  radio band (EU/US), trigger thresholds, and seed template events; push
  config to the mote over BLE 5 (the STM32WL55 also advertises a BLE
  GATT server for provisioning — note: BLE is only used for the initial
  config session, not for field data, to save power).
- **Gateway dashboard:** when the phone is connected (USB-C) to the
  gateway mote it acts as the uplink, displaying a live event feed
  (time, type, magnitude estimate, contributing motes), a map of mote
  positions with battery/solar status, and an array health view (sample
  rate, timing skew per node, last-seen timestamps).
- **Event browser:** tap any event to see the triaxial waveform from all
  detecting motes overlaid on a common time axis, the STA/LTA trace, the
  template-match score, and a preliminary epicenter estimated by
  cross-correlation time-difference-of-arrival across the array.
- **Config sync:** pull/push trigger thresholds, template libraries, and
  mesh slot assignments to the whole array via the gateway.
- **Offline export:** save event waveforms (mini-SEED or SAC format) to
  device storage or upload to a research archive.

Screens implemented in `app/src/`:
- `DeployWizard.tsx` — per-mote provisioning flow.
- `GatewayDashboard.tsx` — live event feed + array map.
- `EventDetail.tsx` — waveform viewer with cross-correlation epicenter.
- `ArrayHealth.tsx` — per-node health table.
- `ConfigSync.tsx` — threshold/template/slot management.
- `App.tsx` — navigation shell.

---

## 7. Use Cases & Target Audience

### 7.1 Primary: Glacio-seismology research

- **Basal sliding dynamics:** array of 30–60 motes deployed in a grid on
  a glacier surface records basal icequakes; cross-correlation TDOA
  locates the asperities and constrains basal friction laws.
- **Crevasse propagation monitoring:** the MEMS high-rate channel captures
  the 100–1000 Hz snap of opening crevasses; a linear array along a
  potential crevasse field maps propagation direction and speed in real
  time — critical for predicting serac collapse onto climbing routes or
  infrastructure.
- **Iceberg calving seismogenic monitoring:** a line of motes along a
  calving front records the sub-aquatic rumble preceding major calving,
  enabling calving-rate time series without visual observation (which is
  blocked by fog / polar night).
- **Subglacial water flow detection:** turbulent water flow generates a
  characteristic 1–10 Hz tremor; a mote array over a suspected subglacial
  channel can map its activation and discharge.
- **Permafrost hydro-fracturing:** in alpine permafrost, ice-wedge cracking
  produces distinct thermal-contraction events; motes monitor the seasonal
  cycle and climate-change-driven acceleration.

### 7.2 Secondary: Natural-hazard early warning

- **Serac collapse alerting:** a mote cluster on a serac above a village or
  climbing route can issue a LoRa alert to a gateway within seconds of a
  detected collapse precursor, enabling evacuation.
- **Ice-dam / GLOF monitoring:** moraine-dammed lakes that threaten GLOFs
  (glacial lake outburst floods) can be instrumented with motes detecting
  the seismic signature of dam instability.
- **Avalanche precursor research:** although not a primary use case, the
  snowpack acoustic emission band (50–500 Hz) overlaps the MEMS channel.

### 7.3 Target audience

- University glaciology / seismology research groups.
- National geological surveys operating in polar / alpine regions.
- Hydropower operators with glaciated catchments (calving into reservoirs).
- Climate-change field campaigns needing dense, cheap, long-duration
  seismic arrays.
- Backcountry-ski / alpine-rescue organizations interested in serac
  collapse alerting.
- Citizen-science and open-science programs (the open licenses and
  $220/node BOM make classroom and small-team deployments feasible).

---

## 8. Mechanical & Environmental

The mote is housed in a POM (acetal) cylindrical pod 70 mm × 240 mm,
chosen for impact toughness, low-temperature ductility, and UV resistance.
The geophone is potted to the pod base with a silicone coupling compound
to ensure mechanical grounding to the ice; three deploy options are
supported: (a) direct surface placement with a spike, (b) shallow augered
hole (the pod slips into a 70 mm borehole), (c) suspended in a crevasse
wall. The solar panel wraps the upper 120° of the cylinder as a curved
ETFE-laminated film, so orientation is irrelevant. The antenna is a
conformal helical whip recessed into the top cap. All external connectors
are IP68 M8 bulkheads.

Thermal management: the LiFePO4 chemistry is specified to -20 °C charge
and -40 °C discharge; below -30 °C the firmware disables charging and
runs on the supercapacitor bridge until the panel warms the pod. A small
1 W polyimide heater under the battery, driven only when solar input is
abundant, keeps the cell above -20 °C for charging. The ADS1256 and
geophone are rated to -40 °C; the ICM-42688 to -40 °C; the STM32WL55 to
-40 °C. The system's cold-start lower bound is -45 °C.

---

## 9. Bill of Materials ( indicative )

| Part | Qty | Unit USD | Note |
|---|---|---|---|
| STM32WL55JC | 1 | 9.50 | dual-core + LoRa |
| ADS1256 | 1 | 14.20 | 24-bit ADC |
| ICM-42688 | 1 | 3.10 | MEMS accel |
| NEO-M9N | 1 | 18.00 | GPS |
| SM-6B 4.5 Hz geophone | 3 | 22.00 | or 1× triaxial puck |
| BQ25895 | 1 | 3.40 | charger |
| TPS62740 | 1 | 2.10 | regulator |
| Fox F9A TCXO (26 MHz) | 1 | 2.80 | |
| Fox F9A TCXO (32.768 kHz) | 1 | 1.60 | RTC |
| 10 F supercap | 1 | 2.40 | |
| 5 W solar curved ETFE | 1 | 12.00 | custom |
| 2× 18650 LiFePO4 | 2 | 6.50 | protected |
| microSD socket + 32 GB card | 1 | 6.00 | |
| POM pod + machining | 1 | 18.00 | |
| PCB (4-layer) | 1 | 8.00 | |
| Passives, connectors, antenna | — | 12.00 | |
| **Total (per mote)** | | **~$220** | |

---

## 10. Directory Structure

```
glaciator-motes/
├── README.md                  # this file
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   ├── adc_driver.{c,h}
│   ├── mems_driver.{c,h}
│   ├── seismology.{c,h}
│   ├── lora_mesh.{c,h}
│   ├── gps_discipline.{c,h}
│   ├── power.{c,h}
│   ├── sdlog.{c,h}
│   ├── lz4.{c,h}
│   └── linker.ld              # (in build)
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── package.json
    ├── App.tsx
    └── src/
        ├── DeployWizard.tsx
        ├── GatewayDashboard.tsx
        ├── EventDetail.tsx
        ├── ArrayHealth.tsx
        └── ConfigSync.tsx
```

---

## 11. License & Credit

- **Hardware (schematics, PCB, mechanical):** CERN-OHL-S v2.
- **Firmware:** GPL-2.0.
- **Companion app:** MIT.

All designs, firmware, code, and documentation authored by **jayis1**.
Copyright © 2026 jayis1. All rights reserved.

This project uses a BSD-licensed subset of LZ4 (credited in `lz4.c`).