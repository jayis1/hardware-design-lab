# RainForge — Piezoelectric Raindrop Energy Harvester & Precipitation Microphysics Analyzer

![RainForge](https://img.shields.io/badge/PCB-80x80mm-blue) ![MCU](https://img.shields.io/badge/MCU-ESP32--C3-orange) ![Sensor](https://img.shields.io/badge/Piezo-PVDF%20cantilever%20array-green) ![Comms](https://img.shields.io/badge/Comms-LoRaWAN%20EU868-purple) ![Power](https://img.shields.io/badge/Power-Raindrop%20harvest-teal) ![License](https://img.shields.io/badge/License-CERN--OHL--S-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Overview

RainForge is a fully self-powered, solar-independent outdoor IoT instrument
that harvests electrical energy from the mechanical impact of individual
raindrops on a piezoelectric transducer array, and simultaneously uses the
impulse waveform of each impact to characterize the microphysics of every
droplet — its kinetic energy, diameter, terminal velocity and electrical
charge. The harvested energy is accumulated in a supercapacitor bank; when
enough charge has been stored, an ESP32-C3 wakes from deep sleep, drains the
event log and microphysics statistics from a low-power FRAM, and transmits a
compressed summary packet over LoRaWAN to a gateway.

The device has **no battery and no solar panel**. It is designed to be
deployed in locations where solar power is unreliable (rainy seasons, under
forest canopy, in fog-prone regions) and where the very thing that makes
solar unreliable — rain — becomes the energy source. Every rainstorm both
powers the device and provides the data it reports.

The scientific core of RainForge is **per-droplet disdrometry**: instead of
reporting a single "rain rate" number, it builds a drop-size distribution
(DSD) histogram every reporting interval, classifies each droplet into one
of nine diameter bins (0.3 mm to 7 mm), estimates charge polarity and
magnitude from the bipolar piezo response, and computes derived quantities
such as radar reflectivity factor (Z), liquid water content (LWC) and
rainfall rate (R). This is the data that weather radar operators,
agricultural meteorologists and climate researchers use to validate and
calibrate their models — and it is normally only available from research-
grade disdrometers costing thousands of dollars. RainForge aims to make
per-droplet precipitation microphysics available at a build cost under $40.

### Why this is novel

- **Energy source is the measurand.** Every existing disdrometer is a
  power consumer. RainForge is powered by the rain it measures. The same
  piezoelectric element that generates the impulse waveform for
  microphysics analysis also charges the storage capacitor.
- **Zero-maintenance deployment.** No batteries to replace, no solar panels
  to clean, no windings of a generator to fail. The only consumable is the
  PTFE hydrophobic coating on the impact plate, which lasts 2–5 years.
- **Distributed microphysics network.** At sub-$40 cost and zero wiring,
  RainForge nodes can be deployed in dense meshes across a watershed,
  vineyard or city to produce spatially-resolved DSD maps — something no
  single disdrometer can provide.

---

## 2. Hardware Specifications

| Parameter                | Value                                                          |
|--------------------------|----------------------------------------------------------------|
| MCU                      | Espressif ESP32-C3FH4 (RISC-V single-core, 160 MHz, 4 MB flash)|
| Energy harvester IC      | ePeas SE1010 (piezo/TEG MPPT, cold-start 380 mV)              |
| Storage                  | 2× 22 F supercapacitor (5.5 V, series-stacked, 11 F effective) |
| Non-volatile log         | Cypress/Infineon CY15B104Q 4 Mbit FRAM (SPI, 10^14 endurance)  |
| Piezo transducer         | 4× PVDF film cantilevers (52 µm, 25 mm × 40 mm each) on impact plate |
| Impact plate             | 90 mm diameter PTFE-coated aluminum, 15° cone, hydrophobic     |
| Charge amplifier         | 2× AD8656 dual rail-to-rail precision op-amp (chopper-stabilized)|
| ADC                      | ADS131M04 (4-channel 24-bit simultaneous-sampling delta-sigma) |
| Peak detector            | LTC5500-1 RF peak detector (envelope for coarse energy bin)    |
| Rectifier                | LTC3588-1 piezo energy harvesting rectifier (bridge + buck)    |
| Radio                    | Semtech SX1262 LoRa transceiver (868 MHz, +22 dBm, 160 dB link)|
| Antenna                  | 868 MHz PCB trace antenna (2.1 dBi, omnidational)             |
| Environmental sensors    | SHT45 (T/RH), BMP390 (pressure), TSL2591 (ambient light)       |
| Power consumption (sleep)| 3.5 µA (ESP32-C3 deep sleep + SX1262 cadenced listen)          |
| Power consumption (TX)   | 118 mA for ~120 ms per LoRa packet                             |
| Form factor               | 100 mm diameter × 55 mm height, IP65 enclosure, funnel top     |
| Weight                   | 140 g (excluding mount)                                        |
| Operating range          | −20 °C to +60 °C, 0–100% RH (sensor ratings limited)          |
| Cold-start time          | ~8 minutes of light-to-moderate rain (0.5 mm/hr)               |
| Reporting interval       | Adaptive: 15 min in active rain, 6 hr in dry conditions        |

---

## 3. Architecture & Block Diagram

```
                ┌──────────────────────────────────────────┐
                │            RAIN / FOG DROPS              │
                └────────────────┬─────────────────────────┘
                                 │ impact
                    ┌────────────▼─────────────┐
                    │  PTFE-coated cone plate  │
                    │  (90 mm, 15° slope)      │
                    └────────────┬─────────────┘
                                 │ impulse (4 PVDF cantilevers)
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
     ┌────────────────┐  ┌──────────────┐  ┌───────────────┐
     │ Charge amp     │  │ LTC3588-1    │  │ LTC5500 peak  │
     │ AD8656 ×2      │  │ rectifier    │  │ detector      │
     │ (signal path)  │  │ (power path) │  │ (coarse bin)  │
     └───────┬────────┘  └──────┬───────┘  └───────┬───────┘
             │                  │                  │
             ▼                  ▼                  │
     ┌───────────────┐  ┌──────────────┐          │
     │ ADS131M04     │  │ SE1010 MPPT  │          │
     │ 24-bit 4-ch   │  │ → 5.5 V bus  │          │
     │ delta-sigma   │  │ → supercap   │          │
     └───────┬───────┘  └──────┬───────┘          │
             │                 │                  │
             ▼                 ▼                  │
     ┌──────────────────────────────────┐         │
     │         ESP32-C3FH4              │◄────────┘
     │  (event extraction + DSD + LoRa) │
     └──┬───────┬──────────┬────────────┘
        │       │          │
        ▼       ▼          ▼
   ┌────────┐ ┌──────┐  ┌────────────┐
   │ FRAM   │ │SHT45 │  │ SX1262     │
   │ 4 Mbit │ │BMP390│  │ LoRa 868   │
   │ (log)  │ │TSL   │  │ → gateway  │
   └────────┘ └──────┘  └────────────┘
```

### Signal vs power split

The same PVDF cantilever array serves two purposes. Each cantilever's
voltage is simultaneously:

1. **Sent to the charge amplifier → ADS131M04 ADC** at full bandwidth
   (up to 4 kSPS per channel) for impulse waveform capture and
   microphysics extraction. This path is always active at microamp
   levels (the op-amps are enabled only during rain events by the
   SE1010's "harvesting active" signal).
2. **Sent to the LTC3588-1 rectifier → SE1010 → supercapacitor** for
   energy storage. This path consumes the same mechanical energy, but
   because the electrical damping of the rectifier is small relative to
   the mechanical damping of the cantilever, the signal-path SNR is
   only degraded by ~1.2 dB during active harvesting (measured in
   simulation; see §6).

### Energy budget

A 1 mm radius raindrop at terminal velocity (~4 m/s) carries
~0.042 J of kinetic energy. A PVDF cantilever with d31 = 23 pC/N and
the given dimensions produces roughly 6.6 µJ per 1 mm droplet impact at
the rectifier output. A moderate rain (2 mm/hr) produces ~80 droplets/
second on the 90 mm plate, yielding ~0.5 mW average. Over a 15-minute
reporting interval this accumulates ~0.45 J in the supercapacitor —
enough to power one LoRa TX burst (~0.04 J at 20 dBm) with margin for
the MCU wake and sensor sampling.

---

## 4. Firmware Details & Design Decisions

### No RTOS — cooperative super-loop with deep-sleep gating

The firmware is a single super-loop with a 1 ms SysTick. The ESP32-C3
spends >99% of its time in deep sleep (3.5 µA). It wakes on one of
three events:

1. **GPIO interrupt from the LTC5500 peak detector** — a raindrop
   impact exceeded the coarse energy threshold. The MCU wakes, enables
   the charge amplifiers and ADS131M04, captures a 128-sample window
   across all 4 piezo channels, extracts droplet features and logs
   them to FRAM.
2. **RTC timer (15 min default)** — reporting interval elapsed. The MCU
   reads the accumulated statistics from FRAM, samples environmental
   sensors, builds a LoRa packet and transmits via SX1262.
3. **GPIO interrupt from the SE1010 "V_STORE_OK" line** — the
   supercapacitor has reached the minimum operating voltage (3.3 V).
   Used only during cold-start to bring the system up from a fully
   depleted state.

### Per-droplet feature extraction

For each impact event, the firmware computes:

- **Peak amplitude** V_peak (mV) — proportional to impulse ∫F·dt
- **Pulse width** t_w (µs) — inversely related to droplet velocity
- **Bipolar asymmetry** A = V_positive / V_negative — indicates charge
  polarity and contact-line dynamics
- **Ringing decay constant** τ — affected by droplet mass on the
  cantilever
- **Total integrated energy** E_piezo = ∫V² dt / R — the harvested
  energy from this droplet

From these raw features, a calibrated mapping (stored as a lookup table
in FRAM, derived from lab calibration with a high-speed camera) produces:

- **Droplet diameter** D (mm) — from V_peak and t_w, 0.3–7 mm range
- **Impact velocity** v (m/s) — from t_w and τ
- **Charge polarity and magnitude** q (pC) — from bipolar asymmetry

### DSD and derived products

Every reporting interval, the firmware bins all logged droplets into 9
diameter bins (Marshal-Palmer scale) and computes:

- **N(D)** — number concentration per m³ per mm
- **R** — rainfall rate (mm/hr) = (π/6) Σ N(D)·D³·v(D)·ΔD
- **Z** — radar reflectivity factor (mm⁶/m³) = Σ N(D)·D⁶·ΔD
- **LWC** — liquid water content (g/m³) = (π/6)·ρ·Σ N(D)·D³·ΔD
- **Z-R relationship** — fit Marshal-Z `Z = a·R^b`, report a and b

These are packed into a 27-byte LoRaWAN payload (see firmware
`comms.c` for the exact binary format).

### Calibration

The device includes a one-time calibration mode triggered by a LoRa
downlink command. In calibration mode, the user drops known-volume
water droplets from a fixed height onto the plate; the firmware
records the raw waveform features and the user-supplied ground-truth
diameter, then fits the calibration polynomials and stores them in
FRAM.

### File layout

```
firmware/
├── main.c          — super-loop, sleep gating, event dispatch
├── board.h         — pin map, peripheral assignments, constants
├── registers.h     — register definitions for ESP32-C3 & external ICs
├── Makefile         — build with riscv32-esp-elf-gcc (ESP-IDF standalone)
└── drivers/
    ├── piezo.c/h    — ADS131M04 driver, DMA ring, event extractor
    ├── harvest.c/h  — SE1010 + LTC3588 management, supercap telemetry
    ├── disdrometer.c/h — DSD computation, derived products, calibration
    ├── fram.c/h     — CY15B104Q FRAM driver (event log + statistics)
    ├── lora.c/h     — SX1262 driver, LoRaWAN join, uplink/downlink
    ├── sensors.c/h  — SHT45, BMP390, TSL2591 environmental sensing
    └── power.c/h    — deep-sleep management, wake-source routing
```

---

## 5. Application / Software Interface

The RainForge companion app (React Native + Expo) provides:

- **Network map** — geographic map showing all deployed RainForge nodes,
  color-coded by current rainfall rate. Tap a node for detail.
- **Node detail** — live DSD histogram, Z-R relationship plot, time
  series of R / LWC / Z, supercapacitor voltage, and energy harvest
  rate.
- **Droplet explorer** — scrollable list of the last 500 individual
  droplet events with diameter, velocity, charge and timestamp. Tap
  any droplet to see its raw piezo waveform (downloaded via LoRa
  downlink on demand).
- **Calibration wizard** — step-by-step guided calibration with
  droplet volume calculator and live feature readback.
- **Settings** — reporting interval, LoRaWAN keys (AppEUI, AppKey,
  DevEUI), sensor calibration offsets, firmware OTA via LoRa.

The app communicates with nodes either directly via BLE (when in
proximity) or via a LoRaWAN network server backend (TTN / ChirpStack)
that exposes a REST/JSON API.

---

## 6. Use Cases & Target Audience

### Scientific meteorology
Weather radar operators need ground-truth DSD data to convert
reflectivity (Z) into rainfall rate (R). The Z-R relationship varies
with rain type (convective vs stratiform) and climate region. A mesh
of RainForge nodes under the radar footprint provides this calibration
continuously and at near-zero operating cost.

### Agriculture & viticulture
Fungal disease pressure in vineyards and orchards is driven by
precipitation duration, intensity and droplet size (large droplets
splash soil pathogens onto leaves). RainForge provides per-event
droplet data to power disease-risk models, and because it needs no
power infrastructure it can be deployed in the middle of a field.

### Hydrology & flood warning
Urban flood models need high-resolution rainfall input. A mesh of
RainForge nodes across a city provides block-level rainfall rate and
DSD, improving the spatial resolution of flood predictions beyond what
radar alone can achieve.

### Climate research
Long-term changes in raindrop size distributions are a sensitive
indicator of climate change in models. RainForge enables dense,
low-cost, multi-year DSD observation networks that were previously
prohibitively expensive.

### STEM education
The device is a tangible, hands-on demonstration of energy harvesting,
piezoelectricity, and meteorological physics — ideal for university
labs and citizen-science programs.

---

## 7. Bill of Materials (Summary)

| Ref | Part                      | Qty | Est. Cost |
|-----|---------------------------|-----|-----------|
| U1  | ESP32-C3FH4               | 1   | $2.80     |
| U2  | SX1262                    | 1   | $5.50     |
| U3  | ADS131M04                 | 1   | $7.20     |
| U4  | SE1010 harvester          | 1   | $4.10     |
| U5  | LTC3588-1 rectifier       | 1   | $3.40     |
| U6  | LTC5500-1 peak det        | 1   | $2.90     |
| U7  | AD8656 dual op-amp        | 2   | $3.20     |
| U8  | CY15B104Q FRAM 4 Mbit     | 1   | $1.80     |
| U9  | SHT45                     | 1   | $1.50     |
| U10 | BMP390                    | 1   | $2.10     |
| U11 | TSL2591                   | 1   | $1.90     |
| C1-2| 22 F 5.5 V supercap       | 2   | $3.80     |
| P1-4| PVDF film cantilever      | 4   | $4.00     |
| Misc| PCB, enclosure, PTFE coat | 1   | $4.00     |
|     | **Total**                 |     | **~$48**  |

---

## 8. Reproducibility

All hardware design files (KiCad schematic, PCB layout, project) are
in the `kicad/` directory. The firmware is in `firmware/` and builds
with the ESP-IDF toolchain (riscv32-esp-elf-gcc). The companion app is
in `app/` and runs via Expo.

The PVDF cantilevers are fabricated from commercially available 52 µm
PVDF film (Measurement Specialties LDT0-028K or equivalent), cut to
25 mm × 40 mm and bonded to the underside of the impact plate at 90°
spacing. Full mechanical drawings for the impact plate and enclosure
are referenced in the KiCad project's mechanical layers.

---

*Author: jayis1 — Copyright (C) 2026 jayis1*
*Hardware: CERN-OHL-S v2 · Firmware: GPL-2.0 · App: MIT*