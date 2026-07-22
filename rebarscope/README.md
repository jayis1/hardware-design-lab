# RebarScope — Open-Source Concrete Corrosion NDT Scanner & Rebar Cover Mapper

![PCB](https://img.shields.io/badge/PCB-148%C3%9774mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32L4R5-orange) ![ADC](https://img.shields.io/badge/ADC-ADS1220%20%2B%20AD5940-green) ![Modality](https://img.shields.io/badge/NDT-HCP%20%2B%20Resistivity%20%2B%20Cover%20%2B%20LPR-purple) ![Comms](https://img.shields.io/badge/Comms-BLE%205.0%20%2B%20USB--C-teal) ![Power](https://img.shields.io/badge/Power-18650%20Li--ion-red) ![Author](https://img.shields.io/badge/Author-jayis1-orange) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)**

> **RebarScope is a handheld, battery-powered, fully open-source non-destructive testing instrument for reinforced-concrete structures. It fuses three complementary inspection modalities — half-cell potential (HCP) mapping per ASTM C876, Wenner four-probe concrete electrical resistivity, and pulsed eddy-current rebar cover-depth / diameter estimation — plus an optional linear-polarization-resistance (LPR) corrosion-rate mode, into a single wand with a wheel encoder and IMU for self-referenced grid surveying. A React Native companion app builds live 2D corrosion-probability heatmaps, cover-depth maps, and ASTM-compliant inspection reports over BLE 5. No open-source device combines these modalities today; commercial equivalents (Proceq Resipod, Germann CorMap, Hilti PS 250) cost $3,000–$10,000 and lock every algorithm behind proprietary firmware.**

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [The Problem with Concrete Corrosion NDT](#2-the-problem-with-concrete-corrosion-ndt)
3. [Novelty](#3-novelty)
4. [Sensing Modalities](#4-sensing-modalities)
5. [Hardware Specifications](#5-hardware-specifications)
6. [Architecture & Block Diagram](#6-architecture--block-diagram)
7. [Firmware Design](#7-firmware-design)
8. [Calibration & Reference Electrodes](#8-calibration--reference-electrodes)
9. [Companion Application](#9-companion-application)
10. [Use Cases & Target Audience](#10-use-cases--target-audience)
11. [Power Budget](#11-power-budget)
12. [Bill of Materials](#12-bill-of-materials)
13. [Safety & Limitations](#13-safety--limitations)
14. [License](#14-license)

---

## 1. Purpose & Overview

Reinforced concrete is the most widely used structural material on Earth — and rebar corrosion inside it is the dominant cause of premature structural failure, costing the global economy an estimated 3–5% of GDP annually. Chloride ingress from marine exposure or de-icing salts destroys the passive oxide film on steel rebar; carbonation lowers concrete pH; both trigger active corrosion. The expanding rust exerts tensile stresses exceeding 3× concrete tensile strength, spalling the cover and exposing rebar to accelerated attack long before the structure's design life ends.

The good news: the *electrochemistry* of rebar corrosion produces measurable signals **years before** visible spalling. As the rebar transitions from passive to active, its half-cell potential shifts from mildly positive (≥ −200 mV vs Cu/CuSO₄) to strongly negative (≤ −350 mV). Simultaneously, the concrete's bulk electrical resistivity drops, because corrosion products and chloride-laden pore solution are more conductive. And the thickness and condition of the concrete *cover* over the rebar — the physical barrier protecting it — can be verified non-destructively by eddy-current sensing, locating the rebar and measuring its depth without any X-ray or radar licensing. A corrosion rate can additionally be estimated by linear polarization resistance (LPR), where a small polarization around the free corrosion potential is applied and the resulting current gives the kinetics of the anodic/cathodic reaction.

RebarScope unifies all four of these measurements into a single handheld wand that an inspector walks across a slab, bridge deck, or parking garage. A wheel encoder plus 6-axis IMU tracks the probe's position on the surface so that the user needs only a single starting reference mark — the device builds its own survey grid internally. Each tap of the probe (or continuous rolling in low-resolution mode) records a data tuple `{x, y, E_hcp, ρ, d_cover, i_corr}` and streams it over BLE to the phone app, which rasterizes the survey into color heatmaps, segments hotspots with ASTM C876 thresholds, and exports a signed PDF report keyed to GPS-tagged photos.

The instrument is built around an STM32L4R5 low-power Cortex-M4F MCU, a precision 24-bit delta-sigma ADC (TI ADS1220) for DC potential, an Analog Devices AD5940 impedance AFE for AC resistivity and LPR, a discrete eddy-current analog front-end (excitation oscillator + synchronous demodulator) for cover depth, a BGM220E BLE 5 module, a 128×64 monochrome OLED for live readout, and a single 18650 cell for a full day of surveying. Everything — schematic, PCB, firmware, and app — is open, field-calibratable, and documented.

---

## 2. The Problem with Concrete Corrosion NDT

| Problem | Impact |
|---|---|
| Commercial tools are closed & expensive | A half-cell + resistivity kit from one vendor easily exceeds $4,000; a combined cover meter adds another $1,500. Firmware is binary-only, calibration coefficients are hidden, and data export is locked to subscription desktop software. |
| Modalities are split across devices | Inspectors carry a half-cell potential wheel, a separate Wenner resistivity probe, and a separate cover meter — three wands, three batteries, three coordinate systems that must be manually reconciled. |
| No positional self-referencing | Every commercial system requires the user to chalk a 1×1 m grid on the concrete beforehand. On a 5000 m² bridge deck that is hours of layout work and a source of error. |
| No open corrosion-rate option | LPR is a standard technique (ASTM G59) that converts potential-polarization current into corrosion penetration rate (mm/year), giving a quantitative prognosis, not just a binary "corroding / not" verdict. Commercial handhelds rarely include it; lab potentiostats cost thousands and aren't field tools. |
| Heatmaps live in a silo | Most devices dump CSV files; correlating the half-cell map with the cover-depth map requires post-processing in a GIS or MATLAB. |

RebarScope attacks all five at once.

---

## 3. Novelty

No open-source hardware project combines half-cell potential, concrete resistivity, eddy-current cover depth, and LPR corrosion rate into one instrument with self-referenced positioning and a phone-based heatmap app. Existing open efforts are single-modality hobby projects (a voltmeter wired to a Cu/CuSO₄ electrode, or a Wenner probe using an off-the-shelf LCR meter) with no fusion, no coordinate tracking, and no reporting pipeline. RebarScope's specific contributions are:

1. **Tri-modal electrochemical + electromagnetic fusion** — simultaneously quantifying *probability* (HCP), *conduciveness of the environment* (resistivity), and *physical protection* (cover depth) at the same coordinate, so an inspector can tell the difference between "corroding because the cover is thin" and "corroding because chlorides have penetrated thick cover."
2. **Self-referenced surveying** — a wheel encoder (0.5 mm resolution) fused with a 6-axis IMU (drift-corrected dead-reckoning) eliminates chalk-grid layout. The operator walks a serpentine path; the device builds the grid.
3. **On-device LPR corrosion-rate estimation** — a tiny, low-current potentiostat function (≤ ±250 mV polarization, ≤ 10 µA compliance) implemented through the AD5940, using the rebar itself as the working electrode and a wet sponge counter/reference electrode. No commercial handheld below $2,000 does this.
4. **ASTM C876 classification on-device** — potential bins (< −350 mV, −200 to −350 mV, > −200 mV) and resistivity bins (≥ 200 kΩ·cm, 100–200, 50–100, < 50) computed in firmware so the OLED shows live classification.
5. **Open, calibration-capable, signed-report workflow** — the phone app signs each measurement record with the device's unique ID and calibration date and exports a tamper-evident report (SHA-256 hash chain), suitable for chain-of-custody inspection records.

---

## 4. Sensing Modalities

### 4.1 Half-Cell Potential (HCP) — ASTM C876

A Cu/CuSO₄ saturated reference electrode is pressed against a wetted patch of concrete; the rebar inside is electrically connected (via an exposed rebar end or a drilled-and-tapped connection). The open-circuit voltage between reference and rebar is measured with a high-impedance buffer (>1 GΩ, AD8603) into a 24-bit delta-sigma ADC (ADS1220) at 20 SPS with 50/60 Hz rejection. The reading, in mV vs Cu/CuSO₄, is classified per ASTM C876:

| Potential (mV vs Cu/CuSO₄) | Interpretation (C876) |
|---|---|
| > −200 | 90% probability of **no corrosion** |
| −200 to −350 | Uncertain (active corrosion possible) |
| < −350 | 90% probability of **active corrosion** |

A silver/silver-chloride (Ag/AgCl) electrode is supported as an alternative, with an offset constant (≈ +70 mV relative to Cu/CuSO₄) applied in firmware.

### 4.2 Wenner Four-Probe Resistivity

Four equally-spaced spring-loaded contact pins (Wenner α-array, α = 50 mm) are pressed into the concrete surface. The AD5940 injects a 1 kHz, 10 µA RMS sinusoid between the outer two pins and measures the voltage between the inner two pins, demodulated synchronously (lock-in) to reject 50/60 Hz and harmonic pickup. Bulk resistivity is:

    ρ = 2πa · (V / I)   [Ω·m]

The firmware averages 32 phase-locked samples, discards outliers via a median filter, and applies a surface-contact correction factor k(θ) based on the measured contact impedance. The resistivity map identifies chloride-laden or water-saturated zones — areas where corrosion is *likely* even if the potential is still ambiguous.

### 4.3 Pulsed Eddy-Current Cover Depth & Diameter

A ferrite-cored coil is pulsed at a 1 kHz repetition rate with a 100 µs current ramp (0 → 100 mA). The decaying magnetic field induces eddy currents in the steel rebar; their secondary field is captured by a sensing coil and synchronously demodulated. The **peak time** t_p of the decay waveform is proportional to cover depth:

    d ≈ √(η · σ_rebar · t_p)   with σ_rebar ≈ 1.1×10⁶ S/m

Calibrated against known cover depths (12, 25, 38, 50 mm) the firmware solves a piecewise-linear d(t_p) curve. Rebar **diameter** is estimated from the amplitude of the secondary peak relative to depth, distinguishing #4 (13 mm) from #6 (19 mm) and #8 (25 mm) bars within ±2 mm in the 5–60 mm range.

### 4.4 Linear Polarization Resistance (LPR) — Optional

When an exposed rebar connection is available, the user places a small wet counter electrode sponge on the surface and the device sweeps the rebar potential ±25 mV around the free corrosion potential E_corr at 0.1 mV/s while measuring current (0.01 µA resolution). The polarization resistance R_p is the slope ΔE/ΔI; corrosion current density is:

    i_corr = B / R_p ,    B ≈ 26 mV (active), 52 mV (passive)

and corrosion penetration rate:

    v_corr (mm/yr) = 0.00327 · i_corr (µA/cm²) · (M/n) ,  (M/n)_Fe = 27.9 g/equiv

This converts a binary "corroding / not" HCP verdict into a quantitative **mm/year** loss — the number that actually predicts remaining service life.

---

## 5. Hardware Specifications

| Parameter | Specification |
|---|---|
| MCU | STM32L4R5ZIT6 — Cortex-M4F @ 120 MHz, 640 KB SRAM, 2 MB Flash, hardware crypto (for report hashing) |
| BLE module | Silicon Labs BGM220E — BLE 5.0, UART HCI, 2.4 GHz, internal antenna |
| DC potential ADC | TI ADS1220IPW — 24-bit ΔΣ, 20 SPS, PGA 1–128, 50/60 Hz notch, SPI |
| Buffer amp | AD8603AUJZ — auto-zero, 1 µV max offset, 1 pA bias, >1 GΩ input |
| Impedance AFE | AD5940BCPZ — 0.1 Hz–200 kHz signal gen, 16-bit ADC 800 kS/s, TIA, DFT engine, for Wenner & LPR |
| Eddy-current analog | Discrete: AD9833 DDS → OPA569 power driver → sense coil; AD8295 PGA → ADS1220 demodulated sample |
| Reference electrode | Cu/CuSO₄ saturated (default), Ag/AgCl (selectable in firmware) |
| Resistivity contacts | 4× spring-loaded gold-plated pins, 50 mm Wenner spacing |
| Eddy coil | Custom ferrite-cored, Ø20 mm, 100 turns, 220 µH |
| Position sensing | Wheel encoder: 200 PPR quadrature (0.5 mm/edge); IMU: LSM6DSO 6-axis |
| Display | 128×64 monochrome OLED (SSD1306, SPI) |
| Storage | microSD (SPI mode) — raw CSV logging, calibration data |
| Power | 1× 18650 Li-ion (3.4 Ah), MCP73831 charger, MAX17048 fuel gauge, ~14 h survey life |
| Connectivity | USB-C (CDC virtual serial + charging); BLE 5.0 (GATT survey profile) |
| Form factor | Wand, Ø 42 mm × 260 mm, IP54, replaceable electrode tip |
| Operating conditions | −10 to +50 °C; non-conductive dry concrete surface; moisture on surface acceptable for HCP/wet Wenner |
| Mass | ~380 g with battery |

---

## 6. Architecture & Block Diagram

```
                     +-------------------------------+
                     |      STM32L4R5 MCU            |
                     |  (Cortex-M4F @ 120 MHz)       |
                     |  - survey state machine       |
                     |  - HCP classifier (C876)       |
                     |  - Wenner ρ computation        |
                     |  - eddy cover-depth solver    |
                     |  - LPR sweep + i_corr         |
                     |  - position filter (encoder+IMU)|
                     |  - BLE GATT server (UART)     |
                     |  - SHA-256 report hash chain  |
                     +---+-----+-----+-----+----+----+
                         |     |     |     |    |
            SPI   +------+ |   |     |     |    +--------+  UART
        ADS1220   |  HCP  |   |     |     |             |    BGM220E
        (24-bit)  |  ADC  |   |     |     |   BLE       |   (BLE 5.0)
        +AD8603  +-------+   |     |     |  module      +-------+
                     SPI|   |SPI   |GPIO|              |  HCI
                  +------+   |     |     |              +---+
        AD5940    | Wenner    |     |     |   wheel enc |   |
        (AFE)     | + LPR     |     |     +--- LSM6DSO  |   |
        + 4-pin   | AFE       |     |     +-------------+   |
        + sponge  +-----------+     |                      |
                              SPI   |                      |
                      +--------------+                      |
        AD9833 DDS    | Eddy-current |                      |
        + OPA569      | driver +     |                      |
        + sense coil  | demodulator  |                      |
                      +--------------+                      |
                                                            |
        +----------------+    I2C    +-----------------------+
        | microSD (SPI)  |           | SSD1306 OLED display  |
        | MAX17048 FG    |           +-----------------------+
        | MCP73831 chg   |
        +----------------+
```

**Data flow:** Each survey point triggers a sequenced acquisition: (1) HCP sample (20 ms), (2) Wenner AC burst + lock-in (80 ms), (3) eddy pulse + decay capture (40 ms). The MCU computes mV, ρ, and d_cover, fuses the current position from the encoder/IMU, appends a record to the in-RAM ring buffer and the microSD, and pushes it over BLE as a GATT notification. In LPR mode the user holds the sponge still while a 10-minute sweep runs, then the record carries the i_corr / mm-year fields.

---

## 7. Firmware Design

The firmware is a single super-loop with a cooperative scheduler; no RTOS, to keep the flash footprint small and the timing deterministic. Modules live under `firmware/drivers/` and the application logic in `firmware/main.c`.

- **survey.c / survey.h** — survey state machine (IDLE → ARM → SCAN → POINT → LPR → STOP), coordinate fusion of encoder + IMU via a complementary filter, ring buffer of survey points, ASTM C876 classification.
- **ads1220.c** — SPI driver for the 24-bit ADC; PGA and sample-rate configuration; 50/60 Hz notch enable; offset calibration on power-up (input shorted).
- **ad5940.c** — impedance AFE driver; sets 1 kHz sine for Wenner, sweeps DC bias for LPR; reads DFT magnitude/phase over SPI; lock-in via internal DFT engine.
- **eddy.c** — DDS frequency set, coil driver enable, ADC sampling of decay waveform, peak-time extraction via running max, depth calibration curve lookup.
- **ble_uart.c** — GATT custom service (UUID 0f4c...) over the BGM220E UART HCI; NUS-like RX/TX characteristics; framing with COBS + CRC-16.
- **encoder.c** — EXTI-driven quadrature decode (TIM3 encoder mode); position accumulator with backlash compensation.
- **imu.c** — LSM6DSO I²C driver; 104 Hz accel/gyro; dead-reckoning yaw integration; drift zeroing when stationary.
- **oled.c** — SSD1306 SPI framebuffer; live readout of {mV, ρ kΩ·cm, d mm}, battery %, BLE link state.
- **report.c** — SHA-256 hash chain over survey records for tamper-evidence; record signing with device serial + calibration date.

Key design decisions:

- **Single ADC, switched front-end.** The ADS1220 muxes between the HCP buffer output and the synchronous demodulator of the eddy-current channel — only one is active at a time, saving a second ADC and reducing power. The AD5940 handles all AC work.
- **No RTOS.** Survey cadence is ≤ 5 Hz; a 120 MHz Cortex-M4F has ample margin. Avoiding an RTOS means no scheduler jitter in the timing-sensitive eddy peak detector.
- **Power gating.** Each analog block is powered through a PMOS load switch controlled by the MCU; the Wenner AFE and eddy driver draw ~40 mA combined and are only enabled during their acquisition window, keeping average current under 12 mA.
- **Calibration in flash.** Zero-offset (ADS1220), open-circuit eddy t_p, and reference-electrode offset constants are stored in a dedicated flash page, read at boot.

---

## 8. Calibration & Reference Electrodes

1. **HCP zero.** Short the buffer input to the Cu/CuSO₄ electrode body (no rebar connection); the ADS1220 reading is stored as the zero-offset constant (typically ±2 mV).
2. **Wenner contact check.** On a known 100 Ω·cm resistor block (supplied calibration fixture), the firmware verifies ρ within ±5%.
3. **Eddy depth calibration.** Place the coil on the calibration block with reference steel bars at 12, 25, 38, 50 mm; the firmware builds a piecewise-linear t_p→d table.
4. **LPR cell constant.** With the standard counter electrode area (10 cm²), the firmware stores the geometric constant K (≈ 2) used to convert measured R_p to i_corr.

The companion app warns when a calibration is older than 90 days.

---

## 9. Companion Application

A React Native (Expo) app for iOS/Android:

- **Connect screen** — scan for RebarScope devices, show battery, firmware version, calibration date; connect to one.
- **Survey setup** — name the site, set grid resolution (0.2 / 0.5 / 1.0 m), choose modalities (HCP only / HCP+ρ / full / +LPR), pick reference electrode type, enter rebar connection quality.
- **Live survey** — large readout of {mV, ρ, d mm}; auto-snapping to grid; haptic feedback on out-of-range values; a breadcrumb trail of measured points.
- **Heatmap** — three stacked maps (HCP, ρ, cover depth) overlaid on a site sketch; ASTM C876 color scale; pinch-zoom; tap a cell for raw values and time stamp.
- **LPR detail** — potentiodynamic sweep plot, E_corr, R_p, i_corr, mm/year; comparison with HCP verdict.
- **Report** — PDF export with site photo (camera), coordinate of each hotspot, calibration status, device serial, SHA-256 hash; email/share.

The app stores surveys locally in SQLite and syncs to an optional cloud bucket.

---

## 10. Use Cases & Target Audience

- **Bridge deck surveys** — State DOTs and county engineers routinely walk bridge decks after winter de-icing; RebarScope replaces three rented instruments with one owned wand.
- **Parking garage condition assessment** — high chloride exposure structures; corrosion maps drive waterproofing / overlay decisions.
- **Marine structure inspection** — docks, seawalls, piers; resistivity + cover depth identify splash-zone hotspots.
- **Pre-purchase / due-diligence** — forensic engineers quantifying corrosion risk before acquisition of aging buildings.
- **Academic research** — corrosion labs that currently rent a single half-cell cart can instrument many slabs in parallel.
- **Asset-management programs** — owners of large portfolios (universities, transit agencies) running annual condition surveys.
- **Educational** — civil-engineering teaching labs; students learn all four modalities at <$200 of parts instead of a $10k closed box.

---

## 11. Power Budget

| Block | Active current | Duty cycle | Average |
|---|---|---|---|
| STM32L4R5 (run) | 8 mA | 100% | 8 mA |
| OLED | 4 mA | 100% | 4 mA |
| ADS1220 | 0.5 mA | 100% | 0.5 mA |
| AD5940 + Wenner AFE | 38 mA | 5% | 1.9 mA |
| Eddy driver + DDS | 45 mA | 3% | 1.4 mA |
| BGM220E BLE TX | 15 mA | 2% | 0.3 mA |
| MAX17048 + misc | 2 mA | 100% | 2 mA |
| **Total** | | | **≈ 18 mA** |

With a 3.4 Ah 18650 the survey life is ~14 h, comfortably a full day in the field.

---

## 12. Bill of Materials

| Ref | Part | Qty | Cost (USD) |
|---|---|---|---|
| U1 | STM32L4R5ZIT6 | 1 | 9.20 |
| U2 | TI ADS1220IPW | 1 | 5.40 |
| U3 | AD5940BCPZ | 1 | 12.80 |
| U4 | AD8603AUJZ | 1 | 3.10 |
| U5 | BGM220E | 1 | 8.50 |
| U6 | LSM6DSO | 1 | 3.20 |
| U7 | SSD1306 OLED 128×64 | 1 | 2.40 |
| U8 | MAX17048 | 1 | 1.80 |
| U9 | MCP73831 | 1 | 0.90 |
| U10 | AD9833 DDS | 1 | 4.20 |
| U11 | OPA569 | 1 | 6.10 |
| ENC1 | 200 PPR quadrature encoder | 1 | 4.50 |
| REF1 | Cu/CuSO₄ electrode tip | 1 | 7.00 |
| MECH | Wenner probe head + 4 pins | 1 | 6.00 |
| MECH | Eddy coil + ferrite core | 1 | 3.50 |
| PCB | 4-layer 148×74 mm | 1 | 8.00 |
| MISC | passives, connectors, IP54 housing | — | ~12 |
| **Total** | | | **~$101** |

---

## 13. Safety & Limitations

- **Half-cell / LPR require rebar connection.** The user must verify continuity to the rebar network (low-ohm bond); a poor connection produces misleading potentials. The firmware flags contact-resistance > 1 kΩ.
- **Surface moisture matters.** HCP and Wenner need a damp surface; the device prompts the operator to wet the contact patch if the open-circuit contact check fails.
- **Eddy-current limitation.** Cover-depth estimation is reliable for 5–80 mm of cover and rebar ≥ #4; closely-spaced double-mat reinforcement produces ambiguous peaks — the firmware reports depth uncertainty.
- **LPR polarization must be small.** The device limits polarization to ±250 mV and current to 10 µA to avoid perturbing the rebar; longer-duration DC (galvanostatic) modes are deliberately not implemented.
- **Not a substitute for a licensed structural assessment.** RebarScope is a screening tool; flagged hotspots must be confirmed by coring / chloride profiling.

---

## 14. License

- **Hardware** (KiCad schematic, PCB, mechanical): CERN-OHL-S v2 — see `kicad/`.
- **Firmware** (all C source): GPL-3.0 — see `firmware/`.
- **Companion app**: MIT — see `app/`.

All authored by **jayis1**. Copyright © 2026 jayis1. All rights reserved.

---

*RebarScope: open the black box of concrete corrosion inspection.*