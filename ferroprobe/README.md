# FerroProbe — Open-Source 3-Axis Vector Fluxgate Magnetometer

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

![PCB](https://img.shields.io/badge/PCB-120×72mm-blue)
![MCU](https://img.shields.io/badge/MCU-STM32H743%20Cortex--M7-orange)
![Sensor](https://img.shields.io/badge/Sensor-3%E2%80%90Axis%20Fluxgate-green)
![Resolution](https://img.shields.io/badge/Resolution-0.1%20nT-brightgreen)
![BLE](https://img.shields.io/badge/BLE-5.0-yellow)
![License](https://img.shields.io/badge/License-MIT-yellow)

> **FerroProbe is a portable, battery-powered 3-axis vector fluxgate magnetometer that achieves sub-nanotesla (0.1 nT) resolution for geophysical surveying, archaeological prospecting, UXO detection, pipeline inspection, and space science. It combines three orthogonal fluxgate sensors with a 6-axis IMU for orientation-compensated vector measurement, GPS positioning, SD-card logging, and a React Native companion app that renders real-time magnetic field maps. Commercial fluxgate magnetometers cost $2,000–$20,000+; FerroProbe brings this capability to researchers, citizen scientists, and field engineers as a fully open-source design.**

---

## Table of Contents

1. [Overview & Purpose](#1-overview--purpose)
2. [How It Works — Fluxgate Magnetometry](#2-how-it-works--fluxgate-magnetometry)
3. [Hardware Specifications](#3-hardware-specifications)
4. [Architecture & Block Diagram](#4-architecture--block-diagram)
5. [Fluxgate Sensor Design](#5-fluxgate-sensor-design)
6. [Lock-In Detection Engine](#6-lock-in-detection-engine)
7. [Firmware Design](#7-firmware-design)
8. [Companion Application](#8-companion-application)
9. [Calibration & Orientation Compensation](#9-calibration--orientation-compensation)
10. [Use Cases & Target Audience](#10-use-cases--target-audience)
11. [Directory Structure](#11-directory-structure)

---

## 1. Overview & Purpose

### 1.1 The Problem

Precise measurement of the Earth's magnetic field — and anomalies within it — is a fundamental capability across multiple scientific and engineering disciplines. Archaeologists map subsurface structures by detecting magnetic anomalies from fired clay, hearths, and metal objects. Geophysicists prospect for mineral deposits by mapping magnetic susceptibility variations. Unexploded ordnance (UXO) clearance teams locate buried munitions through their ferromagnetic signatures. Pipeline operators detect corrosion and mechanical damage through magnetic flux leakage. Space scientists characterize spacecraft attitude and map planetary magnetospheres.

The instrument of choice for these applications is the **fluxgate magnetometer**: a device that measures DC and low-frequency magnetic fields with nanotesla-level resolution, vector sensitivity (direction + magnitude), and no cryogenic requirements. Unlike Hall-effect sensors (limited to ~µT resolution) or SQUIDs (requiring liquid helium), fluxgates operate at room temperature, consume modest power, and achieve the dynamic range and sensitivity needed for field surveying.

However, commercial fluxgate magnetometers are expensive ($2,000–$20,000+ for research-grade instruments from Bartington, Magson, Stevens, or Billingsley), proprietary, and offer limited customization. Researchers who need to deploy arrays of sensors, integrate magnetometers into custom platforms, or adapt measurement protocols are locked into vendor ecosystems with closed firmware and undocumented calibration procedures. There is no comprehensive open-source fluxgate magnetometer design with complete schematics, firmware, and companion software.

### 1.2 The Solution — FerroProbe

FerroProbe is a complete, open-source 3-axis vector fluxgate magnetometer designed for field surveying and research. It provides:

- **Three orthogonal fluxgate sensors** built around high-permeability mu-metal cores, driven by a common excitation oscillator at 15.625 kHz. Each axis is independently demodulated using a digital lock-in amplifier running on the STM32H743's Cortex-M7 DSP engine, achieving 0.1 nT resolution over a ±100 µT measurement range (covering the full Earth's field).

- **Orientation-compensated vector measurement**: A BMI270 6-axis IMU (accelerometer + gyroscope) provides real-time attitude estimation at 400 Hz. The measured magnetic vector is rotated from the sensor frame to the Earth-level frame, enabling walk-mode magnetic surveying where the device orientation changes continuously during data collection.

- **GPS-tagged data logging**: A u-blox NEO-M9N GNSS receiver provides position fixes at 10 Hz with 2 m CEP accuracy. Every magnetic field sample is tagged with latitude, longitude, altitude, and UTC timestamp, enabling post-processing into magnetic anomaly maps.

- **On-device anomaly detection**: A configurable threshold-based algorithm flags magnetic anomalies in real time, with audio/visual alerts (buzzer + RGB LED) for field UXO surveys where the operator needs immediate feedback.

- **Comprehensive data export**: Raw and processed data are logged to a FAT32-formatted microSD card in binary format with a companion CSV/JSON export mode. Data includes timestamp, position, raw Bx/By/Bz, calibrated Bx/By/Bz, total field |B|, device orientation (roll/pitch/heading), and temperature.

- **React Native companion app**: A mobile app connects via BLE 5.0 to display real-time 3-axis magnetic field vectors, a time-series strip chart, a GPS-tagged survey map with color-coded anomaly overlays, calibration management, and data download/export.

### 1.3 Design Philosophy

FerroProbe is designed to be **buildable by a motivated individual** using standard PCB fabrication services (JLCPCB, OSH Park), commercially available components (DigiKey/Mouser), and a 3D-printed or machined enclosure. The fluxgate sensors use readily available mu-metal tape (Coilcraft 53-22 or equivalent high-µ cores) wound with enameled copper wire. No custom ASICs, no NDA-restricted parts, no specialized tooling. The total BOM cost target is under $150 in single quantities.

---

## 2. How It Works — Fluxgate Magnetometry

### 2.1 The Fluxgate Principle

A fluxgate magnetometer exploits the nonlinear (saturating) magnetic permeability of a ferromagnetic core. The core is driven into magnetic saturation in both polarities by an AC excitation field from a drive coil. In the absence of an external field, the core's magnetization is symmetric, and the sense coil picks up only odd harmonics of the excitation frequency. When an external DC field is present, it biases the core's magnetization curve, breaking the symmetry and producing a **second harmonic** component in the sense coil output that is proportional to the external field magnitude and direction.

The key signal processing step is **synchronous (lock-in) detection**: the sense coil output is multiplied by a reference signal at the second harmonic of the excitation frequency (2f = 31.25 kHz), then low-pass filtered to extract the DC component proportional to the external field. This rejects noise at all other frequencies, providing excellent sensitivity and noise rejection.

### 2.2 Second-Harmonic Detection

For a fluxgate driven at excitation frequency f₀, the sense coil voltage contains:

V_sense(t) = Σ [aₙ · sin(n · 2πf₀t)]

where the coefficients aₙ depend on the core material, excitation amplitude, and external field B_ext. The second harmonic coefficient a₂ is approximately linearly proportional to B_ext:

a₂ ≈ k · B_ext · N_s · μ_core · f₀

where k is a geometry-dependent constant, N_s is the number of sense turns, and μ_core is the core permeability. The lock-in amplifier extracts a₂ by mixing with a phase-locked 2f₀ reference and integrating.

### 2.3 Three-Axis Implementation

FerroProbe uses three identical fluxgate sensors mounted orthogonally — one for each Cartesian axis (X, Y, Z). All three share a common excitation oscillator (synchronized), so a single 2f₀ reference signal serves all three demodulators. This reduces clock jitter and ensures phase coherence between axes, which is critical for accurate vector measurement and cross-axis calibration.

---

## 3. Hardware Specifications

| Parameter | Value |
|---|---|
| **Measurement Range** | ±100 µT per axis (covers full Earth's field ±65 µT) |
| **Resolution** | 0.1 nT (24-bit ADC, 1 Hz output rate) |
| **Noise Floor** | < 10 pT/√Hz at 1 Hz |
| **Bandwidth** | DC – 100 Hz (configurable via digital filter) |
| **Axes** | 3 (orthogonal, X/Y/Z vector) |
| **Accuracy** | ±1 nT after calibration (±0.5 nT with temperature compensation) |
| **Sampling Rate** | 1–100 Hz configurable (10 Hz default for survey mode) |
| **Excitation Frequency** | 15.625 kHz (common to all 3 axes) |
| **MCU** | STM32H743VIT6 — Cortex-M7 @ 480 MHz, 2 MB Flash, 1 MB SRAM |
| **ADC** | TI ADS1256 — 24-bit, 8-channel, 30 kSPS (for analog sense signals) |
| **DAC** | TI DAC8568 — 16-bit, 8-channel (for feedback/nulling coils) |
| **IMU** | Bosch BMI270 — 6-axis accelerometer/gyroscope, 400 Hz |
| **GNSS** | u-blox NEO-M9N — multi-constellation, 10 Hz, 2 m CEP |
| **BLE** | Nordic nRF52840 — BLE 5.0, 2 Mbps PHY |
| **Storage** | MicroSD card (FAT32), up to 32 GB |
| **Display** | SSD1306 OLED — 128×64 monochrome (status + field reading) |
| **Power** | 3.7V 18650 LiPo (2200 mAh), USB-C charging (MCP73871) |
| **Battery Life** | ~8 hours continuous surveying at 10 Hz |
| **Connectivity** | USB-C (data + charging), BLE 5.0, microSD |
| **Operating Temp** | -20°C to +55°C (commercial), -40°C to +85°C with industrial-grade parts |
| **Form Factor** | 120mm × 72mm × 28mm (handheld wand with sensor head) |
| **Weight** | ~180 g (with battery) |
| **Enclosure** | 3D-printed PETG or machined aluminum (sensor head mu-metal shielded) |

### Power Budget

| Component | Current | Notes |
|---|---|---|
| STM32H743 | 35 mA | 480 MHz, peripherals active |
| ADS1256 (ADC) | 25 mA | 24-bit, continuous conversion |
| DAC8568 | 1.5 mA | Low-power DAC for feedback coils |
| BMI270 (IMU) | 0.85 mA | 400 Hz accel + gyro |
| NEO-M9N (GPS) | 27 mA | 10 Hz fix rate |
| nRF52840 (BLE) | 8 mA | Connected, 2 Mbps |
| SSD1306 (OLED) | 12 mA | Display on |
| Fluxgate drivers | 20 mA | Excitation coil drive (3 axes) |
| SD card | 15 mA | Write operations (average) |
| **Total** | **~144 mA** | **~15 hours from 2200 mAh** (with duty-cycling: ~8 h) |

---

## 4. Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          FerroProbe Architecture                          │
└─────────────────────────────────────────────────────────────────────────┘

  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
  │  X-Axis      │    │  Y-Axis      │    │  Z-Axis      │
  │  Fluxgate    │    │  Fluxgate    │    │  Fluxgate    │
  │  Sensor      │    │  Sensor      │    │  Sensor      │
  │ (Mu-Metal +  │    │ (Mu-Metal +  │    │ (Mu-Metal +  │
  │  Drive/Sense │    │  Drive/Sense │    │  Drive/Sense │
  │  Coils)      │    │  Coils)      │    │  Coils)      │
  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
         │ Vx_sense          │ Vy_sense          │ Vz_sense
         │                   │                   │
  ┌──────▼───────┐    ┌──────▼───────┐    ┌──────▼───────┐
  │ Preamp +     │    │ Preamp +     │    │ Preamp +     │
  │ Bandpass     │    │ Bandpass     │    │ Bandpass     │
  │ (2f₀=31.25k) │    │ (2f₀=31.25k) │    │ (2f₀=31.25k) │
  └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
         │                   │                   │
         │  AIN0             │  AIN1             │  AIN2
         ▼                   ▼                   ▼
  ┌──────────────────────────────────────────────────────────┐
  │              TI ADS1256 24-bit ADC                        │
  │              (8-channel, 30 kSPS, SPI to MCU)           │
  │              Also: AIN3 = temp sensor, AIN4-7 = spare    │
  └──────────────────────┬───────────────────────────────────┘
                         │ SPI (MOSI/MISO/SCK/CS)
                         ▼
  ┌──────────────────────────────────────────────────────────┐
  │                STM32H743VIT6 (Cortex-M7 @ 480 MHz)       │
  │                                                          │
  │  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐  │
  │  │ Lock-In    │  │ Calibration  │  │ Orientation      │  │
  │  │ Demodulate │  │ (offset,     │  │ Compensation     │  │
  │  │ (3 axes)   │  │ scale, temp) │  │ (IMU→quaternion) │  │
  │  └────────────┘  └──────────────┘  └──────────────────┘  │
  │                                                          │
  │  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐  │
  │  │ Survey     │  │ Anomaly      │  │ Data Logger      │  │
  │  │ State      │  │ Detector     │  │ (SD + BLE)       │  │
  │  │ Machine    │  │ (threshold)  │  │                  │  │
  │  └────────────┘  └──────────────┘  └──────────────────┘  │
  └──┬────────┬────────┬────────┬────────┬────────┬─────────┘
     │ SPI    │ I2C    │ UART   │ SPI    │ GPIO    │ USB-C
     │        │        │        │        │        │
     ▼        ▼        ▼        ▼        ▼        ▼
  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐
  │DAC85 │ │BMI270│ │NEO-  │ │micro │ │RGB + │ │USB-C │
  │68    │ │IMU   │ │M9N   │ │SD    │ │Buzzer│ │Data/ │
  │(feedb│ │      │ │GPS   │ │card  │ │LED   │ │Power │
  │ null)│ │      │ │      │ │      │ │      │ │      │
  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘

                         ┌───────────┐
                         │  nRF52840  │
                         │  BLE 5.0   │
                         │ (UART to   │
                         │  H743)     │
                         └─────┬─────┘
                               │ BLE
                               ▼
                     ┌──────────────────┐
                     │  React Native    │
                     │  Companion App   │
                     │  (iOS/Android)   │
                     └──────────────────┘

  Excitation Path:
  ┌─────────────┐    PWM      ┌──────────────┐    Drive    ┌─────────────┐
  │  STM32H743  │────────────▶│ H-Bridge     │───────────▶│ X/Y/Z Drive │
  │  TIM1 PWM   │  15.625 kHz │ (TPS28225 +  │  Coils     │ Coils (3)   │
  │  2f₀ ref    │             │ FDS8958A)    │            │             │
  └─────────────┘             └──────────────┘            └─────────────┘
```

### Bus Topology

| Bus | Devices | Notes |
|---|---|---|
| SPI1 | ADS1256 (24-bit ADC), microSD card | 12 MHz max for ADS1256, 25 MHz for SD |
| SPI2 | DAC8568 (feedback DAC) | 20 MHz |
| I2C1 | BMI270 (IMU), SSD1306 (OLED) | 400 kHz |
| UART4 | nRF52840 (BLE module) | 1 Mbps, hardware flow control |
| USART1 | NEO-M9N (GPS) | 38400 baud, NMEA |
| USB | USB-C (CDC + MSC) | Data transfer + charging |
| TIM1 | Fluxgate excitation PWM | 15.625 kHz, center-aligned |
| TIM2 | ADC sampling trigger | Synced to TIM1 2f₀ |

---

## 5. Fluxgate Sensor Design

### 5.1 Core Material

Each fluxgate sensor uses a **high-permeability ring core** wound from 25 µm mu-metal tape (permalloy, µ_r > 80,000). The ring core has an outer diameter of 14 mm, inner diameter of 10 mm, and height of 3 mm — fabricated by winding mu-metal tape on a mandrel and annealing at 1150°C in a hydrogen atmosphere. For the open-source version, commercially available Coifil cores (or Coilcraft 53-series ferrite cores with acceptable permeability) can be substituted with slightly reduced sensitivity.

### 5.2 Coil Winding

Each sensor has two coils wound on the ring core:

- **Drive (excitation) coil**: 50 turns of 0.2 mm enameled copper wire, wound bifilar around the full circumference of the ring core. Driven by the H-bridge at 15.625 kHz with ~20 mA peak current, sufficient to saturate the core in both polarities.

- **Sense (pickup) coil**: 200 turns of 0.1 mm enameled copper wire, wound over the drive coil. Picks up the second-harmonic voltage (at 31.25 kHz) proportional to the external field component along the sensor axis.

The three ring cores are mounted in an orthogonal arrangement on a 3D-printed sensor head, with the Z-axis core oriented vertically and the X/Y cores horizontal and perpendicular.

### 5.3 Signal Conditioning

The sense coil output (typically 1–50 µV at 31.25 kHz for Earth's field) passes through:

1. **Low-noise preamplifier**: AD8429 instrumentation amplifier, gain = 100, noise = 1 nV/√Hz
2. **Bandpass filter**: Center frequency 31.25 kHz, Q = 10, using OPA1637 full-differential amplifier
3. **Anti-aliasing filter**: 2nd-order Butterworth at 1 kHz (post lock-in, before ADC)
4. **Level shifter**: Converts bipolar signal to 0–5V for ADS1256 unipolar input

The ADS1256 24-bit ADC digitizes the conditioned signals at 1 kHz per channel (well above the 100 Hz signal bandwidth), with software lock-in demodulation performed on the STM32H743.

### 5.4 Feedback Nulling

For highest linearity, FerroProbe implements **closed-loop feedback**: the DAC8568 drives a nulling coil around each sensor that cancels the external field, keeping the core near zero bias. The feedback current is proportional to the field, and the DAC output serves as a coarse measurement while the lock-in provides fine resolution. This technique extends the linear range beyond the core's saturation point and reduces temperature sensitivity.

---

## 6. Lock-In Detection Engine

### 6.1 Digital Lock-In Algorithm

The lock-in detection is implemented entirely in firmware on the STM32H743, leveraging the Cortex-M7's DSP extension instructions (SIMD, MAC) for efficient computation.

The ADC samples each axis at f_s = 30 kSPS. For each axis, the firmware performs:

1. **Multiply** each sample by the reference sinusoid at 2f₀ = 31.25 kHz (phase-locked to the excitation via TIM1)
2. **Low-pass filter** the product using a 4th-order cascaded integrator-comb (CIC) filter with decimation to 1 kHz, followed by a 2nd-order IIR Butterworth at 100 Hz
3. **Output** the demodulated DC value, which is proportional to the external field component

The reference signal is generated from the TIM1 excitation PWM: the 2f₀ reference is a square wave at 31.25 kHz, synchronized to the excitation. The lock-in processes both in-phase (I) and quadrature (Q) components to extract both magnitude and phase, allowing calibration of the sensor phase response.

### 6.2 Data Rate and Filtering

| Mode | Output Rate | Bandwidth | Averaging | Application |
|---|---|---|---|---|
| Fast | 100 Hz | 0–50 Hz | 4x | High-rate transient capture |
| Survey | 10 Hz | 0–5 Hz | 40x | Walking survey (default) |
| Precision | 1 Hz | 0–0.5 Hz | 400x | Static measurement, calibration |
| Ultra | 0.1 Hz | 0–0.05 Hz | 4000x | Observatory-grade monitoring |

---

## 7. Firmware Design

### 7.1 Architecture

The firmware follows a **cooperative super-loop** architecture with interrupt-driven data acquisition. The main loop runs the state machine, display update, and BLE communication, while high-priority interrupts handle ADC sampling, GPS parsing, and IMU reading.

### 7.2 State Machine

```
                    ┌──────────┐
                    │  BOOT    │
                    │  INIT    │
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │  IDLE    │◄──────────────────────┐
                    │  (OLED   │                       │
                    │  status) │                       │
                    └────┬─────┘                       │
            ┌────────────┼────────────┐                │
            │            │            │                │
       ┌────▼────┐  ┌───▼────┐  ┌───▼──────┐      ┌───▼────┐
       │ SURVEY  │  │CALIB   │  │MONITOR   │      │ CONFIG │
       │ (GPS +  │  │(3-axis │  │(static   │      │(BLE    │
       │  SD log)│  │ offset)│  │ reading) │      │ settings│
       └────┬────┘  └───┬────┘  └───┬──────┘      └───┬────┘
            │           │            │                 │
            └───────────┴────────────┴─────────────────┘
                         │
                    ┌────▼─────┐
                    │ SHUTDOWN │
                    │ (save +  │
                    │  power   │
                    │  off)    │
                    └──────────┘
```

### 7.3 Key Firmware Modules

| Module | File | Function |
|---|---|---|
| Main loop + state machine | `main.c` | Initialization, super-loop, mode transitions |
| Fluxgate excitation | `drivers/fluxgate.c` | PWM configuration, H-bridge control, excitation timing |
| Lock-in demodulation | `drivers/lockin.c` | I/Q demodulation, CIC filter, IIR filter, field computation |
| ADC driver | `drivers/adc_drv.c` | ADS1256 SPI communication, DMA, channel sequencing |
| IMU driver | `drivers/imu.c` | BMI270 I2C, quaternion computation, gravity reference |
| GPS parser | `drivers/gps.c` | NMEA sentence parsing, position/velocity/time extraction |
| SD logger | `drivers/sdlog.c` | FAT32 file I/O, binary log format, ring buffer |
| BLE protocol | `drivers/ble.c` | UART framing, command/response, data streaming |
| Calibration | `drivers/calib.c` | Ellipsoid fitting, offset/scale/cross-axis correction, temp comp |
| Board config | `board.h` | Pin assignments, clock tree, peripheral mapping |
| Register defs | `registers.h` | STM32H7 peripheral register addresses and bit fields |

### 7.4 Design Decisions

1. **STM32H743 over STM32F4**: The Cortex-M7's DSP extensions (SIMD, FMA) are essential for real-time lock-in demodulation of three channels at 30 kSPS. The 480 MHz clock and 1 MB SRAM provide ample headroom for the digital filters and quaternion math.

2. **ADS1256 over internal ADC**: The STM32's internal ADC (16-bit) lacks the resolution for 0.1 nT field detection. The ADS1256's 24-bit delta-sigma architecture provides the dynamic range (~21 effective bits) needed for the fluxgate's second-harmonic signal.

3. **Software lock-in over hardware**: A hardware analog lock-in (AD630) would simplify firmware but add cost and reduce flexibility. The software implementation allows arbitrary filter bandwidths, phase correction, and multi-axis synchronization at no additional BOM cost.

4. **Closed-loop feedback via DAC**: The DAC8568 drives nulling coils to cancel the external field, keeping the fluxgate core near zero bias. This dramatically improves linearity and temperature stability at the cost of one extra SPI device.

5. **GPS-tagged logging**: Integrating position data directly into the magnetic record eliminates the need for a separate GPS device and enables single-pass survey mapping.

6. **BMI270 IMU for orientation**: Unlike magnetometer-only designs that assume the device is held level, FerroProbe uses the IMU to compute the device orientation (roll, pitch, heading) and rotate the measured magnetic vector into a level reference frame — enabling walk-mode surveying where the device orientation changes continuously.

---

## 8. Companion Application

The FerroProbe React Native app provides four primary screens:

### 8.1 Real-Time Monitor

Displays the current 3-axis magnetic field vector as:
- Numeric readout (Bx, By, Bz, |B|, declination, inclination) with 0.1 nT resolution
- 3D vector visualization (rotating cube showing the field direction relative to the device)
- Rolling time-series strip chart (selectable axis, 60-second window)
- Device status (battery, GPS fix, temperature, sample rate)

### 8.2 Survey Map

A GPS-tagged magnetic field map showing:
- Map view (Apple Maps/Google Maps) with color-coded data points overlaid
- Anomaly highlighting (points exceeding user-defined threshold)
- Track recording with start/stop/pause controls
- Post-survey gradient analysis (horizontal gradient estimation from adjacent points)

### 8.3 Calibration

Guided 3-axis calibration procedure:
- "Figure-8" motion prompt with real-time progress indicator
- Ellipsoid fitting visualization (3D scatter plot of raw Bx/By/Bz measurements)
- Computed offset/scale/cross-axis matrix display
- Temperature compensation data collection (optional)

### 8.4 Settings & Data

- Sample rate selection (1/10/100 Hz)
- Filter bandwidth configuration
- Anomaly threshold setting
- Data export (CSV/JSON to phone storage or cloud)
- Firmware version check and OTA update (via BLE DFU)

---

## 9. Calibration & Orientation Compensation

### 9.1 Ellipsoid Calibration

In an ideal magnetometer, the raw readings trace a sphere when the device is rotated in a uniform field. Real fluxgates have:
- **Offset** (electronic bias from the sense coil preamplifier)
- **Scale mismatch** (different sensitivities per axis)
- **Non-orthogonality** (axes not perfectly perpendicular)
- **Soft-iron distortion** (sensor head materials bending the field)

These combine to produce an **ellipsoid** rather than a sphere. FerroProbe performs a least-squares ellipsoid fit on ~1000 samples collected during a guided "figure-8" calibration motion, solving for the 9-parameter correction (3 offset, 3 scale, 3 cross-axis) that maps the ellipsoid to a unit sphere.

### 9.2 Orientation Compensation

The IMU provides roll (φ) and pitch (θ) via accelerometer gravity reference, and heading (ψ) via gyro integration (magnetometer-free). The measured magnetic vector **B_meas** (in sensor frame) is rotated to the Earth-level frame using the rotation matrix R(φ, θ, ψ):

**B_level** = R(φ, θ, ψ) · **B_meas**

This ensures that the magnetic vector is reported in a consistent frame regardless of how the device is held, which is essential for walk-mode surveying where the operator's stride causes continuous orientation changes.

### 9.3 Temperature Compensation

The fluxgate sensor's sensitivity varies with temperature (~0.01%/°C for mu-metal). An on-board temperature sensor (ADS1256 AIN3, fed by an LM35) provides temperature data for a polynomial correction (2nd-order: a₀ + a₁·T + a₂·T²). The coefficients are determined during a temperature sweep calibration (-20°C to +55°C) in a known field.

---

## 10. Use Cases & Target Audience

### 10.1 Archaeological Prospecting

Fired clay (pottery, hearths, kilns) has an enhanced thermoremanent magnetization that creates local magnetic anomalies of 5–50 nT. Buried metal objects produce dipole anomalies of 100 nT to 100,000 nT. By walking a systematic grid with FerroProbe in survey mode, archaeologists can map subsurface features before excavation — identifying hearth locations, building foundations, and metal artifacts without disturbing the site.

### 10.2 Mineral Exploration

Magnetic susceptibility variations between rock types (e.g., magnetite-bearing ores vs. surrounding host rock) produce anomalies detectable at the surface. FerroProbe's 0.1 nT resolution and GPS-tagged logging make it suitable for grassroots magnetic surveys to identify potential ore bodies and map geological structures.

### 10.3 UXO Detection

Unexploded ordnance (bombs, shells, mines) has a strong ferromagnetic signature. FerroProbe's real-time anomaly detection with audio alerts enables rapid area clearance. The GPS-tagged data provides a permanent record for quality assurance and re-survey verification.

### 10.4 Pipeline Inspection

Steel pipelines create magnetic anomalies that change with wall thickness, mechanical damage, and corrosion. By surveying along a pipeline route, operators can detect anomalous magnetic signatures indicative of pipe condition changes.

### 10.5 Space Science & Education

Fluxgate magnetometers are standard instruments on spacecraft for magnetic field mapping. FerroProbe serves as a low-cost educational platform for teaching magnetometry principles, and the open firmware allows adaptation for CubeSat and high-altitude balloon missions.

### 10.6 Geomagnetic Monitoring

The Earth's magnetic field varies diurnally (by 10–100 nT) due to ionospheric currents, and during magnetic storms by 1000+ nT. FerroProbe in precision mode (1 Hz, 0.1 nT) can serve as a low-cost geomagnetic observatory node for space weather monitoring networks.

### 10.7 Target Audience

| Audience | Use Case |
|---|---|
| Archaeologists | Pre-excavation site surveying |
| Geophysicists | Mineral exploration, structural mapping |
| UXO clearance teams | Rapid area clearance with GPS-tagged records |
| Pipeline operators | Route inspection and condition monitoring |
| University researchers | Teaching, CubeSat/balloon missions, instrument development |
| Citizen scientists | Community geomagnetic monitoring networks |
| Museums & heritage organizations | Non-invasive site assessment |

---

## 11. Directory Structure

```
ferroprobe/
├── README.md                    ← This document
├── firmware/
│   ├── main.c                    ← Main loop, state machine, initialization
│   ├── board.h                   ← Pin assignments, clock config, constants
│   ├── registers.h               ← STM32H7 register definitions
│   ├── Makefile                   ← ARM GCC build system
│   └── drivers/
│       ├── fluxgate.c/.h          ← Excitation PWM, H-bridge, core saturation
│       ├── lockin.c/.h            ← Digital lock-in demodulation (I/Q, CIC, IIR)
│       ├── adc_drv.c/.h           ← ADS1256 24-bit ADC SPI driver
│       ├── imu.c/.h               ← BMI270 IMU, quaternion, gravity reference
│       ├── gps.c/.h               ← NEO-M9N NMEA parser
│       ├── sdlog.c/.h             ← FAT32 SD card logger
│       ├── ble.c/.h               ← BLE UART protocol
│       └── calib.c/.h             ← Ellipsoid fit, orientation comp, temp comp
├── kicad/
│   ├── device.kicad_pro          ← KiCad project file
│   ├── device.kicad_sch           ← Schematic with real components and nets
│   └── device.kicad_pcb           ← PCB layout with placed footprints
└── app/
    ├── App.js                     ← React Native entry point
    ├── package.json               ← Dependencies and configuration
    ├── screens/
    │   ├── MonitorScreen.js       ← Real-time 3-axis field display
    │   ├── SurveyMapScreen.js     ← GPS-tagged magnetic survey map
    │   ├── CalibrationScreen.js   ← Guided 3-axis calibration
    │   └── SettingsScreen.js      ← Device configuration and data export
    ├── components/
    │   ├── FieldVector3D.js       ← 3D vector visualization
    │   ├── StripChart.js           ← Time-series strip chart
    │   └── StatusBar.js            ← Device status bar
    └── utils/
        └── protocol.js            ← BLE protocol parser and command builder
```

---

## License

| Component | License |
|---|---|
| Hardware design (KiCad schematics/PCB) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

**Author: jayis1** — All designs, firmware, code, and documentation by jayis1.
Copyright © 2026 jayis1. All rights reserved.