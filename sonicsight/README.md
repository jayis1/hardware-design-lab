# SonicSight — Portable Acoustic Tomography Scanner for Wood & Composite NDT

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

![PCB](https://img.shields.io/badge/PCB-160%C3%97120mm-blue)
![MCU](https://img.shields.io/badge/MCU-STM32H743-orange)
![ADC](https://img.shields.io/badge/ADC-ADS5281%20(8ch%2C%2050MSPS)-green)
![Sensors](https://img.shields.io/badge/Transducers-32ch%20PZT-brightgreen)
![License](https://img.shields.io/badge/License-MIT-yellow)

> **A portable, open-hardware acoustic tomographic imager for non-destructive evaluation (NDE) of standing trees, structural timber, laminated composites, and masonry. SonicSight drives an array of up to 32 piezoelectric transducers, measures ultrasonic time-of-flight across hundreds of ray paths, and reconstructs a 2D cross-sectional velocity/slowness image revealing internal decay, voids, cracks, delaminations, and material anomalies — all in a battery-powered field instrument with a React Native companion app for real-time visualization.**

---

## Table of Contents

1. [Overview & Purpose](#1-overview--purpose)
2. [How It Works](#2-how-it-works)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Firmware Design](#5-firmware-design)
6. [Companion Application](#6-companion-application)
7. [Reconstruction Algorithm](#7-reconstruction-algorithm)
8. [Calibration & Validation](#8-calibration--validation)
9. [Use Cases & Target Audience](#9-use-cases--target-audience)
10. [Directory Structure](#10-directory-structure)
11. [Getting Started](#11-getting-started)

---

## 1. Overview & Purpose

**SonicSight** is a complete, open-hardware solution for **acoustic tomography** — a non-destructive testing technique that uses sound wave propagation to image the internal structure of materials. The fundamental principle is simple: sound travels faster through solid, intact material and slower through decayed, cracked, or voided material. By measuring the time-of-flight (ToF) of ultrasonic pulses along many different ray paths between transducers arranged around the test object's perimeter, SonicSight reconstructs a 2D map of acoustic velocity (or slowness, its reciprocal) that directly reveals internal anomalies.

### Why Acoustic Tomography?

Existing non-destructive testing methods for wood and composites are limited:

| Method | Limitation |
|--------|-----------|
| **Visual inspection** | Can only detect surface defects |
| **Resistance drilling** | Invasive, single-path, misses off-axis decay |
| **Screw withdrawal** | Measures only localized holding strength |
| **X-ray / CT scanning** | Expensive, safety concerns, impractical for field use |
| **Ground-penetrating radar** | Limited resolution in wood, requires coupling |
| **Commercial acoustic tomographs** (e.g., PICUS, Arbotom) | $8,000–$25,000, closed/proprietary, no user-serviceable parts |

SonicSight aims to deliver **comparable or superior diagnostic capability at <10% of the cost** (~$450 BOM), fully open from transducer driver to tomographic reconstruction.

### Key Innovations

1. **32-channel arbitrary transmit beamforming** — any subset of transducers can be fired simultaneously with programmable phase delays, enabling directional steering, focused interrogation of specific zones, and synthetic aperture techniques.

2. **On-device ray-path time-of-flight extraction** — the STM32H7's CORDIC and DSP extensions run a cross-correlation peak detector in hardware-accelerated fixed-point, extracting ToF for all N×(N−1) ray paths in under 2 seconds for a 32-sensor array.

3. **Python-free embedded reconstruction** — the firmware itself runs a damped least-squares (Tikhonov-regularized) tomographic inversion on the MCU using the ARM CMSIS-DSP matrix library, outputting a 64×64 slowness grid directly. No connected PC needed for basic imaging.

4. **Field-calibration with reference phantom** — a calibration routine using a known acrylic disc with drilled defect holes compensates for transducer coupling variation, cable delays, and temperature dependence.

5. **BLE 5.3 real-time streaming** — raw ray-path ToF data and reconstructed images stream over BLE to a companion React Native app for live display, annotation, and report generation.

---

## 2. How It Works

### 2.1 Transducer Arrangement

The user places 8–32 piezoelectric transducers (typically 40–100 kHz resonant frequency) around the circumference of the test object — a tree trunk, timber beam, or composite panel — at roughly equal spacing. Each transducer is coupled to the surface with a small dab of silicone grease or acoustic gel. Spring-loaded clamp bands or elastic straps hold them in place.

### 2.2 Data Acquisition Cycle

1. **Transmit phase**: One transducer is excited with a burst of 3–5 cycles at its resonant frequency (e.g., 50 kHz) at 150 Vpp via a high-speed MOSFET pulser. The pulse is generated by a PWM timer output of the STM32.

2. **Multiplexed receive**: All remaining transducers are simultaneously sampled by the 8-channel 50 MSPS ADC through a 32:8 analog multiplexer bank (four ADG732 32:1 muxes feeding four ADS5281 quad-ADC channels). The firmware sequentially cycles through mux configurations to capture all receiver channels for each transmitter.

3. **Signal conditioning**: Each receive channel passes through a low-noise preamplifier (40 dB gain), a programmable bandpass filter (center frequency programmable 20–200 kHz), and a variable gain stage (0–40 dB in 1 dB steps) before the ADC.

4. **ToF extraction**: The captured waveforms are cross-correlated against the transmit pulse template. The peak of the cross-correlation (interpolated to sub-sample precision) gives the arrival time. For noisy signals, a threshold-based first-arrival pick (AIC picker or STA/LTA trigger) is used as fallback.

5. **Cycle repeat**: Steps 1–4 repeat for each of the N transmitters, yielding N×(N−1)/2 unique ray paths (496 paths for a 32-sensor array).

6. **Tomographic inversion**: The set of travel times and known sensor positions are fed into a damped least-squares solver that discretizes the cross-section into a 64×64 pixel grid and iteratively solves for the slowness (1/velocity) in each cell.

### 2.3 Reconstruction Pipeline

```
Sensor positions (θ, r) → Ray tracing (straight-ray approximation)
→ Build sensitivity matrix S (M rays × N pixels) 
→ Solve: (S^T S + λI) m = S^T t
→ m = slowness vector → reshape to 64×64 image
→ Apply smoothing filter → Display as velocity map (m/s)
```

The regularization parameter λ (Tikhonov factor) is automatically selected via the L-curve criterion or set manually by the user through the app.

---

## 3. Hardware Specifications

### 3.1 Core Components

| Component | Part Number | Purpose |
|-----------|-------------|---------|
| **MCU** | STM32H743ZIT6 | Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM, dual-bank, CORDIC + DSP extensions |
| **Quad ADC** | ADS5281 × 2 | 8 channels total, 50 MSPS, 12-bit, LVDS serial output (per channel) |
| **Transmit Pulser** | MD1210K (MOSFET driver) + TC6320 (half-bridge) | 150 Vpp, 5 A peak, burst-mode excitation |
| **Analog Mux** | ADG732 × 4 | 32:1 differential mux, 4 ns switching, −3 dB @ 220 MHz |
| **PGA + Filter** | LTC1563-2 (4th-order programmable filter) + AD8331 (VGA) | 20–200 kHz BW, 0–40 dB gain |
| **HV Supply** | LT3905 (DC-DC) + LT3481 (regulated boost) | +150 V transmit rail, +5 V analog rail |
| **BLE Module** | NRF52840 (standalone module) | BLE 5.3, 8 MB flash for data logging, 1 MB SRAM for image buffering |
| **IMU** | ICM-20948 | 9-axis, for sensor orientation logging and tilt correction |
| **Temperature** | TMP117 | ±0.1°C precision, for temperature-dependent velocity compensation |
| **Battery** | 18650 × 2 (series) | 7.4 V nominal, 3500 mAh each → ~8 hours continuous scanning |
| **Display** | 1.54" 240×240 IPS LCD (GC9A01) | Real-time status, signal quality indicators, basic image preview |
| **SD Card** | microSD via SPI | Raw waveform storage (up to 32 GB), calibration profiles |

### 3.2 Specifications Table

| Parameter | Value |
|-----------|-------|
| **Transducer channels** | 32 (expandable to 64 via daisy-chain) |
| **Transducer types** | 30–150 kHz PZT discs, 15 mm diameter, spring-loaded housings |
| **Excitation voltage** | 50–150 Vpp programmable, burst 1–20 cycles |
| **Receive bandwidth** | 1 kHz – 500 kHz (−3 dB) |
| **ADC resolution** | 12-bit, 50 MSPS per channel |
| **Input noise floor** | < 5 µV/√Hz (RTI) |
| **Max gain** | 80 dB (40 dB LNA + 40 dB VGA) |
| **Time-of-flight resolution** | < 1 µs (with cross-correlation interpolation: < 0.1 µs) |
| **Ray paths per scan** | Up to 496 (32 sensors) |
| **Reconstruction grid** | 64 × 64 pixels (user-selectable 32×32 to 128×128) |
| **Scan time (full 32-sensor)** | ~4 seconds (3600 with averaging × 3) |
| **Image update rate** | 1 frame per 5 seconds (streaming) |
| **Battery life** | 8 hours continuous / 14 days standby |
| **Storage** | microSD up to 32 GB (FAT32) |
| **Connectivity** | BLE 5.3, USB-C (CDC + MSC + DFU), UART debug |
| **Dimensions** | 160 × 120 × 40 mm (enclosure) |
| **Weight** | ~420 g (with battery) |
| **Operating temp** | −10 °C to +50 °C |
| **Ingress protection** | IP54 (splash-resistant, with sealed connector panel) |

### 3.3 Connector Panel

| Connector | Type | Purpose |
|-----------|------|---------|
| TX/RX 1–32 | 50-pin DSUB hybrid | Individual transducer connections (shielded twisted pairs) |
| USB-C | USB 2.0 + PD | Data, charging (5–20 V PD input), DFU firmware update |
| Trigger In | BNC | External trigger sync for multi-unit phased arrays |
| Probe ID | 6-pin micro-JST | External temperature/humidity probe for ambient compensation |

---

## 4. Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         SONICSIGHT MAIN PCB                              │
│                                                                          │
│  ┌─────────────┐     ┌──────────────────┐     ┌──────────────────────┐  │
│  │   Battery    │────→│  Power Tree      │────→│  +3.3V / +5V /       │  │
│  │  2× 18650    │     │  LT3905 + LT3481 │     │  +150V / −5V         │  │
│  └─────────────┘     │  TPS63020 (3.3V)  │     │  (all regulated)     │  │
│                      └──────────────────┘     └──────────────────────┘  │
│                                                                          │
│  ┌───────────────────────────┐  ┌──────────────────────────────────────┐│
│  │    TX PULSE SECTION       │  │    RX ANALOG SECTION                 ││
│  │  ┌─────────────────────┐  │  │  ┌──────────┐  ┌─────┐  ┌────────┐ ││
│  │  │ STM32 TIM PWM (×8)  │  │  │  │ ADG732   │→│LNA  │→│AD8331  │ ││
│  │  │ → Gate drive (MD1210)│──│──│→│ 32:1 Mux │  │40dB │  │VGA 0–40│ ││
│  │  │ → Half-bridge TC6320 │  │  │  │ (×4)     │  └─────┘  │ dB     │ ││
│  │  │ → 150 Vpp burst out  │  │  │  └──────────┘           └───┬────┘ ││
│  │  └─────────────────────┘  │  │                               │      ││
│  │  ┌─────────────────────┐  │  │  ┌──────────┐  ┌──────────┐  │      ││
│  │  │ TX MUX (×4 ADG732)  │──│──│→│ LTC1563-2│→│ ADS5281  │←─┘      ││
│  │  │ → Routes HV pulse    │  │  │  │ 4th-order│  │ 50 MSPS  │        ││
│  │  │   to any transducer  │  │  │  │ LPF/BPF  │  │ 12-bit   │        ││
│  │  └─────────────────────┘  │  │  └──────────┘  └────┬─────┘        ││
│  └───────────────────────────┘  │                      │ LVDS         ││
│                                 │  ┌────────────────────┴──────────┐  ││
│                                 │  │  STM32H743 (Cortex-M7 @ 480MHz)│  │
│                                 │  │  • DCMI (LVDS deserializer)    │  │
│                                 │  │  • FMC (PSRAM for image buf)   │  │
│                                 │  │  • SDMMC (microSD)             │  │
│                                 │  │  • SPI (LCD, NRF52, IMU)       │  │
│                                 │  │  • CORDIC (cross-correlation)  │  │
│                                 │  │  • DSP (matrix inversion)      │  │
│                                 │  └──────────┬───────────────────┘  │
│                                 │             │                       │
│  ┌──────────────────────────────┴─────────────┴────────────────────┐ │
│  │  PERIPHERALS                                                    │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌──────────┐  ┌──────────┐  │ │
│  │  │ NRF52840    │  │ ICM-20948   │  │ GC9A01   │  │ TMP117   │  │ │
│  │  │ BLE 5.3 +   │  │ 9-axis IMU  │  │ 240×240  │  │ ±0.1°C   │  │ │
│  │  │ 8MB flash   │  └─────────────┘  │ IPS LCD  │  │ temp     │  │ │
│  │  └─────────────┘                   └──────────┘  └──────────┘  │ │
│  └─────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.1 Signal Flow

1. **Transmit path**: STM32 TIM1/TIM8 generate burst waveforms → MD1210 MOSFET driver → TC6320 half-bridge → TX MUX (ADG732) → selected transducer (150 Vpp, 3–5 cycle burst)

2. **Receive path**: All 32 transducers → 4× 32:1 muxes (ADG732) → 8× LNA (fixed 40 dB) → 8× VGA (AD8331, 0–40 dB) → 8× LTC1563-2 filters → 2× ADS5281 quad ADCs (8 channels total, 50 MSPS) → LVDS → STM32 DCMI port

3. **Processing**: STM32 deserializes LVDS → DMA into SRAM ping-pong buffers → cross-correlation via CORDIC → ray-path assembly → CMSIS-DSP matrix solve → 64×64 grid → color-map → SPI to LCD + BLE to app

4. **Power tree**: 2× 18650 (7.4 V) → LT3905 (boost to +150 V for TX) + LT3481 (buck to +5 V for analog) + TPS63020 (buck-boost to +3.3 V for digital) + TPS7A47 (low-noise +3.3 V for analog front-end)

---

## 5. Firmware Design

### 5.1 Architecture

The firmware follows a **super-loop with interrupt-driven state machine** architecture, chosen for deterministic timing of the acquisition sequence:

```
main() → HAL_Init() → SystemClock_Config() → MX_GPIO_Init()
  → MX_DCMI_Init() → MX_SPI_Init() → MX_SDMMC_Init()
  → MX_TIM_Init() → MX_FMC_Init()
  → sonicsight_init() → calibrate_transducers() 
  → while(1) {
      state_machine_run()
      if (state == IDLE)      → sleep_until_request()
      if (state == ACQUIRE)   → run_acquisition_cycle()
      if (state == PROCESS)   → run_tomographic_inversion()
      if (state == STREAM)    → send_results_via_ble()
      if (state == CALIBRATE) → run_calibration_routine()
    }
```

### 5.2 Acquisition State Machine

| State | Description | Timing |
|-------|-------------|--------|
| `IDLE` | Low-power sleep (~2 mA), BLE advertising, waiting for app command or button press | Indefinite |
| `ARM_SENSORS` | Clamp-belt tension check, coupling quality test (short echo pulse per transducer) | 2 s |
| `TX_CHANNEL_i` | Fire transducer i, capture all N−1 receivers, extract ToFs for this transmitter | 100 ms per TX |
| `ALL_TX_DONE` | All N transmitters fired; ray matrix complete | N × 100 ms |
| `RECONSTRUCT` | Run tomographic inversion, generate image | ~2 s (32 sensors) |
| `DISPLAY` | Push image to LCD, generate report data | 200 ms |
| `STREAM` | Stream via BLE to companion app | Continuous |
| `CALIBRATE` | Measure reference phantom, set gain/timing offsets | 30 s |
| `ERROR` | Fault display (bad coupling, low battery, SD error) | Until reset |

### 5.3 Key Design Decisions

**Why STM32H7 instead of an FPGA for signal processing?**  
The H7's CORDIC and DSP extensions, combined with CMSIS-DSP, can cross-correlate 496 ray paths (each 1024 samples × 12 bit) in ~800 ms. An FPGA would be faster but would dramatically increase power consumption and reduce the accessibility of the design. The H7 strikes the optimal balance between processing power and ease of development for an open-source project.

**Why 150 Vpp transmit?**  
Acoustic attenuation in wood at 50 kHz is ~1–2 dB/cm. For a 50 cm diameter tree trunk, round-trip path lengths of 70+ cm mean the signal must survive 70–140 dB of attenuation. Higher transmit voltage improves SNR without requiring preamp gains that would introduce noise. 150 Vpp is the practical limit for compact DC-DC converters and MOSFET pulse circuits.

**Cross-correlation vs. threshold-based first arrival:**  
Threshold-based picking (e.g., AIC or STA/LTA) is faster but less accurate in noisy conditions. The firmware implements both: cross-correlation (sub-sample precision, ~0.1 µs accuracy) is the default; the AIC picker is used as a validation check and on channels where the cross-correlation SNR is below 3 dB.

**On-board reconstruction vs. cloud offload:**  
In field environments, network connectivity cannot be guaranteed. Performing the reconstruction on the device ensures the operator always gets a result. The raw ToF data is also stored to SD card so the user can re-process with different regularization parameters later on their laptop.

### 5.4 File Structure

| File | Purpose |
|------|---------|
| `main.c` | Entry point, state machine, acquisition loop |
| `board.h` | Pin mappings, peripheral base addresses, oscillator frequencies |
| `registers.h` | Register bit definitions for custom peripherals, ADC modes, mux control |
| `acquisition.c` / `acquisition.h` | Transducer firing sequence, ADC capture via DCMI, mux switching |
| `crosscorr.c` / `crosscorr.h` | Cross-correlation peak detection with sub-sample interpolation |
| `tomography.c` / `tomography.h` | Tikhonov-regularized least-squares reconstruction, ray tracing |
| `calibration.c` / `calibration.h` | Reference phantom measurement, per-channel gain/offset calibration |
| `ble.c` / `ble.h` | BLE GATT service definitions, notification streaming |
| `display.c` / `display.h` | GC9A01 LCD driver, color-mapped image rendering, menu system |
| `sdlog.c` / `sdlog.h` | FAT32 file write for waveform and ToF storage |
| `Makefile` | Build system with cross-compiler toolchain |

### 5.5 Memory Map

| Region | Size | Contents |
|--------|------|----------|
| ITCM RAM | 64 KB | Interrupt vectors, critical ISR code |
| DTCM RAM | 128 KB | ADC buffers (ping-pong × 2), stack |
| AXI SRAM | 512 KB | Cross-correlation workspace, ray matrix |
| SRAM1 | 128 KB | Tomography solver workspace |
| SRAM2 | 128 KB | Image buffer (64×64 × 4 bytes float) |
| SRAM3 | 64 KB | BLE GATT database, LCD framebuffer |
| SDRAM (FMC) | 8 MB | Raw waveform storage during acquisition, calibration profiles |
| QSPI Flash | 16 MB | Waveform templates, calibration constants, font bitmaps |

---

## 6. Companion Application

### 6.1 Overview

The **SonicSight Companion** is a React Native mobile app (iOS and Android) that connects to SonicSight via BLE 5.3. It serves as the primary user interface for scanning operations, data visualization, and report generation.

### 6.2 Screens

| Screen | Function |
|--------|----------|
| **Home** | Device status (battery, SD, BLE RSSI), recent scan history gallery, quick-start guide |
| **Live Scan** | Real-time tomogram display (64×64 color velocity map), signal quality bar per transducer, gain/regulator sliders, single-scan and continuous-scan modes |
| **Sensor Layout** | Interactive transducer placement diagram — user marks positions on a photograph of the test object, app computes angular coordinates and sends to device |
| **History** | Sorted list of past scans with thumbnails, labels, GPS locations, and notes |
| **Detail** | Full-size tomogram with crosshair cursor (shows pixel value = velocity in m/s), ToF matrix heatmap overlay, diagnostic report (min/mean/max velocity, anomaly area estimate, decay percentage) |
| **Export** | Generate PDF report (CSN EN 15534 / ASTM D5987 compliance), CSV ToF data export, DICOM-formatted image export for medical-grade cross-referencing |
| **Settings** | BLE pairing, calibration management, firmware update (OTA via BLE DFU), regularization presets, language selection |

### 6.3 Key Features

- **Live BLE streaming**: Up to 1 tomogram per 5 seconds at full resolution, 10 fps at reduced (32×32) resolution
- **Anomaly detection**: Automated contour detection of regions where velocity deviates >30% from the mean, with area estimation in cm²
- **Change monitoring**: Time-lapse comparison of sequential scans with difference maps — critical for tracking decay progression in heritage trees
- **GPS tagging**: Each scan is geotagged; export as KML for Google Earth visualization
- **Multi-unit support**: Connect to multiple SonicSight units for large-area scanning (e.g., scanning a bridge beam with 4 units simultaneously)

---

## 7. Reconstruction Algorithm

### 7.1 Straight-Ray Approximation

For a homogeneous cross-section of diameter D with N sensors at positions (r_i, θ_i), the travel time t_ij between sensor i and sensor j is:

```
t_ij = ∫_L s(x,y) dL
```

where s(x,y) = 1/v(x,y) is the slowness and L is the straight line between sensors. The integral is discretized over a 64×64 pixel grid:

```
t_ij = Σ_k S_ijk · m_k
```

where S_ijk is the length of ray ij passing through pixel k, and m_k is the slowness in pixel k.

### 7.2 Tikhonov Regularization

The system S · m = t is highly underdetermined (496 equations for 4096 unknowns). A damped least-squares solution is used:

```
m = (S^T S + λ L^T L)^{-1} S^T t
```

where L is a second-order derivative (Laplacian) operator enforcing smoothness, and λ is the regularization parameter. The solver uses the Cholesky decomposition (ARM CMSIS-DSP `arm_mat_cholesky_f32`).

### 7.3 Automatic λ Selection

The L-curve criterion (Hansen, 1992) is implemented: the solver runs for λ = 10^−2 to 10^2 in logarithmic steps, and the point of maximum curvature in the (||Sm − t||², ||Lm||²) plane is selected. For field use with limited compute, a fixed λ = 0.5 is the default and works well for most wood species.

### 7.4 Velocity-to-Decay Mapping

Based on published correlations (Bucur, 2006; Dackermann et al., 2014):

| Velocity (m/s) | Interpretation | Color |
|----------------|---------------|-------|
| > 3500 | Sound, intact wood | Blue |
| 2500–3500 | Early decay / low moisture | Cyan → Green |
| 1500–2500 | Moderate decay / fungal attack | Yellow → Orange |
| < 1500 | Advanced decay / void / crack | Red → Black |

---

## 8. Calibration & Validation

### 8.1 Reference Phantom

Each SonicSight unit ships with (and the design includes files for 3D printing) a calibration phantom: a 150 mm diameter acrylic disc (PMMA), 40 mm thick, with five 10 mm diameter holes drilled at specific coordinates (r = 25, 40, 55 mm, at angles 0°, 72°, 144°, 216°, 288°). This provides known defects for:
- Confirming spatial accuracy of the reconstruction
- Calibrating per-channel gain and timing offsets
- Validating the regularization parameter

### 8.2 Coupling Quality Check

Before each scan, all 32 transducers are pulsed briefly at low voltage (20 V, 1 cycle) and the echo from the near surface is measured. A transducer with poor coupling (echo amplitude < 50% of reference, or arrival time > 20 µs from expected) is flagged in the app and can be excluded from the reconstruction.

### 8.3 Temperature Compensation

Wood acoustic velocity decreases by approximately 0.5–1.0% per °C temperature increase (due to changes in modulus of elasticity). The TMP117 sensor provides ±0.1°C accuracy, and the firmware applies a linear correction factor:

```
v_corrected = v_raw × (1 + α(T − T_ref))
```

where α = −0.007 °C⁻¹ (default for oak/softwood) and T_ref = 20 °C. The user can override α in the app for different materials.

---

## 9. Use Cases & Target Audience

### 9.1 Primary Use Cases

| Use Case | Description | Impact |
|----------|-------------|--------|
| **Tree risk assessment** (arborists, municipal tree officers) | Scan standing trees to detect internal decay, hollowing, and root rot before they cause catastrophic failure | Prevents liability, preserves heritage trees |
| **Timber grading** (sawmills, structural engineers) | Assess internal quality of logs before sawing, inspect glued-laminated timber (glulam) for delamination | Reduces waste, ensures structural safety |
| **Heritage conservation** (monuments, historic structures) | Non-invasively inspect wooden beams, pillars, and sculptures in historic buildings | Preserves cultural heritage |
| **Composite NDT** (aerospace, wind energy, marine) | Inspect GFRP/CFRP laminates for delamination, water ingress, and core crush in wind turbine blades and boat hulls | Extends service life, prevents catastrophic failure |
| **Masonry assessment** (civil engineers) | Assess grout integrity in stone columns, detect voids behind retaining walls | Infrastructure safety |

### 9.2 Target Audience

- **Professional arborists and tree surgeons** — the largest immediate market (100,000+ in the US alone)
- **Municipal urban forestry departments** — tree risk management programs
- **Sawmills and timber processors** — log grading optimization
- **Structural engineering firms** — post-disaster and renovation assessments
- **Heritage restoration professionals** — non-invasive building diagnostics
- **Wind turbine maintenance crews** — blade root and adhesive joint inspection
- **Maker and research community** — open-hardware enthusiasts, forestry and NDT researchers
- **University engineering / forestry departments** — teaching tool for acoustic NDT principles

### 9.3 Why Now?

The global NDT services market is projected to reach $28 billion by 2028, with acoustic methods being the fastest-growing segment. At the same time, the availability of high-performance MCUs like the STM32H7 (480 MHz Cortex-M7 with DSP and CORDIC, ~$12 in quantity) and affordable BLE modules with application processors (NRF52840, ~$5) makes an on-device tomographic solver economically feasible for the first time. SonicSight democratizes acoustic tomography — putting a capability previously confined to $20,000+ specialized instruments into an open-hardware, $450 BOM device.

---

## 10. Directory Structure

```
sonicsight/
├── README.md                ← This file
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── acquisition.c
│   ├── acquisition.h
│   ├── crosscorr.c
│   ├── crosscorr.h
│   ├── tomography.c
│   ├── tomography.h
│   ├── calibration.c
│   ├── calibration.h
│   ├── ble.c
│   ├── ble.h
│   ├── display.c
│   ├── display.h
│   ├── sdlog.c
│   └── sdlog.h
├── kicad/
│   ├── sonicsight.kicad_sch
│   ├── sonicsight.kicad_pcb
│   └── sonicsight.kicad_pro
└── app/
    ├── package.json
    ├── App.js
    └── src/
        ├── screens/
        │   ├── HomeScreen.js
        │   ├── LiveScanScreen.js
        │   ├── HistogramScreen.js
        │   ├── DetailScreen.js
        │   └── SettingsScreen.js
        ├── components/
        │   ├── TomogramView.js
        │   ├── SignalQualityBar.js
        │   ├── SensorLayout.js
        │   └── ExportDialog.js
        ├── services/
        │   ├── BleService.js
        │   └── StorageService.js
        └── utils/
            ├── TomoDecoder.js
            └── ReportGenerator.js
```

---

## 11. Getting Started

### 11.1 Building the Firmware

Requirements: ARM GCC toolchain (gcc-arm-none-eabi ≥ 10.3), GNU Make ≥ 4.3, OpenOCD (for flashing via ST-Link).

```bash
cd firmware
make clean
make -j4
# Flash via ST-Link
make flash
# Or via DFU over USB-C:
make dfu
```

### 11.2 KiCad Design

Open `kicad/sonicsight.kicad_pro` in KiCad 7.x or later. The schematic is organized hierarchically:
- `power.sch` — battery management, DC-DC converters, LDOs
- `mcu.sch` — STM32H743 with decoupling, oscillator, JTAG
- `tx_frontend.sch` — high-voltage pulser, TX muxes
- `rx_frontend.sch` — analog muxes, LNAs, VGAs, filters, ADCs
- `peripherals.sch` — BLE module, IMU, LCD, SD card, temp sensor

### 11.3 Running the App

```bash
cd app
npm install
npx react-native run-ios    # or run-android
```

### 11.4 Calibrating for First Use

1. Place transducers on the acrylic calibration phantom
2. Connect all 32 cables
3. Power on SonicSight, open the app
4. Navigate to Settings → Calibration → "Run Full Calibration"
5. Wait ~30 seconds for the automated sequence
6. Save calibration profile

---

**SonicSight** is dedicated to the arborists, engineers, conservators, and researchers who keep our structures safe and our heritage standing. By making acoustic tomography open and affordable, we aim to democratize non-destructive evaluation — one ray path at a time.

*— jayis1, 2026*