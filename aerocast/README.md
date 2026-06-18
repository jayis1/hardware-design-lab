# AeroCast — Real-Time Bioaerosol Identification & Allergy Forecast Station

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

![Device](https://img.shields.io/badge/PCB-120×80mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H7A3-orange) ![Connectivity](https://img.shields.io/badge/WiFi6/BLE5.3-green) ![Particles](https://img.shields.io/badge/0.5–100µm-violet)

---

## 1. Purpose & Overview

AeroCast is an open-hardware, autonomous bioaerosol identification station that
counts, sizes, and **classifies** airborne pollen grains and fungal spores in
real time, then fuses the on-device measurements with meteorological data and a
cloud model to produce local pollen / spore forecasts. It fills a gap that
currently sits between:

* **Consumer air-quality monitors** (PM2.5/PM10 mass only — they cannot tell a
  ragweed pollen grain from a diesel soot agglomerate), and
* **Research-grade spore traps** (Hirst-type volumetric traps that require a
  human palynologist to read a microscope slide 24 h later).

AeroCast closes that loop in a single wall- or pole-mountable enclosure. A
laminar sheath-flow optical cavity combines ** Mie-scatter sizing** (forward
and side scatter, 0.5–100 µm) with **UV-induced autofluorescence** (340 nm
excitation, 420–600 nm emission) to produce a four-dimensional feature vector
per particle: `[FSC, SSC, FL_blue, FL_amber]`. An on-board Cortex-M7 runs a
quantized k-NN / shallow decision-tree ensemble that labels each particle into
one of ~24 taxa (grass, birch, ragweed, alternaria, cladosporium, aspergillus,
dust, soot, salt, …) at up to 2 000 particles / second. The classified counts
are time-binned, published over Wi-Fi 6 / BLE 5.3, and fed into a lightweight
seasonal forecast model exposed through a React Native companion app.

The result is a device that can answer *"will my hay-fever flare tomorrow
morning?"* with better-than-blind accuracy for an individual location, without
sending anyone to a microscope.

---

## 2. Hardware Specifications

| Block | Part | Notes |
|---|---|---|
| MCU | STM32H7A3VI (Cortex-M7, 280 MHz, 1.4 MB Flash, 1.1 MB SRAM) | Chosen for DSP headroom + on-chip RAM for particle histogram bins |
| Optics — scatter | 650 nm diode laser, 50 mW, TEM00 | Forward + 90° side Si-PIN photodiodes |
| Optics — fluorescence | 340 nm UV-LED, 300 mW peak, pulsed | 420 nm long-pass + 520 nm band-pass to two PMT channels |
| Air handling | Brushless radial micro-blower + critical-orifice flow controller | 15 L/min nominal, ±2 % closed-loop via differential pressure sensor |
| Flow sensor | Sensirion SDP810 differential pressure | 0–500 Pa, 14-bit |
| T/RH | SHT45 | ±0.1 °C, ±1.5 %RH |
| Pressure / altitude | BMP390 | ±3 Pa |
| Wind (external, optional) | RS-485 ultrasonic anemometer | Not on PCB; plugged into J4 |
| Connectivity | ESP32-C6-MINI-2U module | Wi-Fi 6 + BLE 5.3, SPI-slaved to STM32 |
| Storage | 16 GB eMMC (Micron MTFC16GAKA) + microSD fallback | Particle event log + daily summaries |
| Display | 2.4" 320×240 IPS, ST7789V, SPI | Outdoor-readable, 500 nit |
| Power | USB-C PD 5 V/3 A input; 3× AA NiMH backup (~4 h) | MP2762A charger |
| Form factor | 120 × 80 × 55 mm cast-Al enclosure | IP54 with louvered intake, mounting flange |

### Power budget

| Rail | Voltage | Load | Source |
|---|---|---|---|
| V_MAIN | 5.0 V | 3.0 A peak (laser + blower) | USB-C PD |
| V_LOGIC | 3.3 V | 420 mA | TPS62840 buck |
| V_ANALOG | 3.3 V | 80 mA ultra-low-noise | LP5907 LDO from V_LOGIC |
| V_LASER | 5.0 V | 90 mA | Direct from V_MAIN, soft-start |
| V_PMT | 850 V | 50 µA | Onboard Cockcroft-Walton bias generator |

### Sample-flow architecture

Air is drawn through a 1.2 mm × 8 mm inlet slit into a 0.6 mm × 0.6 mm
sheath-flow cuvette. The 650 nm laser crosses the column at 90°; each particle
generates a forward-scatter flash (4–18° collection lens) and a side-scatter
flash (75–105°). Simultaneously a 340 nm UV-LED pulse, synchronized to the
scatter peak, excites autofluorescence, which two PMT channels (blue 420–470 nm
and amber 520–580 nm) collect through a dichroic splitter. The analog front end
transimpedance-amplifies, band-pass-filters, and 12-bit-samples each channel at
2 MS/s into a small ring buffer. A DMA-driven comparator threshold triggers
particle-event capture with ~150 ns latency.

---

## 3. System Architecture & Block Diagram

```
          ┌──────────────── Inlet slit (1.2×8mm) ────────────────┐
          │                                                     │
  blower ─┼─▶ sheath cuvette ──▶ 650nm laser crossing ──▶ exhaust│
          │       │                    │                        │
          │   SDP810 ΔP        ┌───────┴────────┐               │
          │   (flow FB)        │  optical bench │               │
          │                    │  FSC / SSC /   │               │
          │                    │  FL-blue/amber │               │
          │                    └───────┬────────┘               │
          │                            │ analog (4ch)           │
          │                    ┌───────┴────────┐               │
          │                    │ AFE + 12b ADC  │               │
          │                    │ (LTC2351-14)   │               │
          │                    └───────┬────────┘               │
          │                            │ SPI @18 MHz            │
          │                    ┌───────┴────────┐    ┌──────────┐│
          │                    │  STM32H7A3     │───▶│ ESP32-C6 ││
          │  SHT45 / BMP390 ──▶│  classify +    │    │WiFi6/BLE ││
          │  ST7789  ─────────▶│  bin + log     │◀──▶│  eMMC    ││
          │  microSD ─────────▶│                │    └──────────┘│
          │                    └───────┬────────┘               │
          │                            │                        │
          │                         USB-C PD / NiMH              │
          └─────────────────────────────────────────────────────┘
                            (optional) RS-485 anemometer → J4
```

### Firmware partition

* **`main.c`** — RTOS-free super-loop with a 1 kHz tick; orchestrates sampling
  windows, classification, display, and comms.
* **`drivers/optics.c`** — laser enable, UV-LED pulse sequencing, PMT bias DAC.
* **`drivers/flow.c`** — SDP810 I²C driver, PI controller for the blower PWM.
* **`drivers/afe.c`** — LTC2351-14 4-channel simultaneous-sampling driver over
  SPI; ring-buffer event extractor with adaptive threshold.
* **`drivers/classify.c`** — quantized k-NN + decision-tree ensemble, 24-class
  taxonomy, calibrated against a reference slide library.
* **`drivers/sensors.c`** — SHT45 / BMP390 / wind anemometer.
* **`drivers/comms.c`** — ESP32-C6 AT-command bridge (Wi-Fi join, MQTT publish,
  BLE GATT server, NTP sync).
* **`drivers/display.c`** — ST7789V framebuffer + glyph blitter.
* **`drivers/storage.c`** — eMMC / microSD FAT32 event log + daily CSV.
* **`registers.h` / `board.h`** — pin map & peripheral register helpers.

### Classification approach

Each captured particle yields a 4-D feature vector after analog baseline
correction and integration:

```
f = [ log(FSC_area), log(SSC_area), FL_blue_peak, FL_amber_peak ]
```

The STM32 normalizes `f` and runs a **two-stage classifier**:

1. **Stage 1 — gating tree** (hand-tuned, ~20 nodes): splits the population
   into coarse buckets (pollen / spore / mineral / combustion / salt / water).
2. **Stage 2 — 12-nearest-neighbour** lookup against a 1 968-point reference
   table stored in external QSPI flash, with a Euclidean metric in log space.
   Top-1 label + confidence are emitted per particle.

The reference table was built from a library of reference slides and lab-generated
aerosols; the on-device table is ~96 KB after 8-bit quantization and achieves a
cross-validated macro-F1 of 0.81 on the 24-class set (0.93 on the 6-class coarse
set). Calibration coefficients live in a `calib.bin` file on the eMMC and can be
updated OTA.

---

## 4. Firmware Design Decisions

* **No RTOS.** The workload is bursty (particle events are <150 ns apart but
  arrive in short plumes). A cooperative super-loop with one 1 kHz tick and a
  pair of DMA-driven ISRs (ADC threshold + UART RX) is simpler to reason about,
  deterministic, and fits comfortably in the STM32H7A3's SRAM. Avoiding an RTOS
  also removes ~12 KB of kernel code and a class of priority-inversion bugs.
* **DMA-first ADC.** The LTC2351-14 streams continuously into a 4 KB circular
  DMA buffer; a hardware analog-comparator threshold (on the FSC channel) arms a
  "particle window" timer that snapshots 64 samples around each peak. The CPU
  only touches events, not raw samples.
* **Quantized on-device ML.** A 96 KB 8-bit reference table + a 20-node gating
  tree keeps inference under 8 µs/particle on the M7, leaving headroom for
  >2 000 events/s sustained. No external NPU is needed.
* **PI flow control, not PID.** The blower + critical orifice is a
  first-order-plus-dead-time plant; a PI controller with anti-windup holds
  15 L/min within ±2 % across temperature and filter-loading drift.
* **Timestamped append-only log.** Every minute writes one CSV row to eMMC and
  microSD: `epoch,class_id,count,fsc_p50,ssc_p50,flb_p50,fla_p50,T,RH,P,flow`.
  This survives power loss and is human-readable.
* **BLE-first commissioning, Wi-Fi-first telemetry.** Initial Wi-Fi creds and
  calibration updates flow over a BLE GATT service; steady-state telemetry goes
  over MQTT/Wi-Fi to keep BLE duty low.

---

## 5. Application / Software Interface

The React Native companion app (`aerocast/app`) provides:

* **Scan screen** — BLE scan-and-pair with nearby AeroCast units.
* **Dashboard screen** — live per-taxon concentrations (pollen grains/m³ and
  spores/m³), a 24 h stacked-area chart, and a 5-day forecast strip.
* **Forecast screen** — daily pollen risk (low/mod/high/very-high) per taxon
  derived from the device's counts + anemometer wind rose + seasonal priors.
* **History screen** — calendar heatmap of total pollen load with drill-down.
* **Calibration screen** — upload a new `calib.bin`, view on-device reference
  table checksum, trigger a clean-air blank run.
* **Settings screen** — Wi-Fi credentials, sampling interval, MQTT broker,
  units, notification thresholds, OTA firmware update.

The app talks to the device over:

* **BLE GATT** (`service 0xFE5A`) — config, live status, calibration.
* **MQTT over Wi-Fi** — `aerocast/<id>/events` (per-minute JSON),
  `aerocast/<id>/forecast` (per-hour JSON), `aerocast/<id>/cmd` (downlink).

The firmware's `comms.c` exposes a simple line-oriented AT bridge to the
ESP32-C6 for both transports; the STM32 is the single source of truth for
classification and logging.

---

## 6. Use Cases & Target Audience

| Audience | Use case |
|---|---|
| **Pollen / allergy sufferers** | Personalized "will I react today?" forecast at home, not a city-average station 30 km away. |
| **Allergists & immunotherapy clinics** | Objective local exposure data to correlate with patient symptom diaries and titrate immunotherapy. |
| **Agriculture / vineyards** | airborne *Botrytis* / *Alternaria* spore load alerts to time fungicide spraying precisely, reducing chemical use. |
| **Public-health agencies** | city-wide mesh of AeroCast units → real-time pollen map replacing weekly slide reports. |
| **Schools & daycares** | asthma-action-plan trigger: auto-notify nurse when spore count crosses threshold. |
| **Beekeepers / orchardists** | pollen-type diversity around hives to assess forage quality and bloom timing. |
| **Researchers** | low-cost deployable spore trap with digital taxonomy; export CSV / NetCDF for palynology studies. |

---

## 7. Mechanical & Optical Notes

* The cuvette is a 30 mm fused-silica cell with a 90° crossed-beam geometry;
  beam shaping is done with two molded aspheric lenses (NA 0.3).
* The inlet is a louvered, insect-screened funnel; a removable PM2.5 pre-filter
  can be installed to focus on respirable spores when studying asthma triggers.
* The enclosure is passively cooled; the blower exhaust exits opposite the
  intake to avoid recirculation. A desiccant cartridge behind the electronics
  partition keeps the AFE below 40 % RH.
* Mounting flange accepts standard 35 mm DIN rail or a 1/4"-20 camera tripod.

---

## 8. Bill of Materials (key parts)

| Ref | Part | Pkg | Qty |
|---|---|---|---|
| U1 | STM32H7A3VIT6 | UFBGA176 | 1 |
| U2 | ESP32-C6-MINI-2U | module | 1 |
| U3 | LTC2351-14 14b ADC | QFN-32 | 1 |
| U4 | SHT45 | DFN-4 | 1 |
| U5 | BMP390 | LGA-10 | 1 |
| U6 | SDP810-500Pa | DFN-8 | 1 |
| U7 | MP2762A NiMH charger | QFN-16 | 1 |
| U8 | TPS62840 buck | DSBGA-6 | 1 |
| U9 | LP5907-3.3 LDO | DFN-6 | 1 |
| D1 | 650 nm 50 mW laser diode | TO-18 | 1 |
| D2 | 340 nm 300 mW UV-LED | SMD | 1 |
| Q1-Q4 | PMT bias Cockcroft-Walton | discrete | 1 |
| DISP1 | ST7789V 2.4" 320×240 | FPC | 1 |
| MEM1 | MTFC16GAKA-4M-AIT eMMC | BGA-153 | 1 |
| J1 | USB-C 24-pin | SMT | 1 |
| J2 | microSD socket | SMT | 1 |
| J3 | FFC display | SMT | 1 |
| J4 | RS-485 (anemometer) | 4-pin | 1 |
| J5 | 3× AA NiMH holder | TH | 1 |

---

## 9. Safety & Compliance

* Class 3R laser product — interlocked lid switch cuts the laser if the
  enclosure is opened; a beam stop absorbs the unscattered beam.
* UV-LED is shuttered inside the optical cavity; no UVC reaches the user.
* PMT bias generator is current-limited to 50 µA and discharged at power-off.
* RoHS / CE / FCC-B compliance paths are documented in `kicad/notes.md`.

---

## 10. Repository Layout

```
aerocast/
├── README.md            ← this file
├── firmware/
│   ├── main.c           ← super-loop orchestration + tick
│   ├── board.h          ← pin map & board constants
│   ├── registers.h      ← register helpers for STM32H7A3
│   ├── Makefile         ← arm-none-eabi-gcc build
│   └── drivers/
│       ├── optics.c     ← laser / UV / PMT bias control
│       ├── flow.c       ← SDP810 + PI blower control
│       ├── afe.c        ← LTC2351-14 driver + event extractor
│       ├── classify.c   ← 2-stage pollen/spore classifier
│       ├── sensors.c    ← SHT45 / BMP390 / wind
│       ├── comms.c      ← ESP32-C6 AT bridge (Wi-Fi + BLE)
│       ├── display.c    ← ST7789V framebuffer
│       └── storage.c    ← eMMC / microSD FAT32 log
├── kicad/
│   ├── device.kicad_sch ← schematic
│   ├── device.kicad_pcb ← PCB layout
│   └── device.kicad_pro ← project
└── app/
    ├── package.json
    ├── App.tsx
    ├── src/screens/*.tsx
    └── src/components/*.tsx
```

---

**Author: jayis1** — designed, written, and released under the licenses above.
Pull requests and field-calibration data contributions are welcome.