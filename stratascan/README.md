# StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar Imager

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

![Device](https://img.shields.io/badge/PCB-180×120mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H743-orange) ![RFIC](https://img.shields.io/badge/RF-1MHz–3GHz%20SFCW-green) ![Depth](https://img.shields.io/badge/Depth-0–30m-violet) ![BLE](https://img.shields.io/badge/BLE-5.2-yellow)

---

## 1. Purpose & Overview

**StrataScan** is a fully open-hardware, open-firmware **Stepped-Frequency
Continuous-Wave (SFCW) Ground Penetrating Radar (GPR)** designed for affordable,
high-resolution subsurface imaging. It fills the enormous gap between
commercially available GPR systems (which cost $15,000–$80,000+ and are locked
behind proprietary software) and the complete absence of any open-source GPR
with real imaging capability.

Where existing open-source efforts produce only raw A-scan amplitude
envelopes with no migration, no depth calibration, and no synthetic-aperture
processing, StrataScan delivers a complete imaging pipeline: it transmits a
stepped-frequency chirp from 1 MHz to 3 GHz across 1024 frequency steps,
captures coherent I/Q reflections from a single bowtie antenna pair, performs
on-device range-domain FFT migration, and streams a fully-formed B-scan
(radargram) cross-section to a companion mobile app over BLE 5.2. The app
renders the subsurface image in real time with depth-axis calibration,
auto-gain, background subtraction, and interactive layer annotation.

### Why SFCW over pulsed/impulse GPR?

Traditional impulse GPR transmits a nanosecond-duration pulse and samples the
return with an equivalent-time sampling front-end. This requires exotic
broadband antennas, fast switches, and sub-nanosecond jitter timing circuits
that are difficult to reproduce and calibrate in an open design. The
stepped-frequency approach instead transmits a series of narrowband CW tones
sweeping across the band, measuring the amplitude and phase of each reflected
tone. The time-domain range profile is then recovered via an inverse FFT of the
frequency-domain data. Benefits:

1. **Lower instantaneous bandwidth** — each tone is narrowband, so standard RF
   components (mixers, filters, amplifiers) suffice. No exotic pulse
   generation or picosecond sampling is needed.
2. **Superior dynamic range** — narrowband detection at each step allows a
   narrow IF filter, improving sensitivity and signal-to-noise ratio.
3. **Flexible band selection** — the frequency sweep range, step count, and
   dwell time are software-programmable, enabling the same hardware to operate
   as a shallow high-resolution scanner (1–6 GHz for concrete/rebar) or a deep
   geological sounder (100 kHz–500 MHz for groundwater/strata).
4. **Frequency-domain calibration** — system transfer function
   characterization (cable delay, antenna mismatch, amplifier gain ripple) is
   straightforward in the frequency domain, enabling built-in self-calibration.
5. **Regulatory simplicity** — narrowband emissions are easier to certify under
   Part 15 / ETSI radiated emission limits than ultra-wideband impulse
   transmissions.

### Target subsurface targets

| Application Domain         | Typical Targets                                   | Depth Range  | Band Preset            |
|----------------------------|---------------------------------------------------|-------------|------------------------|
| Utility locating           | Pipes, conduits, cables, vaults                  | 0.1–3 m     | 500 MHz–3 GHz (HI)    |
| Concrete NDT               | Rebar spacing, cover depth, delaminations, voids  | 0.02–0.5 m  | 1–3 GHz (UHI)         |
| Archaeology                | Walls, foundations, graves, features              | 0.2–4 m     | 200 MHz–1.5 GHz (MED) |
| Geotechnical / civil       | Bedrock depth, soil layers, water table          | 1–15 m      | 50–800 MHz (LO)       |
| Glaciology / environmental | Ice thickness, peat depth, permafrost             | 2–30 m      | 10–250 MHz (DEEP)     |
| Forensics                  | Clandestine graves, buried objects               | 0.2–2 m     | 200 MHz–1.5 GHz (MED) |

---

## 2. Novelty — What Does Not Exist Today

No open-source hardware project currently provides all of the following in one
device:

1. **Full SFCW architecture with PLL-generated tones** — Most open radar work
   uses impulse/UWB with sampling-oscilloscope front ends that cannot resolve
   phase-coherent reflections across a wide band. StrataScan uses an Analog
   Devices ADF4159 fractional-N PLL with a wideband VCO (HMC5841, 1–3 GHz
   fundamental, with harmonic extension down to ~1 MHz via divided outputs) to
   synthesize each frequency step with sub-Hz resolution and coherent phase
   continuity.

2. **Homodyne coherent I/Q detection with 16-bit ADC** — The received signal is
   mixed with a sample of the transmitted tone (homodyne / zero-IF) to produce
   baseband I and Q, digitized by a dual-channel ADS131M08 (24-bit delta-sigma
   ADC) at 8 kSPS per step. Phase coherence is maintained across the entire
   sweep via a shared 10 MHz reference.

3. **On-device range migration** — The STM32H743 (480 MHz Cortex-M7) computes a
   1024-point complex inverse FFT of the I/Q trace to produce a range-domain
   A-scan, then stacks successive A-scans into a B-scan matrix and performs
   sinc-interpolation synthetic-aperture focusing (SAF) for along-track
   resolution improvement as the user drags the unit along the surface.

4. **Programmable band presets** — A single hardware platform supports five
   user-selectable frequency presets (DEEP / LO / MED / HI / UHI) via firmware,
   adjusting sweep range, step count, and dwell time. Antenna matching is handled
   by switchable matching networks and selectable bowtie / planar monopole /
   loaded dipole elements on a quick-change antenna sled.

5. **Open companion app with live radargram rendering** — A React Native app
   renders the B-scan radargram in real time with pinch-zoom, color-map
   selection, distance measurement cursors, depth-calibrated vertical axis
   (using measured or user-entered relative permittivity), layer auto-labeling,
   and export of GPR-compatible HDF5/SEG-Y files.

---

## 3. Hardware Specifications

| Parameter                 | Value                                                                 |
|---------------------------|-----------------------------------------------------------------------|
| **MCU**                   | STM32H743VIT6, ARM Cortex-M7, 480 MHz, 2 MB Flash, 1 MB SRAM          |
| **RF PLL / Synthesizer**  | Analog Devices ADF4159, fractional-N, 13.4 GHz max, sub-Hz resolution |
| **VCO**                   | HMC5841LP5 (1–3 GHz fundamental) + HMC585 (500 MHz–1 GHz) + divide chain for 1 MHz–500 MHz coverage |
| **Receiver Front End**    | HMC624 LPF + HMC788 LNA (NF 1.8 dB, 21 dB gain) + HMC595 I/Q demodulator |
| **ADC**                   | TI ADS131M08, 8-channel simultaneous-sampling 24-bit delta-sigma, 8 kSPS |
| **Antennas**              | Quick-change sled: bowtie (HI/UHI), planar dipole (MED), loaded dipole (LO/DEEP) |
| **Frequency Range**       | 1 MHz – 3 GHz (5 programmable presets)                               |
| **Frequency Steps**       | 128 – 1024 per sweep, configurable                                    |
| **Dwell Time per Step**   | 125 µs – 8 ms, configurable (default 1 ms)                           |
| **Sweep Time**            | 130 ms (fast) – 8.2 s (deep), configurable                           |
| **Depth Resolution**      | ~5 cm (air) to ~1 m (deep soil), depends on ε_r and band              |
| **Max Penetration**       | ~30 m (DEEP band in low-loss medium)                                 |
| **Dynamic Range**          | >110 dB (coherent integration, 1024 steps)                           |
| **IMU**                   | BMI270 (accel + gyro) for along-track position dead-reckoning       |
| **Wheel Encoder**         | 1024-PPR quadrature encoder on survey wheel for precise trace spacing|
| **GNSS**                  | u-blox SAM-M10Q (multi-constellation, 10 Hz, 2 m CEP)                |
| **Display**               | 2.42" OLED, SSD1309, 128×64, for status and preview A-scan           |
| **Storage**               | MicroSD card (FAT32, raw + processed B-scans logged as SEG-Y)       |
| **Connectivity**          | BLE 5.2 (nRF52840), USB-C (CDC + Mass Storage), RS-485 (multi-unit sync) |
| **Power**                 | 7.4 V 18650×2 battery pack, 8 h continuous survey; USB-C PD passthrough |
| **Form Factor**           | 180 × 120 × 55 mm control unit + detachable antenna sled; IP54       |
| **Weight**                | 520 g (control unit + battery), 240 g (antenna sled)                 |
| **Operating Temp**        | −20 °C to +55 °C                                                      |

---

## 4. Architecture & Block Diagram

```
 ┌─────────────────────────────────────────────────────────────────────┐
 │                        STM32H743VIT6 (Host)                          │
 │  ┌──────────┐  ┌──────────────┐  ┌───────────┐  ┌───────────────┐   │
 │  │ Sweep    │  │ I/Q Capture │  │ Range FFT │  │ SAF Migration │   │
 │  │ Manager  │→ │ (DMA/EXTI)  │→ │ (CMSIS-DSP│→ │ (Sinc Interp) │   │
 │  └────┬─────┘  └──────┬──────┘  └─────┬─────┘  └───────┬───────┘   │
 │       │               │                │                 │          │
 │       │ SPI1         │ SPI3           │ shared          │ BLE/SD    │
 └───────┼───────────────┼────────────────┼─────────────────┼──────────┘
         │               │                │                 │
    ┌────▼────┐    ┌─────▼─────┐    ┌──────▼──────┐    ┌─────▼─────┐
    │ADF4159  │    │ADS131M08  │    │  BMI270    │    │ nRF52840  │
    │PLL Ctrl │    │24-bit ADC │    │  IMU (I2C) │    │ BLE 5.2  │
    └────┬────┘    └─────▲─────┘    └────────────┘    └───────────┘
         │               │
    ┌────▼────┐    ┌──────┴──────┐
    │HMC5841  │    │HMC595 I/Q   │
    │VCO 1-3G │    │Demodulator  │
    └────┬────┘    └──────▲──────┘
         │  RF OUT          │ I/Q baseband
    ┌────▼──────────────────▼────┐
    │   TX/RX Antenna Sled        │
    │  ┌─────────┐    ┌─────────┐ │
    │  │ Bowtie  │    │ Bowtie  │ │
    │  │ TX Ant.  │    │ RX Ant.  │ │
    │  └────┬────┘    └────┬────┘ │
    │       │  ground      │      │
    │  ─────┴──────────────┴─────│
    │       Subsurface Targets   │
    └────────────────────────────┘

    Survey Wheel Encoder (TIM3)  ─── trace spacing
    SAM-M10Q GNSS (USART1)     ─── position tagging
    MicroSD (SDMMC1)           ─── SEG-Y logging
    SSD1309 OLED (I2C1)        ─── status display
    USB-C (OTG_FS)             ─── config + data download
    RS-485 (USART2)            ─── multi-unit array sync
```

### Signal flow

1. The **Sweep Manager** programs the ADF4159 PLL to frequency *f_k* (k = 1…N).
2. The VCO output is split: one path drives the **TX bowtie**; another is routed
   as the LO to the I/Q demodulator.
3. The reflected wave is captured by the **RX bowtie**, amplified by the LNA,
   and mixed with the LO in the HMC595 I/Q demodulator, producing baseband
   **I** and **Q**.
4. The ADS131M08 simultaneously samples I and Q at the end of each dwell period.
5. The 2N complex frequency-domain samples {I_k + j·Q_k} are inverse-FFT'd to
   produce the range-domain A-scan.
6. As the user drags the antenna sled along the surface, the survey wheel
   triggers each A-scan capture at fixed distance intervals (e.g., 2 cm).
7. Successive A-scans are assembled into a **B-scan** matrix and processed with
   synthetic-aperture focusing migration for along-track resolution improvement.
8. The processed B-scan is streamed to the app over BLE and logged to SD card
   in SEG-Y format.

---

## 5. Firmware Design

### Design decisions

- **Cooperative super-loop + DMA interrupt**: The main loop is a state machine
  (BOOT → IDLE → SURVEY → PAUSE → CALIBRATE → SHUTDOWN). Time-critical ADC
  capture is driven by DMA with a transfer-complete interrupt that feeds a
  lock-free ring buffer processed by the FFT pipeline in the main loop.
- **CMSIS-DSP for all transforms**: The 1024-point complex IFFT is executed
  using the CMSIS-DSP `arm_cfft_f32` / `arm_cfft_radix2_f32` primitives on the
  Cortex-M7 with hardware FPU for sub-1 ms transform time.
- **Configurable sweep profiles**: Five band presets are defined as structs
  containing start frequency, stop frequency, step count, dwell time, and
  antenna selection; the user switches via the app or OLED menu.
- **Self-calibration**: A shorted-load calibration is performed at boot (and on
  demand): the unit records a sweep with TX power disabled to characterize the
  system noise floor and DC offsets, then subtracts these from subsequent
  measurements in the frequency domain before the IFFT.
- **Survey-wheel driven trace acquisition**: The TIM3 quadrature decoder
  triggers an EXTI line at every N pulses, initiating a full sweep. This makes
  trace spacing independent of walking speed. IMU and GNSS data are tagged to
  each trace for position recovery.
- **Power management**: Between sweeps the PLL and LNA are placed in
  power-down; the MCU enters WFI during idle. The 480 MHz core clock is
  reduced to 240 MHz during deep-survey mode to extend battery life.

### Firmware modules

| File              | Purpose                                                | Lines |
|-------------------|--------------------------------------------------------|-------|
| `main.c`          | State machine, sweep orchestration, B-scan assembly   | 720   |
| `board.h`         | Pin map, clock config, physical constants              | 320   |
| `registers.h`     | STM32H7 peripheral register definitions & bit masks    | 510   |
| `drivers/pll.c`   | ADF4159 PLL programming, frequency sweep, VCO select   | 340   |
| `drivers/receiver.c` | ADS131M08 ADC driver, DMA capture, I/Q buffering    | 310   |
| `drivers/radar.c` | Range FFT, calibration, synthetic-aperture migration   | 430   |
| `drivers/imu.c`   | BMI270 IMU driver for dead-reckoning position          | 200   |
| `drivers/wheel.c` | Survey wheel quadrature encoder + trace trigger        | 180   |
| `drivers/gnss.c`  | SAM-M10Q NMEA parser, position tagging                 | 230   |
| `drivers/sdlog.c` | SD card SEG-Y logging + FAT32                           | 260   |
| `drivers/ble.c`   | nRF52840 BLE protocol, B-scan streaming                | 350   |
| `drivers/display.c` | SSD1309 OLED driver, status + A-scan preview        | 220   |

### Range-domain recovery math

The measured complex frequency-domain vector is:

  S(f_k) = |a(f_k)|·exp(j·φ(f_k)),  k = 0 … N−1

The range-domain reflectivity profile (A-scan) is the inverse DFT:

  s(r_n) = (1/N) Σ_{k=0}^{N−1} S(f_k)·exp(+j·2π·f_k·τ_n)

where τ_n = n / Δf and Δf = (f_stop − f_start) / (N − 1). The unambiguous
range window is R_max = c·(N−1) / (2·Δf·√ε_r), and the range resolution is
δR = c / (2·B·√ε_r) where B = f_stop − f_start.

Synthetic-aperture focusing (SAF) integrates traces across the synthetic
aperture to sharpen along-track resolution: for each subsurface pixel (x, z),
the algorithm coherently sums all traces whose TX-RX midpoint is within one
Fresnel zone of the pixel, applying phase corrections for the two-way path
length sqrt((x−x_tr)² + z²).

---

## 6. Companion Application

The React Native companion app (in `app/`) provides:

- **Live Radargram Screen** — real-time scrolling B-scan with selectable color
  maps (greyscale, jet, seismic), pinch-zoom, and depth-calibrated vertical
  axis using user-entered or auto-estimated relative permittivity ε_r.
- **A-Scan Inspector** — tap any column to view the raw A-scan waveform with
  range markers; auto-detect peak reflections.
- **Survey Screen** — start/stop/pause survey, set trace spacing, select band
  preset, monitor battery/GNSS/wheel status, and view live position on a map
  with trace trajectory overlay.
- **Calibration Screen** — guided calibration wizard: shorted-load, known-depth
  target, ε_r estimation from a measured reflection.
- **Export** — SEG-Y / HDF5 export of raw and processed B-scans; CSV export of
  detected layer depths and positions.
- **Settings** — sweep parameters, color map, gain, background subtraction mode,
  firmware version check, BLE connection management.

### App structure

| File                          | Purpose                              |
|-------------------------------|--------------------------------------|
| `App.js`                      | Navigation + BLE context provider     |
| `screens/SurveyScreen.js`     | Survey control + status              |
| `screens/RadargramScreen.js`   | Live B-scan rendering                |
| `screens/AscanScreen.js`       | A-scan waveform inspector            |
| `screens/CalibrationScreen.js` | Calibration wizard                   |
| `screens/SettingsScreen.js`   | App + device settings                |
| `utils/BleContext.js`         | BLE manager + protocol decoder       |
| `utils/protocol.js`           | Binary protocol definition           |
| `utils/gprMath.js`            | ε_r estimation, depth calc helpers   |
| `components/RadargramCanvas.js`| High-performance canvas renderer     |
| `components/StatusBar.js`      | Battery/GNSS/wheel status bar        |
| `components/AscanPlot.js`      | A-scan waveform plot                 |

---

## 7. Use Cases & Target Audience

### Primary users

1. **Utility locating contractors** — finding buried pipes, conduits, and
   vaults before excavation. Current commercial GPR rentals cost
   $200–$500/day; StrataScan aims for a BOM under $600.
2. **Civil & geotechnical engineers** — concrete cover/spacing inspection,
   bedrock profiling, pavement layer thickness, void detection under slabs.
3. **Archaeologists** — non-invasive prospection for walls, foundations, and
   burial features without excavation.
4. **Forensic investigators** — locating clandestine graves and buried
   objects.
5. **Environmental scientists & glaciologists** — peat depth, permafrost
   active layer, ice thickness, water table mapping.
6. **Educators & students** — the first affordable GPR suitable for teaching
   RF subsurface imaging, signal processing, and field survey methodology.
7. **DIY / maker community** — hobbyist prospecting, amateur archaeology
   (within legal limits), and hardware experimentation.

### Survey workflow

1. Attach the appropriate antenna sled for the target depth.
2. Power on; the unit self-calibrates (30 s), acquires GNSS fix.
3. In the app, select a band preset and enter (or estimate) soil ε_r.
4. Place the sled on the surface, press Start, and drag along the survey line.
5. The live radargram renders in the app; depth-calibrated reflections appear.
6. Pause to inspect; annotate suspected layers/objects with cursors.
7. Stop; the full B-scan + GNSS track is logged to SD card (SEG-Y) and synced to
   the app for post-processing export.

---

## 8. Bill of Materials (Summary)

| Ref  | Part             | Function                     | Qty | Est. Cost |
|------|------------------|------------------------------|-----|-----------|
| U1   | STM32H743VIT6    | Host MCU                     | 1   | $22       |
| U2   | ADF4159BCPZ      | Fractional-N PLL             | 1   | $14       |
| U3   | HMC5841LP5       | VCO 1–3 GHz                  | 1   | $48       |
| U4   | HMC585LC4       | VCO 0.5–1 GHz                | 1   | $36       |
| U5   | HMC788LP3       | LNA                          | 1   | $12       |
| U6   | HMC595LP4       | I/Q Demodulator              | 1   | $28       |
| U7   | ADS131M08IPBSR  | 24-bit ADC                   | 1   | $18       |
| U8   | nRF52840 QFAA   | BLE module                   | 1   | $9        |
| U9   | BMI270          | IMU                          | 1   | $5        |
| U10  | SAM-M10Q        | GNSS                         | 1   | $25       |
| U11  | SSD1309 OLED    | Display                      | 1   | $8        |
| Misc | Passives, connectors, antenna PCBs, enclosure | — | — | ~$85 |
| **Total BOM** |           |                              |     | **~$310** |

---

## 9. Directory Structure

```
stratascan/
├── README.md                     — This document
├── firmware/
│   ├── Makefile                  — arm-none-eabi-gcc build
│   ├── linker.ld                 — STM32H743 linker script
│   ├── board.h                   — Pin map, clock, constants
│   ├── registers.h               — STM32H7 register definitions
│   ├── main.c                    — State machine, sweep orchestration
│   └── drivers/
│       ├── pll.c / pll.h         — ADF4159 PLL driver
│       ├── receiver.c / .h       — ADS131M08 ADC + DMA capture
│       ├── radar.c / .h          — Range FFT + SAF migration
│       ├── imu.c / .h            — BMI270 IMU driver
│       ├── wheel.c / .h          — Survey wheel encoder
│       ├── gnss.c / .h           — SAM-M10Q GNSS parser
│       ├── sdlog.c / .h          — SD card SEG-Y logging
│       ├── ble.c / .h            — nRF52840 BLE protocol
│       └── display.c / .h        — SSD1309 OLED
├── kicad/
│   ├── device.kicad_sch          — Schematic
│   ├── device.kicad_pcb          — PCB layout
│   └── device.kicad_pro          — Project file
└── app/
    ├── App.js                    — React Native entry
    ├── package.json              — Dependencies
    ├── screens/                 — 5 app screens
    ├── utils/                   — BLE, protocol, math helpers
    └── components/               — Canvas + plot components
```

---

## 10. Licensing & Attribution

- **Hardware (schematics, PCB, mechanical):** CERN-OHL-S v2
- **Firmware (C source):** GPL-2.0
- **Companion app (JS/React Native):** MIT

All designs, firmware, code, and documentation authored by **jayis1**.
Copyright © 2026 jayis1. All rights reserved.

StrataScan is the first open-source ground penetrating radar with a complete
SFCW architecture, on-device migration, and a real-time companion imaging app —
bringing subsurface radar imaging within reach of every engineer, researcher,
and educator.