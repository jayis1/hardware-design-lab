![HydraScan](https://img.shields.io/badge/Form--Factor-72×44mm%20pocket-blue)
![MCU](https://img.shields.io/badge/MCU-STM32H733-orange)
![Optical](https://img.shields.io/badge/Optical-8--LED%20UV--NIR-green)
![EIS](https://img.shields.io/badge/EIS-AD5940%201Hz–100kHz-purple)
![Wireless](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20USB--C-teal)
![Battery](https://img.shields.io/badge/Battery-2%20days-red)
![Author](https://img.shields.io/badge/Author-jayis1-orange)
![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

# HydraScan — Pocket Liquid Fingerprinting Instrument

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)
**Version:** 1.0.0
**Date:** 2026-08-02

> A pocket-sized, battery-powered instrument that identifies an unknown liquid
> by combining **eight-wavelength optical absorbance** (255–940 nm) with
> **broadband electrochemical impedance spectroscopy** (1 Hz–100 kHz) and runs
> an on-device edge-ML classifier to name the liquid and flag adulteration —
> in seconds, with no consumables, and at a fraction of the cost of a
> laboratory analyser.

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [Why This Is Novel](#2-why-this-is-novel)
3. [How It Works — Dual-Modality Liquid Fingerprinting](#3-how-it-works--dual-modality-liquid-fingerprinting)
4. [Hardware Specifications](#4-hardware-specifications)
5. [System Architecture & Block Diagram](#5-system-architecture--block-diagram)
6. [Firmware Design](#6-firmware-design)
7. [Companion Application](#7-companion-application)
8. [Calibration & Liquid Library](#8-calibration--liquid-library)
9. [Use Cases & Target Audience](#9-use-cases--target-audience)
10. [Directory Structure](#10-directory-structure)
11. [Getting Started](#11-getting-started)

---

## 1. Purpose & Overview

Identifying an unknown liquid is a deceptively hard problem. A glass of
clear liquid could be water, vodka, methanol, diluted milk, or a dangerous
counterfeit spirit — and the human eye cannot tell them apart. Today the
options are polarised: send a sample to a **$40,000+ laboratory spectroscopy
and chromatography bench** (slow, expensive, requires trained staff and
consumables), or rely on **single-strip colourimetric test strips** that test
for one analyte at a time and cannot identify a liquid *as a whole*.

There is no open-hardware, field-deployable instrument that asks the simple
question **"what is this liquid?"** and answers it in seconds by treating the
liquid as a multi-dimensional fingerprint rather than measuring a single
parameter. **HydraScan** fills that gap.

HydraScan is a pocket instrument (72 × 44 × 18 mm, 55 g) with a reusable
stainless-steel interdigitated electrode and a sealed optical cuvette slot.
You place 0.5 mL of liquid into the cuvette, dip the electrode, press the
single button, and within ~4 seconds the device reports on its 128×64 OLED
and over BLE:

- **Identity** — nearest-matched liquid from an onboard library (e.g. *whole
  milk*, *gin (40 % ABV)*, *olive oil*, *tap water*, *isopropanol*).
- **Confidence** — Mahalanobis-distance probability that the match is correct.
- **Adulteration flag** — if the fingerprint lies between two known classes
  (e.g. *milk + added water*), the device reports the interpolated mixture
  ratio and flags **adulteration**.
- **Raw fingerprint** — the full 72-dimensional feature vector (8 optical
  absorbances + 32 complex impedance points + temperature), exportable to the
  app for custom library building.

Two complementary physical modalities make the fingerprint far more
discriminating than either alone:

| Modality | What it sees | Discriminates |
|---|---|---|
| **Multi-wavelength optical absorbance** (8 LEDs, 255–940 nm) | Molecular absorption (aromatics, π-bonds, colour, NIR water/O–H) | Hydrocarbon type, alcohol vs water, dyes, dilution |
| **Electrochemical impedance spectroscopy** (AD5940, 1 Hz–100 kHz) | Ionic conductivity, dielectric permittivity, double-layer capacitance | Ion content, alcohol fraction, dissolved salts, oil vs aqueous |

Optics separates *water* from *alcohol* and *oils* by their distinct
absorption signatures; EIS separates *tap water* from *distilled water* and
*saline* by ionic conductivity. Together they resolve liquids that are
optically identical but electrically distinct (and vice versa).

### Target problems

- **Counterfeit & adulterated spirits** — methanol contamination, watered-down
  whisky. A 2018 WHO study estimated 25 % of alcohol consumed globally is
  illicit; methanol poisoning kills thousands annually.
- **Milk adulteration** — watering, urea addition, synthetic milk. India's
  FSSAI reports over 60 % of milk samples are adulterated in some regions.
- **Fuel adulteration** — kerosene/petrol blending, water in diesel.
- **Pharmaceutical & cosmetic verification** — confirm a clear injectable is
  the labelled product.
- **Honey dilution & origin** — detect glucose/syrup adulteration.
- **Field water screening** — distinguish distilled, tap, river, saline, and
  industrial effluent for environmental inspectors.

---

## 2. Why This Is Novel

No existing open-hardware device combines **broadband EIS** with
**multi-wavelength optical absorbance** in a single, pocket, reusable,
no-consumable instrument for general liquid identification. The closest
related devices in this repository measure a single domain:

- *HydroFluor* is a submersible fluorescence sonde for natural-water analytes
  (CDOM, chlorophyll) — it uses fluorescence, not absorbance + EIS, and does
  not identify arbitrary liquids or detect adulteration.
- *LithoCore* uses EIS but for battery cell health, not liquid identification.
- *AlloyScan*, *SpectraPest* etc. target single analyte classes.

Commercial equivalents either do not exist (no "general liquid ID" pocket
product is on the market) or are benchtop FTIR/Raman spectrometers costing
$15,000–$60,000 and requiring trained operators. HydraScan brings the
**dual-modality fingerprinting** approach — previously confined to research
labs — into a $~90 open bill of materials.

### Technical novelty points

1. **Reusable interdigitated stainless-steel electrode** — no disposable
   screen-printed electrodes; cleaned by a brief polarising pulse between
   measurements. Reduces cost-per-test to zero.
2. **On-device Mahalanobis-distance classifier with mixture interpolation** —
   not only classifies pure liquids but estimates blend ratios along the line
   between two library centroids, directly yielding an adulteration
   percentage.
3. **Auto-PCA dimension reduction** in firmware — 72 raw features are
   whitened and projected to 16 principal components stored in flash, so the
   classifier runs in fixed-point on the Cortex-M7 without floating-point
   matrix overhead per measurement.
4. **Self-calibrating optical path** — a reference photodiode monitors each
   LED's absolute output via a beamsplitter fibre, so LED ageing and
   temperature drift are ratioed out on every shot.

---

## 3. How It Works — Dual-Modality Liquid Fingerprinting

### 3.1 Optical absorbance

Eight LEDs illuminate the sample cuvette (a 5 mm path-length borosilicate
vial) in sequence. A transimpedance-amplified photodiode on the far side
measures transmitted intensity *I*. A second "reference" photodiode taps a
fixed fraction of each LED's output via a 90:10 beamsplitter fibre, giving
the incident intensity *I₀*. The absorbance is:

    A(λ) = −log₁₀( I_sample / I_reference )

Eight absorbances form the optical sub-vector **o** ∈ ℝ⁸.

| LED λ (nm) | Package | What it probes |
|---|---|---|
| 255 | UVC deep-UV SMD | Aromatics, unsaturated bonds, nucleic acids, proteins |
| 280 | UVC | Tryptophan / protein backbone |
| 365 | UV-A | Optical brighteners, certain hydrocarbons, oils |
| 470 | Blue | Dyes, chlorophyll, colour |
| 590 | Amber | Yellow/orange adulterant dyes, caramel (whisky) |
| 660 | Red | Turbidity proxy, general colour |
| 850 | NIR | O–H overtone (water content), sugar |
| 940 | NIR | Second O–H overtone, alcohol/water discrimination |

### 3.2 Electrochemical impedance spectroscopy

A four-wire interdigitated electrode (0.2 mm pitch, 8 mm active length, 316L
stainless steel over glass) is dipped into the same liquid. The **AD5940**
impedance-analyzer SoC applies a low-amplitude (10 mV rms) sinusoidal
excitation across 20 log-spaced frequencies from 1 Hz to 100 kHz and measures
the complex impedance **Z(f) = R + jX** at each. The 40 real+imaginary values
form the electrical sub-vector **e** ∈ ℝ⁴⁰.

The spectrum encodes:

- **Low-frequency real axis** → charge-transfer / double-layer behaviour
  (redox-active species, proteins).
- **Mid-frequency real axis** → bulk ionic conductivity (salts, acids,
  dissolved ions).
- **High-frequency imaginary axis** → dielectric permittivity (alcohol vs
  water have very different ε).

### 3.3 The full fingerprint

The measurement vector is:

    x = [ o₁…o₈ , e₁…e₄₀ , T ] ∈ ℝ⁴⁹

(temperature T normalises conductivity, which has a ~2 %/°C coefficient).
After whitening and PCA projection it becomes **z ∈ ℝ¹⁶**, the input to the
classifier.

### 3.4 Classification & adulteration

The onboard library stores, for each known liquid *k*, a centroid **μₖ** ∈
ℝ¹⁶ and a diagonal covariance **Σₖ** (learned from calibration shots). For a
new measurement **z**:

1. Compute Mahalanobis distance  *Dₖ² = (z − μₖ)ᵀ Σₖ⁻¹ (z − μₖ)*  to every
   class.
2. Convert distances to a softmax probability; the top class is the reported
   identity, with its probability as confidence.
3. If the nearest two classes are a known *adulterant pair* (e.g. *whole milk*
   and *deionised water*), fit **z** to the line segment between their
   centroids: the fractional position gives the **adulteration ratio**, and a
   flag is raised if it exceeds a stored threshold.

---

## 4. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H733VG, Cortex-M7 @ 480 MHz, 1 MB flash, 564 KB RAM |
| **Impedance SoC** | AD5940BCCZ (16-bit, 1 Hz–200 kHz, 4-wire) |
| **Optical source** | 8× LED: 255 / 280 / 365 / 470 / 590 / 660 / 850 / 940 nm |
| **Photodetectors** | 2× OPT101 (sample + reference), transimpedance 1 MΩ |
| **Temperature** | TMP117 (±0.1 °C, 16-bit) |
| **Display** | 0.96" 128×64 monochrome OLED (SSD1306, SPI) |
| **Connectivity** | BLE 5.2 (ANNA-B112 module), USB-C 2.0 (CDC + MSC) |
| **Battery** | 600 mAh Li-Po, 2-day typical use, USB-C charging (BQ25895) |
| **Power budget** | ~18 mA active measure, <40 µA sleep |
| **Electrode** | 316L stainless interdigitated, 0.2 mm pitch, reusable |
| **Cuvette** | 5 mm path borosilicate glass vial, 0.5 mL sample |
| **Form factor** | 72 × 44 × 18 mm, 55 g |
| **Enclosure** | IP65 glass-filled nylon, hinged cuvette door |
| **Buttons** | Single capacitive sense button + power slide |
| **Clock accuracy** | HSE 16 MHz ±10 ppm; onboard RTC for timestamps |
| **Storage** | 8 MB QSPI flash (library + logs), FATFS over USB MSC |
| **BOM cost (1k)** | ~$92

### Peripherals & pin map (summary)

- AD5940: SPI2 (MOSI/PB15, MISO/PB14, SCK/PB13, CS/PB12), IRQ/PE3, RST/PE4,
  GPIO1/PE5.
- OLED SSD1306: SPI3 (MOSI/PC12, SCK/PC10, DC/PB4, CS/PB5, RST/PB6).
- LED mux: 8 high-side loads via 74HC595 (SPI1 MOSI/PA7, SCK/PA5, LATCH/PA4)
  driving 8 constant-current sinks (AL8805) on a common anode rail.
- Photodiode ADC: ADC1 channels 3 (sample, PA3) and 4 (reference, PA4? — see
  board.h for exact; ADC is dedicated).
- TMP117: I2C1 (SCL/PB8, SDA/PB9), address 0x48.
- BLE ANNA-B112: UART4 (TX/PA0, RX/PA1, CTS/PA2, RTS/PA3, EN/PB0).
- USB-C: USB OTG1 (DM/PA11, DP/PA12, VBUS sensing PA9).
- Charger BQ25895: I2C1 (shared), INT/PC13, CE/PB1.
- QSPI flash: W25Q64 (BK1, IO0/PD11…IO3/PD14, CS/PB10, SCK/PB2).
- Button: capacitive touch on PC0 (TSC).

---

## 5. System Architecture & Block Diagram

```
                  ┌─────────────────────────────────────────────┐
                  │                STM32H733VG                   │
                  │  Cortex-M7 @480MHz │ FPU │ Cache │ RTC       │
                  │  ┌──────────────────────────────────────┐    │
                  │  │  HydraScan FW  (firmware/)            │    │
                  │  │  ├─ main  (orchestrator)             │    │
                  │  │  ├─ optical (LED sweep + ADC)        │    │
                  │  │  ├─ eis    (AD5940 driver + sweep)    │    │
                  │  │  ├─ thermal (TMP117)                 │    │
                  │  │  ├─ classifier (PCA + Mahalanobis)    │    │
                  │  │  ├─ library (flash liquid templates)   │    │
                  │  │  ├─ ble    (GATT server)              │    │
                  │  │  └─ ui     (OLED + button)            │    │
                  │  └──────────────────────────────────────┘    │
                  └──┬────┬────┬────┬────┬────┬────┬────┬────────┘
            SPI2 ────┤    │    │    │    │    │    │    │
            SPI3 ────┼────┘    │    │    │    │    │    │
            SPI1 ────┼─────────┘    │    │    │    │    │
            I2C1 ────┼──────────────┘    │    │    │    │
            ADC1 ────┼───────────────────┘    │    │    │
            UART4────┼────────────────────────┘    │    │
            USB ─────┼─────────────────────────────┘    │
            QSPI─────┼──────────────────────────────────┘    │
                     │
        ┌────────────┴──────────────────────────────────────────┐
        │      │       │       │        │          │            │
   ┌────▼───┐ ┌─▼──┐ ┌─▼───┐ ┌─▼──┐ ┌───▼──┐ ┌─────▼───┐ ┌─────▼───┐
   │ AD5940 │ │LED │ │OLED │ │TMP │ │ BLE  │ │ BQ25895 │ │ W25Q64  │
   │  EIS   │ │mux │ │ SSD │ │117 │ │ANNA-B│ │ charger │ │  QSPI   │
   │  SoC   │ │74H5│ │1306 │ │    │ │ 112  │ │  + USB  │ │  flash  │
   └───┬────┘ └─┬──┘ └─────┘ └────┘ └──────┘ └────┬────┘ └─────────┘
       │        │                              USB-C│
   ┌───▼────┐ ┌─▼──────────────┐               ┌────▼────┐
   │4-wire  │ │ 8× LEDs 255…  │               │ Li-Po   │
   │ inter- │ │ 940 nm +       │               │ 600 mAh │
   │ digit  │ │ beamsplitter   │               └─────────┘
   │ elec.  │ │ ref photodiode │
   └────────┘ └─────────┬──────┘
                           │
                       ┌───▼────┐
                       │ cuvette│  0.5 mL liquid sample
                       │+ PD    │  (5 mm path)
                       └────────┘
```

### Measurement flow

```
  button ──► main_run_measurement()
              │
              ├─► thermal_read()          → T
              ├─► optical_sweep()         → o[8]   (8 LEDs, ref/samp ratio)
              ├─► eis_sweep()             → Z[20]  (complex, 1 Hz–100 kHz)
              ├─► feature_build()        → x[49]
              ├─► pca_project()           → z[16]
              ├─► classifier_classify()   → {id, conf, adulterant?, ratio}
              ├─► ui_show_result()
              └─► ble_notify_result()   + log to QSPI FATFS
```

---

## 6. Firmware Design

The firmware is written in portable C targeting the STM32H733 with the
CMSIS core headers (no heavyweight HAL dependency; register-level drivers).
Source lives in [`firmware/`](firmware/).

### Key design decisions

- **Register-level drivers over HAL.** `registers.h` maps the few peripherals
  actually used; this keeps the build hermetic and the code legible for
  teaching/inspection, and avoids pulling in 50k lines of HAL for a device
  that touches ~8 peripherals.
- **No dynamic allocation.** All buffers are static; the classifier and sweep
  engines run on fixed-size stacks. Predictable, no heap fragmentation on a
  device that runs for years between resets.
- **Fixed-point DSP.** PCA projection and Mahalanobis distance use Q16.16
  arithmetic; the Cortex-M7 FPU is reserved for the optical log and the EIS
  complex maths only.
- **Single-button UX.** One capacitive button does measure (short press),
  power-off (long press) and scroll result history (double press). All
  configuration (library management, calibration, thresholds) happens in the
  app over BLE; the device itself has no menus.
- **AD5940 commanded via SPI2** with a small command queue. Each frequency
  point is configured, the AD5940's onboard DFT returns real+imag, and the
  driver averages N=4 sweeps per point for noise.
- **Optical sweep** uses a 74HC595 shift register to enable one LED at a time
  on a common-anode rail; an integrate-and-hold ADC reads sample and reference
  photodiodes, and the firmware computes −log10 of their ratio. Each LED is
  pulsed for 200 µs to limit photobleaching and power.
- **Library in QSPI flash** as a flat array of `liquid_class_t` records; the
  app can add/retrain classes over BLE by writing a new image into a spare
  flash sector and atomically swapping the active pointer.

### Files (see `firmware/`)

| File | Role | Lines |
|---|---|---|
| `main.c` | Boot, init, button FSM, measurement orchestration, logging | ~210 |
| `board.h` | Pin map, peripheral instances, hardware constants | ~90 |
| `registers.h` | STM32H7 register base addresses and bit defs used | ~120 |
| `drivers/optical.c/.h` | LED mux + ADC1 photodiode acquisition + absorbance | ~150 |
| `drivers/eis.c/.h` | AD5940 SPI driver + frequency sweep + averaging | ~170 |
| `drivers/thermal.c/.h` | TMP117 I2C driver | ~70 |
| `drivers/display.c/.h` | SSD1306 SPI driver + result rendering | ~120 |
| `drivers/ble.c/.h` | BLE GATT server (custom service + Nordic UART) | ~140 |
| `drivers/flash_lib.c/.h` | QSPI liquid-library store | ~90 |
| `classifier.c/.h` | PCA project + Mahalanobis + mixture interpolation | ~180 |
| `Makefile` | arm-none-eabi-gcc build | ~60 |

Total well over 1,400 lines of C.

### Build

```
cd firmware
make              # builds hydrascan.elf + .hex + .bin
make flash        # openocd via ST-Link
```

---

## 7. Companion Application

A **React Native (Expo)** app, `app/`, provides:

- **Scan screen** — live BLE scan, "Measuring…" progress, and the result card
  (identity, confidence, adulteration flag/ratio, temperature, timestamp).
- **Fingerprint screen** — bar chart of the 8 optical absorbances and the
  Nyquist plot (real vs −imag) of the 20-point impedance spectrum, plus the
  PCA scatter with the nearest class centroid highlighted.
- **Library screen** — list of onboard liquid classes; tap to view centroid,
  covariance, calibration sample count; "Add class" walks through capturing
  N calibration shots and uploading a new class to flash.
- **History screen** — list of past scans pulled from QSPI flash over BLE,
  with CSV export / share.
- **Settings screen** — adulteration thresholds per class, BLE pairing,
  firmware OTA (over Nordic UART DFU).

See [`app/README.md`](app/README.md) for build/run instructions.

---

## 8. Calibration & Liquid Library

Each liquid class is calibrated by taking **≥12 shots** of a known reference
liquid across the device's expected temperature range (5–40 °C). The app
collects these, computes the PCA-projected centroid and diagonal covariance,
and writes the class into QSPI flash. The shipped library ships with ~24
common classes:

water (distilled, tap, mineral), whole milk, skim milk, milk+water blends,
ethanol 40 % (vodka/gin), methanol, isopropanol, white wine, red wine, whisky,
olive oil, sunflower oil, kerosene, petrol, diesel, honey (pure), honey+glucose,
0.9 % saline, 5 % glucose IV, vinegar, cola, black coffee, and an
"unknown/other" rejection class.

The mixture-interpolation engine specifically recognises these adulterant
pairs: milk↔water, whisky↔water, honey↔glucose-syrup, petrol↔kerosene,
distilled↔saline.

---

## 9. Use Cases & Target Audience

| Audience | Use case |
|---|---|
| Food safety inspectors | Screen milk, honey, oils for adulteration at point of sale |
| Customs / border | Identify undeclared liquids in unlabelled bottles |
| Public health / WHO field teams | Detect methanol in counterfeit spirits |
| Fuel quality inspectors | Petrol/kerosene/diesel adulteration, water-in-diesel |
| Pharmaceutical QA | Confirm clear injectables match label |
| Environmental inspectors | Distinguish distilled/tap/river/saline effluent |
| Brewers / distillers | QC of ABV and water source |
| Citizen scientists / educators | Open liquid-analysis lab tool |
| Home cooks / foragers | Identify unknown liquids, suspect oils |

---

## 10. Directory Structure

```
hydrascan/
├── README.md                  ← this file
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   └── drivers/
│       ├── optical.c / optical.h
│       ├── eis.c / eis.h
│       ├── thermal.c / thermal.h
│       ├── display.c / display.h
│       ├── ble.c / ble.h
│       └── flash_lib.c / flash_lib.h
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
└── app/
    ├── README.md
    ├── app.json
    ├── package.json
    └── src/
        ├── App.tsx
        ├── ble.ts
        ├── screens/
        │   ├── ScanScreen.tsx
        │   ├── FingerprintScreen.tsx
        │   ├── LibraryScreen.tsx
        │   ├── HistoryScreen.tsx
        │   └── SettingsScreen.tsx
        └── components/
            └── ResultCard.tsx
```

---

## 11. Getting Started

1. **Fabricate the PCB** from `kicad/` (4-layer, 72×44 mm). Populate per BOM.
2. **Flash firmware** — `cd firmware && make && make flash` (ST-Link V3).
3. **Install the app** — `cd app && yarn && yarn start` (Expo, scan QR).
4. **Pair** the device over BLE in the app's Settings screen.
5. **Load a liquid library** (the default library is pre-flashed; add custom
   classes via Library → Add Class).
6. **Measure** — fill the cuvette with 0.5 mL, dip the electrode, close the
   door, short-press the button. Result appears in ~4 s on OLED and app.

### Cleaning the electrode

Between immiscible liquids (oil → water), wipe the electrode with an
alcohol-soaked swab. The firmware runs a 2-second polarising "clean" pulse on
power-on to desorb residues; no consumables are required.

---

*HydraScan — know your liquid. Designed by jayis1.*