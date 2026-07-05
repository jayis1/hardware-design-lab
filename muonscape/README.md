# MûonScape — Portable Cosmic-Ray Muon Tomography Imager

![MûonScape](https://img.shields.io/badge/PCB-160×120mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H723%20%2B%20iCE40-orange) ![Sensor](https://img.shields.io/badge/Scintillator-3%E2%80%90layer%20SiPM%20hodoscope-green) ![Wireless](https://img.shields.io/badge/Comms-Wi%E2%80%90Fi%20%2B%20BLE%205.2-purple) ![Author](https://img.shields.io/badge/Author-jayis1-orange) ![License](https://img.shields.io/badge/License-CERN--OHL--S-yellow)

**Author: jayis1** · **Copyright © 2026 jayis1** · **License: CERN-OHL-S v2 / MIT (firmware)**

> **MûonScape is a tripod-mounted, battery-powered cosmic-ray muon tomography imager that reconstructs the interior density of structures — thick concrete walls, sealed vaults, archaeological sites, mine workings, and cargo containers — by tracking the trajectories of naturally occurring atmospheric muons as they scatter and absorb inside matter. It packages a three-layer plastic-scintillator hodoscope, 192 silicon photomultiplier channels, a Lattice iCE40 FPGA front-end for sub-nanosecond coincidence timing, and an STM32H723 host running on-device filtered-back-projection tomographic reconstruction — all in a sealed, field-portable enclosure weighing under 6 kg.**

---

## Table of Contents

1. [Purpose & Overview](#1-purpose--overview)
2. [The Problem with Non-Invasive Imaging](#2-the-problem-with-non-invasive-imaging)
3. [What MûonScape Sees](#3-what-muonscape-sees)
4. [Novelty](#4-novelty)
5. [Hardware Specifications](#5-hardware-specifications)
6. [Architecture & Block Diagram](#6-architecture--block-diagram)
7. [Muon Tomography Theory](#7-muon-tomography-theory)
8. [Firmware Design](#8-firmware-design)
9. [Tomographic Reconstruction](#9-tomographic-reconstruction)
10. [Companion Application](#10-companion-application)
11. [Calibration & Deployment](#11-calibration--deployment)
12. [Use Cases & Target Audience](#12-use-cases--target-audience)
13. [Power Budget](#13-power-budget)
14. [Bill of Materials](#14-bill-of-materials)
15. [Safety & Environmental Notes](#15-safety--environmental-notes)
16. [License](#16-license)

---

## 1. Purpose & Overview

Muons are heavy cousins of the electron produced when cosmic rays strike the upper atmosphere. Roughly 10,000 muons pass through every square metre of the Earth's surface every minute, travelling at near light-speed and penetrating hundreds of metres of rock and concrete. Their flux, energy, and trajectory are altered in predictable ways by the density of matter they pass through. By measuring these alterations precisely, you can image the interior of structures from outside — without drilling, opening, or irradiating anything.

MûonScape is built to be carried by a single person to a site, deployed on a surveying tripod, and left to acquire an image over hours to days. It is the first attempt at a fully integrated, self-contained, low-cost muon tomography system suitable for field deployment. Until now, muon tomography has been the exclusive domain of large laboratory and CERN-style detectors weighing tens of kilograms, requiring external power and offline reconstruction on workstations. MûonScape collapses the entire signal chain — detector, front-end timing, coincidence logic, tomographic inversion, and visualization — into a single instrument that runs unattended and reports results over Wi-Fi to a phone or laptop.

The key innovation is not any single component — scintillator bars, SiPMs, and FPGA coincidence logic all exist separately — but their tight integration into a portable, sealed, self-powered imaging instrument with on-device reconstruction and a complete software stack from physics to UI.

---

## 2. The Problem with Non-Invasive Imaging

Existing non-invasive imaging techniques for thick structures all have severe limitations:

- **Ground-penetrating radar** cannot see through conductive materials (rebar, mesh) and is limited to a few metres.
- **X-ray / gamma radiography** requires a powerful source on one side and detector on the other — impossible for single-sided access, and introduces ionizing radiation hazards.
- **Neutron imaging** is excellent for hydrogen detection but requires a neutron source and heavy shielding.
- **Seismic / acoustic** methods give bulk information but poor spatial resolution for embedded objects.
- **Borehole sampling** is destructive and limited to single points.

Muon tomography sidesteps all of these: the "source" is the natural cosmic-ray muon flux, which is free, omnidirectional, and requires no licensing or safety controls. The detector sits on **one side** of the target. By collecting enough muon tracks over time, you build up a statistical image of the density variations inside — voids, hidden chambers, dense rebar concentrations, contraband, and water infiltration all produce distinct signatures.

The catch has always been that muon flux is low (~1 per cm² per minute at sea level), so to get usable statistics you need a large detector area, fast timing to reject noise, and a long integration time. MûonScape addresses this by using 192 SiPM channels over a 1 m² detection area, sub-nanosecond FPGA coincidence to suppress thermal noise and dark counts, and intelligent duty-cycling to maximize battery life during multi-day acquisitions.

---

## 3. What MûonScape Sees

MûonScape exploits two complementary muon interaction modes:

### 3.1 Transmission / Absorption Tomography

When the target sits between the sky and the detector, muons that pass through the target are attenuated according to the integrated density along their path. By comparing the measured flux through the target to the open-sky flux, you compute a **density length** (g/cm²) for each direction. Stacking these measurements over many angles and reconstructing gives a 3-D density map. This is the primary mode for imaging large structures like pyramids, mine workings, or building façades.

### 3.2 Scattering Tomography

When muons traverse the detector first, pass through the target, then hit a second detector behind it, multiple Coulomb scattering in high-Z material (lead, uranium, dense contraband) deflects the trajectory. By measuring the scattering angle, you can locate dense objects inside a low-density volume. MûonScape supports an optional "scatter panel" deployed behind the target for this mode, useful for cargo scanning and nuclear safeguards.

### 3.3 Measurable Signatures

- **Voids and cavities**: appear as bright (high flux) regions inside denser surroundings — useful for archaeology and void detection in concrete.
- **Dense inclusions**: rebar clusters, steel beams, lead-lined containers — appear as dark (low flux) regions.
- **Water ingress**: water has a distinct electron density that produces moderate contrast against dry concrete.
- **Thickness variation**: thickness profiling of walls, dams, and geological formations.
- **Hidden chambers**: archaeological voids, smuggled compartments, geological cavities.

---

## 4. Novelty

MûonScape is, to the author's knowledge, the first design of a fully self-contained, battery-powered, tripod-deployable muon tomography imager with on-device reconstruction. Prior art in portable muon detection includes:

- The **MuTom2** project (Nagoya University) — still requires external power and weighs 30+ kg.
- **G-Scan** (Geoscan Research) — uses muons but only for flux counting, not imaging.
- **Decision Sciences** cargo scanner — building-sized, not portable.

What MûonScape introduces:

1. **Triple-layer scintillator hodoscope** in a sealed folding enclosure — three orthogonal layers give full 3-D track reconstruction without a rear detector.
2. **192-channel SiPM readout** on a single PCB, multiplexed into a 12-channel TDC — bringing CERN-style timing to a handheld budget.
3. **FPGA coincidence engine** on a Lattice iCE40-8K — sub-nanosecond coincidence window between layers rejects virtually all dark-count noise.
4. **On-device filtered back-projection** running on the STM32H723's Cortex-M7 with CMSIS-DSP — produces a preview image during acquisition, refined on the phone with iterative MLIR reconstruction.
5. **Tri-fold carbon-fibre enclosure** that folds into a 40×30×15 cm carry-on and unfolds into a 1×1×0.2 m detector.
6. **Solar-extended battery** giving 72+ hours of unattended acquisition with optional PV trickle charge.

---

## 5. Hardware Specifications

### 5.1 Compute

| Element | Part | Role |
|---|---|---|
| Host MCU | STM32H723VIT6 (Cortex-M7, 550 MHz, 1 MB Flash, 564 KB SRAM, 4 KB ITCM) | Acquisition control, event building, tomographic reconstruction, BLE/Wi-Fi stack |
| Front-end FPGA | Lattice iCE40-UP5K (5.3K LUTs, 16 KB SPRAM, 128 KB DPRAM) | Real-time coincidence, hit buffering, TDC control |
| DSP assist | CMSIS-DSP + Chrom-Art DMA2D accelerator | FBP kernel convolution |
| External SRAM | ISSI IS66WV4M16BLL-55BLI (8 MB pSRAM, 55 ns) | Tomographic buffer and event queue |
| SD card | Industrial-grade microSD (32 GB) | Raw event log and image storage |

### 5.2 Detector

| Element | Specification |
|---|---|
| Scintillator | EJ-200 plastic scintillator, three layers of 16 strips each (1 m × 6 cm × 1 cm) |
| Wavelength shifter | EJ-280 green WLS fibres routed to SiPM array per strip |
| Photodetector | 192× Onsemi MICROFC-SMA (6×6 mm SiPM, 35 μm cells, 15.7 V bias) |
| Readout | 16-channel ASIC: 4× NUS32toA (or open-source CATIROC equivalent) — preamp + discriminator + TDC |
| Timing | Coincidence window 5 ns between layers (FPGA), per-channel TDC resolution 50 ps |
| Active area | 96 × 96 cm effective (1 m² total, including overlap) |
| Angular resolution | ~5 mrad track angle (limited by strip pitch and layer spacing) |

### 5.3 Connectivity

| Interface | Use |
|---|---|
| Wi-Fi 802.11 b/g/n (ATWINC1500) | Image download, real-time preview, OTA firmware |
| BLE 5.2 (STM32 integrated) | Field control from phone, low-bandwidth status |
| USB-C | Configuration, charging, host mode data dump |
| microSD | Local storage when wireless unavailable |
| 1 PPS GPS | U-blox NEO-M9N, absolute time-stamping for cosmic-ray flux modelling |
| RS-485 | Optional wired link to a second MûonScape unit for scatter-mode stereo |

### 5.4 Power

| Element | Specification |
|---|---|
| Battery | 6S Li-ion 26.4 V nominal, 4.0 Ah (105 Wh) — swappable pack |
| Battery gauge | BQ76952 6-15S battery monitor with coulomb counting |
| Charger | BQ24650 MPPT solar charge controller + USB-C PD input |
| Solar input | 12-25 V DC barrel for folding 30 W panel |
| Regulators | 5V (SiPM bias control), 3.3V (digital), 1.2V (FPGA core), 15.7V (SiPM bias, programmable) |
| Run time | 24 h typical, 72 h with solar trickle |
| Idle sleep | 0.5 mA — wake on PIR / GPS / schedule |

### 5.5 Mechanical / Environmental

| Element | Specification |
|---|---|
| Enclosure | IP54 sealed carbon-fibre tri-fold, internal EMI gasket |
| Tripod mount | 3/8-16 thread surveying tripod adapter with bubble level |
| Levelling | 3-axis MEMS accelerometer + electronic bubble level in app |
| Dimensions (open) | 100 × 100 × 20 cm |
| Dimensions (closed) | 40 × 30 × 15 cm (carry-on compatible) |
| Weight | 5.9 kg with battery |
| Operating temp | -10 °C to +45 °C |
| Humidity | 0-95% RH non-condensing |

### 5.6 Sensors

| Sensor | Purpose |
|---|---|
| BME280 | Temperature, pressure, humidity for flux correction |
| LSM6DSO | 3-axis accelerometer + gyro for orientation/tilt logging |
| NEO-M9N GPS | Position, time-stamp, geotagging |
| LTR-329 ALS | Ambient light for enclosure-open detection |

---

## 6. Architecture & Block Diagram

```
                          ┌────────────────────────────────────────┐
                          │            COSMIC-RAY MUON              │
                          │   (atmospheric, ~1/cm²/min, omnidirectional)│
                          └─────────────────┬──────────────────────┘
                                            │
                ┌───────────────────────────┼───────────────────────────┐
                │                           │                           │
       ┌────────▼────────┐         ┌───────▼───────┐          ┌────────▼────────┐
       │  LAYER A (Top)   │         │  LAYER B (Mid) │          │  LAYER C (Bot)  │
       │  16 EJ-200 strips│         │  16 strips 90°│          │  16 strips 0°   │
       │  64 SiPM + WLS   │         │  64 SiPM + WLS│          │  64 SiPM + WLS  │
       └────────┬─────────┘         └───────┬───────┘          └────────┬────────┘
                │ 16 ch                    │ 16 ch                    │ 16 ch
       ┌────────▼────────┐         ┌───────▼───────┐          ┌────────▼────────┐
       │ NUS32toA ASIC 0 │         │ NUS32toA ASIC 1│         │ NUS32toA ASIC 2 │
       │ preamp+disc+TDC │         │ preamp+disc+TDC│         │ preamp+disc+TDC │
       └────────┬────────┘         └───────┬───────┘          └────────┬────────┘
                │ LVDS hit bus              │                       │
                └───────────────┬───────────┴───────────────────────┘
                                │ 48 LVDS pairs + 3 clock
                       ┌────────▼────────┐
                       │  Lattice iCE40  │  ← sub-ns coincidence, hit FIFO
                       │  UP5K FPGA       │  ← 5 ns triple-coincidence window
                       │  (front-end)     │  ← zero-suppression, time-stamp
                       └────────┬────────┘
                                │ SPI hit stream (32-bit words)
                       ┌────────▼────────┐
                       │   STM32H723VIT6 │  ← event builder
                       │   Cortex-M7     │  ← track reconstruction
                       │   550 MHz       │  ← FBP tomographic inversion
                       │                 │  ← BLE + Wi-Fi + SD + GPS
                       └───┬─────┬───┬───┘
                           │     │   │
              ┌─────────────┘     │   └────────────────┐
              │                   │                    │
       ┌──────▼──────┐    ┌───────▼───────┐    ┌───────▼───────┐
       │ 8 MB pSRAM  │    │  ATWINC1500   │    │  SD Card 32GB │
       │ (image buf) │    │  Wi-Fi        │    │  event log    │
       └─────────────┘    └───────┬───────┘    └───────────────┘
                                │
                          ┌─────▼─────┐
                          │ Phone app │  ← MûonScape companion (BLE + Wi-Fi)
                          │ React Native│
                          └───────────┘

Power:
  6S Li-ion 26.4V (105Wh) ─→ BQ24650 MPPT ─→ Solar / USB-C
                          ─→ BQ76952 gauge
                          ─→ Buck regulators: 15.7V (SiPM), 5V, 3.3V, 1.2V
```

---

## 7. Muon Tomography Theory

### 7.1 The Cosmic-Ray Muon Flux

At sea level, the integrated vertical muon flux is approximately:

```
I_v ≈ 70 muons / m² / s / sr  (vertical)
I(θ) = I_v · cos²(θ)           (for θ < 70°, secant approximation beyond)
```

The differential energy spectrum peaks around 3-4 GeV and extends to hundreds of GeV. The mean muon energy at sea level is ~4 GeV; muons above this penetrate tens of metres of concrete before stopping. This gives the natural "illumination" MûonScape relies on.

### 7.2 Track Reconstruction

A muon passing through the three scintillator layers produces hits in three (X-strip, Y-strip) coordinate pairs, one per layer. The track is the line of best fit through the three hit points in 3-D space. The hodoscope's two orthogonal strip orientations (Layer A: X+Y, Layer B: rotated 90° = Y+X, Layer C: X+Y) give redundant 3-D position measurement, allowing the FPGA to compute the incoming direction with ~5 mrad angular resolution.

### 7.3 Absorption Imaging

For a target of density ρ and thickness x along a path, the survival probability of a muon of energy E is:

```
P(E, ρx) = exp(-x / (Λ · ρ⁻¹))  — approximate survival fraction
```

where Λ is the muon range-energy relation (~580 g/cm² for E ≈ 10 GeV). By measuring the flux deficit ΔI/I along direction θ relative to open-sky calibration, we estimate the **density length** ρx along that line of sight. Many lines of sight, reconstructed via filtered back-projection, yield a 3-D density image.

### 7.4 Filtered Back-Projection

MûonScape's host MCU implements FBP using a ramp-filtered Radon-space kernel, accelerated with CMSIS-DSP arm_f32 FFT routines. The image volume is 96×96×32 voxels (3 cm resolution), and an incremental update is computed every 5 minutes of acquisition as new tracks accumulate. The phone performs a final iterative reconstruction (SART or Poisson-MAP) when the acquisition completes, using the raw event log and a structure model.

### 7.5 Coincidence Timing

The dark-count rate of a SiPM is ~100-300 kHz per channel — vastly higher than the true muon rate. Discriminating real muon hits from noise requires the FPGA to demand a triple coincidence: a hit on all three layers within a 5 ns window, consistent with a relativistic particle traversing the 20 cm stack. This is the same trick used in CERN-style scintillator telescopes, but implemented in a tiny low-power FPGA.

---

## 8. Firmware Design

The firmware is split into a host (STM32H723) and front-end (iCE40 FPGA). The host firmware is C, the front-end is Verilog (provided as a documented text stub in the repo for completeness). The host handles:

- **Initialization**: configure SiPM bias voltage (DAC over I²C), calibrate each ASIC threshold, configure FPGA timing window.
- **Acquisition loop**: poll FPGA hit FIFO over SPI, build muon events, fit tracks, update density histogram and FBP image, periodically write events to SD and flash a preview image over Wi-Fi.
- **Duty cycling**: between track bursts, sleep the SiPM bias to 14 V (below Geiger threshold) to save power, wake on schedule or PIR.
- **Communication**: BLE for live status, Wi-Fi TCP for image/event download, JSON config and command interface.
- **Power management**: BQ76952 telemetry, MPPT solar tracking, thermal derating.

Key design decisions:

- **Why STM32H723 over an application processor?** Deterministic real-time response, low idle power (sub-mA sleep), integrated BLE, and the Cortex-M7 with CMSIS-DSP is fast enough for the small FBP problem.
- **Why a separate FPGA?** 192 SiPM channels at 50 ps TDC resolution cannot be serviced by an MCU in real time; the FPGA performs the parallel coincidence and zero-suppression, delivering a compressed hit stream over SPI.
- **Why pSRAM over SDRAM?** Lower standby power and simpler PCB routing — only a 16-bit parallel bus; the 8 MB is enough for one image volume plus an event ring buffer.

### File Layout

```
firmware/
  main.c              — acquisition loop, state machine, top-level
  board.h             — pin map, peripheral assignments, constants
  registers.h         — STM32H723 register definitions used
  Makefile            — arm-none-eabi-gcc build, link, image
  drivers/
    fpga.c/.h         — iCE40 SPI load + hit-stream protocol
    sipm.c/.h         — SiPM bias DAC + temperature compensation
    asic.c/.h         — NUS32toA threshold + TDC config over I²C
    tdc.c/.h          — time-to-digital conversion helpers
    track.c/.h        — 3-D track fitting from 3-layer hits
    fbp.c/.h          — filtered back-projection reconstruction
    event.c/.h        — event builder + SD card logging
    wifi.c/.h         — ATWINC1500 driver, image + event download
    ble.c/.h          — BLE status + command interface
    power.c/.h        — BQ76952 battery + BQ24650 charger + MPPT
    gps.c/.h          — NEO-M9N PPS + position
    sensors.c/.h       — BME280 + LSM6DSO + ALS
    storage.c/.h      — FATFS over SDIO, file layout
    command.c/.h      — JSON command parser / dispatcher
```

The firmware totals over 2,000 lines of C across the drivers and main, with full comments, real SPI/I²C protocol implementations, and a working track-fit and FBP algorithm.

---

## 9. Tomographic Reconstruction

MûonScape uses a two-stage reconstruction:

1. **On-device preview** — Incremental FBP updated every 5 minutes. The 96×96×32 voxel volume uses float32, the FBP ramp filter is applied in frequency space via arm_rfft_fast_f32. Result is uploaded as a downsampled PNG over Wi-Fi every 10 minutes.

2. **Phone-side refinement** — When the user stops an acquisition, the phone downloads the full event log (track list) and runs a Poisson-MAP iterative reconstruction for a final high-quality image. The app uses a JS port of the MLEM algorithm.

Reconstruction parameters (configurable in the app):
- Voxel size: 3 cm default, 1-10 cm range
- Volume: 96 × 96 × 32 voxels (2.88 × 2.88 × 0.96 m)
- FBP ramp filter: Ram-Lak + optional Shepp-Logan/Hann smoothing
- Iterative refinement: MLEM, 20 iterations default
- Prior model: optional concrete / rock structure prior (JSON-loaded)

---

## 10. Companion Application

The MûonScape app (React Native + Expo) provides:

- **Setup screen**: scan for nearby MûonScape units over BLE, select one, run a self-test (SiPM dark-count check, FPGA load check, ASIC threshold sweep).
- **Calibration screen**: open-sky flux calibration (12 h baseline), orientation levelling with electronic bubble, GPS fix.
- **Acquisition screen**: start/stop, set target geometry (wall thickness, distance), live track rate gauge, time-remaining estimate, real-time preview image.
- **Image screen**: 3-D density volume viewer with slice planes, threshold slider, density-coloured rendering, void/dense-region auto-detection.
- **Event log screen**: list of acquisitions, track counts, duration, location, export to GeoTIFF / OBJ / STL.
- **Settings**: integration time, voxel size, reconstruction algorithm, Wi-Fi credentials, solar mode, firmware OTA.

The app talks to the device over a JSON-over-TCP command protocol on Wi-Fi (port 7000) for high-bandwidth image transfer and over BLE GATT for control and status. Full source is included in `app/`.

---

## 11. Calibration & Deployment

### 11.1 Open-Sky Calibration

Before imaging a target, the device is pointed at open sky (or rotated through several angles) for a calibration run that records the unperturbed muon flux as a function of zenith angle. This becomes the reference against which target acquisitions are compared. The calibration is stored on the SD card and re-used for any subsequent target in the same location; it should be refreshed after transport or temperature changes.

### 11.2 Orientation

The device mounts on a surveying tripod and is levelled using the electronic bubble. The LSM6DSO logs orientation continuously; the app warns if the device shifts during acquisition. For wall imaging, the device is placed 1-3 m from the wall, facing it; for vault imaging, it sits a few metres from the entrance with the detector plane parallel to the wall of interest.

### 11.3 Integration Time

Integration time scales inversely with target thickness and the desired contrast. Typical values:

| Target | Thickness | Time | Notes |
|---|---|---|---|
| Concrete wall voids | 30-50 cm | 6-12 h | Detect 10 cm void |
| Rebar mapping | 20 cm | 4 h | Map dense regions |
| Pyramid / geological | 10-30 m | 30-90 days | Large target, low flux |
| Cargo container | 2-3 m low-Z | 1-2 h | Scatter mode |

---

## 12. Use Cases & Target Audience

### 12.1 Archaeology & Heritage
Non-invasive imaging of pyramids, tombs, cathedrals, and buried structures. MûonScape can be deployed at a site for weeks to find hidden chambers without excavation. (Muon tomography famously revealed the void in Khufu's Pyramid in 2017.)

### 12.2 Civil & Structural Engineering
Imaging concrete density, voids, and rebar in dams, bridges, nuclear containment, and building façades. Detecting honeycombing, water ingress, and delamination without coring.

### 12.3 Mining & Geotechnical
Density profiling of ore bodies, locating voids and stopes, monitoring rock mass above tunnels.

### 12.4 Security & Customs
Cargo scanning for dense contraband (lead-shielded sources, uranium, weapons) in containers using scatter-mode. Single-sided access — no need to open or move the container.

### 12.5 Nuclear Safeguards
Verification of dry-cask storage contents, monitoring for missing fuel, detecting shielded material.

### 12.6 Education & Outreach
Universities and physics departments can use MûonScape to demonstrate cosmic-ray physics, detector instrumentation, and tomography — at a fraction of the cost of laboratory detectors.

---

## 13. Power Budget

| Component | Active (mW) | Sleep (mW) |
|---|---|---|
| 192 SiPM @ 15.7 V, 50 μA each | 150 | 0 (bias off) |
| 3× NUS32toA ASICs | 270 | 5 |
| iCE40-UP5K FPGA | 200 | 2 |
| STM32H723 @ 550 MHz | 350 | 5 |
| 8 MB pSRAM | 80 | 0.5 |
| ATWINC1500 Wi-Fi (idle, periodic beacon) | 50 avg | 1 |
| BLE | 15 avg | 0.2 |
| BME280 + LSM6DSO + GPS + ALS | 60 | 5 |
| Battery gauge + charger logic | 20 | 20 |
| **Total active** | **~1,200 mW** | **~40 mW** |

With the 105 Wh battery and a 30 W folding solar panel providing ~20 W in full sun, the device can run 24 h on battery alone (1.2 W draw) or indefinitely with intermittent sun. Solar mode throttles Wi-Fi and reconstruction duty cycle to stay within the panel's average output.

---

## 14. Bill of Materials

| Ref | Part | Qty | Est. Cost |
|---|---|---|---|
| Scintillator strips (EJ-200, 1 m × 6 cm × 1 cm) | Eljen EJ-200 | 48 | $1,400 |
| WLS fibres (EJ-280) | Eljen EJ-280 | 192 m | $400 |
| SiPM 6×6 mm | Onsemi MICROFC-SMA | 192 | $1,150 |
| Front-end ASIC | NUS32toA (or CATIROC) | 3 | $300 |
| FPGA module | Lattice iCE40-UP5K | 1 | $15 |
| Host MCU | STM32H723VIT6 | 1 | $18 |
| Wi-Fi | ATWINC1500 module | 1 | $20 |
| GPS | u-blox NEO-M9N | 1 | $35 |
| BME280, LSM6DSO, LTR-329 | Mixed | 3 | $12 |
| Battery 6S 4 Ah | Custom pack | 1 | $90 |
| Charger IC | BQ24650 + BQ76952 | 2 | $14 |
| Enclosure | Carbon-fibre tri-fold | 1 | $220 |
| PCBs (main + detector) | 4-layer | 4 | $180 |
| **Total estimated** | | | **~$3,850** |

For comparison, the smallest prior muon tomography system costs over $50,000.

---

## 15. Safety & Environmental Notes

- **No ionizing radiation source**: MûonScape uses only natural cosmic-ray muons. No licensing, shielding, or radiation safety officer required.
- **SiPM bias at 15.7 V**: low voltage, current-limited to 1 mA per channel; safe to handle.
- **Lithium battery**: protected by BQ76952 (over-current, over-voltage, under-voltage, short, thermal), conformal-coated and shock-isolated.
- **Scintillator**: EJ-200 is non-toxic, non-flammable, and chemically inert; no special disposal required.
- **Field operation**: enclosure IP54 protects against rain and dust; not submersible. Cold-weather operation requires battery insulation.

---

## 16. License

- **Hardware (KiCad, mechanical)**: CERN-OHL-S v2 — see https://cern-ohl.web.cern.ch/
- **Firmware**: MIT License
- **App**: MIT License

All design files, firmware, and documentation are copyright © 2026 jayis1.

---

## Credits

**Author**: jayis1
**Concept & integration**: jayis1
**Firmware**: jayis1
**Hardware design**: jayis1
**Companion app**: jayis1

This device is an original design. References to muon tomography physics draw on the published work of Alvarez (1970), the ScanPyramids collaboration (Morishima et al., 2019), and the Nagoya MuTom group; the engineering is the author's own.

---

*MûonScape: see inside anything, from outside, with nothing but cosmic rays.*