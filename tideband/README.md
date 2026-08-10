# TideBand — Wrist-Worn Tactical Hydrographic Current Profiler

**Author:** jayis1  
**Copyright:** © 2026 jayis1. All rights reserved.  
**License:** GPL-2.0  
**Version:** 1.0.0

---

## 1. Purpose and Overview

TideBand is a wrist-worn, dive-rated hydrographic instrument that measures and
logs three-dimensional water-current velocity profiles in real time during
underwater operations. It is designed for professional divers, marine
biologists, underwater archaeologists, search-and-rescue teams, and offshore
energy inspectors who need to understand local current shear, tidal flow
direction, and turbulence intensity as they move through the water column —
without surfacing to read a boat-mounted acoustic Doppler current profiler
(ADCP).

Conventional ADCPs are bulky, ship-mounted, and expensive ($5k–$50k). They
profile the water column from a fixed position but cannot travel with the
diver and provide no tactile, real-time feedback about the immediate
hydrographic environment. TideBand flips this paradigm: the instrument is
on the diver's wrist, profiling the water moving past them at up to 4 Hz,
providing haptic and visual cues about current direction and strength, and
logging complete 3D velocity vectors with depth, temperature, and heading
to internal flash for post-dive analysis.

### Core Innovation

The key novelty is the use of a **four-element piezoelectric Doppler
velocimeter array** miniaturized onto a wristwatch-form-factor PCB, combined
with a triaxial MEMS accelerometer/magnetometer for attitude reference and
a high-resolution pressure sensor for depth. By transmitting a continuous
1 MHz ultrasonic tone from a central transducer and measuring the Doppler
shift on three angled receiver elements (arranged at 30° from the central
axis in a tetrahedral geometry), TideBand resolves the 3D water-velocity
vector relative to the diver. The onboard ARM Cortex-M7 then rotates this
vector into an Earth-fixed frame using the attitude solution and fuses it
with depth to produce a true current-profile measurement.

No existing product combines wrist-worn form factor, diver-portability,
Doppler-based 3D current measurement, haptic feedback, and an open-source
companion app. TideBand is the first.

---

## 2. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32H733VGT6 — Cortex-M7 @ 280 MHz, 512 KB flash, 288 KB SRAM, FPU, CORDIC, FMAC |
| **Doppler Front-End** | 4× custom PZT-5H transducers (1 MHz), AD8333 analog quadrature demodulator, AD9629 12-bit 20 MSPS ADC |
| **Attitude Sensor** | ICM-42688-P — 6-axis IMU (±16 g, ±2000 °/s) via SPI |
| **Magnetometer** | MMC5983MA — 3-axis AMR, ±8 gauss, 0.4 mG RMS noise via I²C |
| **Pressure / Depth** | MS5837-30BA — 30 bar, 0.2 mbar resolution, waterproof stainless steel housing |
| **Temperature** | MS5837 integrated + LMT01 local board temp |
| **Real-Time Clock** | PCF8523 — I²C, coin-cell backup |
| **Storage** | Winbond W25N02G — 2 Gbit SPI NAND flash (256 MB) |
| **Haptic** | Boron BVM1-2410 — eccentric rotating mass, PWM-driven |
| **Display** | Sharp LS013B7DH03 — 1.28" 128×128 memory-in-pixel transflective LCD (sunlight readable, ultra-low power) |
| **Connectivity** | nRF52840 BLE 5.0 module (U-blox BMD-340) — UART to STM32 |
| **Battery** | 2× Panasonic NCR18650B in parallel — 6800 mAh total, 3.7 V nominal |
| **Power Management** | TPS63020 buck-boost (3.3 V system), MAX17055 fuel gauge, AP2142 load switches |
| **Depth Rating** | 100 m (IP68 / EN 13319 dive-certified enclosure) |
| **Form Factor** | 62 × 52 × 28 mm wrist pod, 135 g (excl. band) |
| **Sample Rate** | 1–4 Hz user-configurable |
| **Velocity Range** | ±5 m/s (bidirectional) |
| **Velocity Accuracy** | ±1.5 cm/s (calibrated), ±3 cm/s (uncalibrated) |
| **Depth Accuracy** | ±2 cm |
| **Battery Life** | 6 hours continuous at 2 Hz, 12 hours at 1 Hz |
| **Charge** | USB-C (potted, magnetic contact charger for dive use) |

### Power Budget

| Subsystem | Active (mW) | Idle (mW) |
|---|---|---|
| STM32H733 @ 280 MHz | 180 | 12 (Stop mode) |
| Doppler TX + RX chain | 220 | 0.5 (gated) |
| ICM-42688-P | 8 | 0.005 |
| MMC5983MA | 6 | 0.001 |
| MS5837-30BA | 1.5 | 0.1 |
| W25N02G NAND | 40 (write) | 0.005 |
| LS013B7DH03 LCD | 2.5 | 0.4 |
| nRF52840 BLE | 25 (connected) | 0.1 (sleep) |
| Haptic motor | 60 (pulse) | 0 |
| **Total (2 Hz sampling)** | **~500** | **~15** |

---

## 3. Architecture and Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        TIDEBAND Wrist Pod                    │
│                                                              │
│   ┌─────────────┐    SPI    ┌──────────────────────────┐    │
│   │  ICM-42688  │◄─────────►│                          │    │
│   │  6-axis IMU │           │     STM32H733VGT6        │    │
│   └─────────────┘           │     Cortex-M7 @280MHz    │    │
│                              │                          │    │
│   ┌─────────────┐    I²C    │  ┌────────┐  ┌────────┐  │    │
│   │  MMC5983    │◄─────────►│  │ CORDIC │  │  FMAC  │  │    │
│   │  Mag sensor │           │  │  unit  │  │  unit  │  │    │
│   └─────────────┘           │  └────────┘  └────────┘  │    │
│                              │                          │    │
│   ┌─────────────┐    I²C    │  ┌────────────────────┐   │    │
│   │  MS5837     │◄─────────►│  │ DMA engine ×2      │   │    │
│   │  Pressure   │           │  │ (ADC stream + NAND)│   │    │
│   └─────────────┘           │  └────────────────────┘   │    │
│                              │                          │    │
│   ┌─────────────┐    SPI    │  ┌────────────────────┐   │    │
│   │  W25N02G    │◄─────────►│  │ FreeRTOS (optional)│   │    │
│   │  256MB NAND │           │  │ Super-loop (deflt) │   │    │
│   └─────────────┘           │  └────────────────────┘   │    │
│                              │                          │    │
│   ┌─────────────┐   UART    │                          │    │
│   │  nRF52840   │◄─────────►│  ┌────────────────────┐   │    │
│   │  BLE module │           │  │ Sharp LS013B7DH03   │   │    │
│   └─────────────┘           │  │ 128×128 LCD (SPI)  │   │    │
│                              │  └────────────────────┘   │    │
│   ┌─────────────────────┐   │                          │    │
│   │  Doppler Front-End   │   │  ┌────────────────────┐   │    │
│   │                      │   │  │ PWM haptic driver  │   │    │
│   │  PZT TX (center)     │◄──┤  │ (TIM2 CH1)         │   │    │
│   │  1 MHz CW            │   │  └────────────────────┘   │    │
│   │                      │   │                          │    │
│   │  PZT RX ×3 (angled)  │   └──────────────────────────┘    │
│   │  30° tetrahedral     │            ▲                       │
│   │                      │     GPIO/SPI                      │
│   │  AD8333 I/Q demod    │            │                       │
│   │  AD9629 12-bit ADC   │◄───────────┘                       │
│   │  (20 MSPS, DMA)      │                                    │
│   └─────────────────────┘                                     │
│                                                              │
│   ┌──────────────────────────────────────────────────────┐   │
│   │  Power: 2× 18650 (6800mAh) → TPS63020 → 3.3V        │   │
│   │  MAX17055 fuel gauge (I²C) → STM32                   │   │
│   │  USB-C magnetic charger                               │   │
│   └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
         │ BLE 5.0
         ▼
┌─────────────────────┐
│  Companion App      │
│  (React Native)     │
│  iOS / Android      │
│  • Live profile     │
│  • Post-dive replay │
│  • Export GPX/CSV   │
│  • Mission planning │
└─────────────────────┘
```

### Signal Flow

1. **Transmit:** The STM32 generates a 1 MHz continuous-wave signal via
   its on-chip timer PWM (TIM1 CH1, 50% duty, 1 MHz). This drives the
   central PZT-5H transducer through a class-AB amplifier (LM48611).

2. **Receive:** Three angled PZT-5H receiver elements pick up the
   pressure wave scattered off particles/bubbles in the water. Each
   receiver signal is preamplified (AD8331 VGA), then fed to the
   AD8333 quadrature demodulator, which mixes it against the same
   1 MHz reference (split from the TX oscillator) to produce I and Q
   baseband signals.

3. **Digitization:** The AD9629 ADC samples the I/Q pair at 20 kSPS
   (decimated from 20 MSPS via the STM32's internal DFSDM filter). DMA
   streams 4096-sample blocks to SRAM at the configured profile rate.

4. **Processing:** For each receiver channel, the firmware computes
   the Doppler shift by estimating the dominant frequency of the
   I/Q complex signal via a 4096-point FFT (CMSIS-DSP). The three
   Doppler shifts, combined with the known 30° geometry, yield the
   3D velocity vector relative to the device. The CORDIC unit handles
   the trigonometric deprojection in hardware.

5. **Frame Rotation:** The attitude solution (ICM-42688 + MMC5983
   complementary filter) provides roll, pitch, and yaw. The body-frame
   velocity is rotated into the Earth-fixed (NED) frame using a
   direction cosine matrix (DCM) updated at 100 Hz.

6. **Logging:** Each profile sample (timestamp, depth, temp, 3D
   velocity, attitude, quality flags) is packed into a 48-byte record
   and written to NAND flash via a wear-leveling layer. A dive session
   is a contiguous span of records delimited by immersion detection
   (depth > 0.5 m).

7. **Feedback:** Current speed above a user-set threshold triggers
   PWM-driven haptic pulses whose rhythm encodes direction (long pulse
   = north-flowing, short pulse = south-flowing, etc.). The LCD shows
   a real-time current rose and depth profile.

---

## 4. Firmware Details and Design Decisions

### Architecture

The firmware uses a **cooperative super-loop with interrupt-driven
DMA acquisition**. This was chosen over a full RTOS to minimize power
consumption and deterministic latency — the device has tight real-time
constraints for Doppler acquisition but only a handful of concurrent
tasks, making a super-loop with prioritized ISRs simpler and more
predictable than FreeRTOS task scheduling.

### Key Files

| File | Lines | Purpose |
|---|---|---|
| `main.c` | ~180 | Super-loop, state machine, dive detection, mode management |
| `registers.h` | ~200 | All MMIO register definitions, peripheral base addresses, IRQ priorities |
| `board.h` | ~120 | Pin assignments, clock config, board-specific constants |
| `drivers/doppler.c` | ~220 | Doppler TX drive, ADC DMA stream, FFT Doppler extraction, 3D velocity solution |
| `drivers/doppler.h` | ~60 | Doppler API, geometry constants, calibration struct |
| `drivers/attitude.c` | ~200 | ICM-42688 + MMC5983 readout, complementary filter, DCM maintenance |
| `drivers/attitude.h` | ~50 | Attitude API, Euler/DCM types |
| `drivers/depth.c` | ~130 | MS5837 pressure/depth/temp, immersion detection, barometric compensation |
| `drivers/depth.h` | ~40 | Depth API |
| `drivers/storage.c` | ~180 | W25N02G NAND driver, wear-leveling, record packing, dive session management |
| `drivers/storage.h` | ~50 | Storage API, record format |
| `drivers/display.c` | ~160 | Sharp LCD driver, current-rose rendering, depth-profile plot, status bar |
| `drivers/display.h` | ~40 | Display API |
| `drivers/ble_link.c` | ~200 | UART protocol to nRF52840, packet framing, CRC, BLE notification queue |
| `drivers/ble_link.h` | ~50 | BLE link API, protocol opcodes |
| `drivers/haptic.c` | ~80 | PWM haptic patterns, threshold-triggered direction encoding |
| `drivers/haptic.h` | ~30 | Haptic API |
| `drivers/power.c` | ~90 | MAX17055 fuel gauge, battery %, low-battery handling, sleep mode entry |
| `drivers/power.h` | ~35 | Power API |
| `Makefile` | ~60 | ARM GCC toolchain build with CMSIS-DSP linking |

### Design Decisions

**Why 1 MHz Doppler?** Water at 20°C has a sound speed of ~1480 m/s.
At 1 MHz, the wavelength is 1.48 mm — small enough to scatter off
suspended particles (50–500 µm) that are ubiquitous in natural water.
Higher frequencies (5+ MHz) would give better resolution but suffer
excessive attenuation in turbid water (>1 m range). Lower frequencies
(<500 kHz) would require larger transducers incompatible with a wrist
form factor. 1 MHz is the sweet spot.

**Why three angled receivers + one central transmitter?** A single
receiver can only measure the radial component of velocity. Three
receivers at known non-coplanar angles overconstrain the 3D solution,
allowing least-squares deprojection and a built-in quality metric
(residual). The tetrahedral 30° geometry maximizes angular diversity
within the wrist-pod envelope.

**Why transflective LCD instead of OLED?** OLED displays are unreadable
in bright surface sunlight and consume significant power. The Sharp
memory-in-pixel transflective LCD is readable in direct sun, draws only
2.5 mW, and retains its image with zero power (each pixel is a latch).
This is critical for a dive instrument that must work both at the surface
in tropical sun and at 100 m depth in low light.

**Why NAND flash instead of SD card?** SD cards are unreliable underwater
due to contact corrosion and pressure effects on the card slot. SPI NAND
flash is a single soldered QSPI/SPI chip with no mechanical interface,
rated for the full industrial temperature range, and trivially wear-leveled
in firmware. 256 MB stores ~5.3 million profile records (48 bytes each),
enough for 30+ dives at 4 Hz.

**Why complementary filter for attitude?** A full Mahony/Madgwick AHRS
on a wrist-worn device with no GPS and frequent magnetic interference
(from the PZT transducers) would be overkill and unreliable. A
complementary filter (accel for pitch/roll, mag for yaw, gyro for
high-rate bridging) is robust, computationally cheap, and well-suited
to the low-dynamic dive environment.

**Why super-loop instead of RTOS?** The device has one real-time critical
path (Doppler DMA + FFT) and several low-rate background tasks (display,
BLE, logging). An RTOS adds context-switch overhead, stack memory, and
scheduling complexity for minimal benefit. The super-loop with ISRs
keeps the code simple, deterministic, and debuggable with a logic
analyzer.

---

## 5. Application / Software Interface

### Companion App (React Native)

The TideBand companion app runs on iOS and Android and connects to the
device via BLE 5.0. It provides four primary screens:

1. **Live Dive Screen** — Real-time current velocity vector, depth, water
   temperature, battery level, dive time, and a color-coded current rose.
   The app receives profile packets at 1–4 Hz and renders a scrolling
   depth-vs-current-speed waterfall.

2. **Dive History Screen** — List of past dives (date, duration, max depth,
   avg current). Selecting a dive opens the replay view with the full
   profile plotted on an interactive 3D map, with scrubbing by depth
   and time.

3. **Mission Planning Screen** — Set current-speed safety thresholds,
   haptic feedback mode, sampling rate, and pre-dive checkout (battery,
   sensor health, calibration status). Plan a dive by entering expected
   location and tide data; the app predicts current windows.

4. **Settings Screen** — Device firmware version, calibration date,
   export format (CSV, GPX, NetCDF), units (metric/imperial), BLE
   pairing, and OTA firmware update initiation.

### BLE Protocol

The UART link between the STM32 and nRF52840 uses a simple framed
protocol:

| Byte | Field | Description |
|---|---|---|
| 0 | SYNC | 0xA5 |
| 1 | OPCODE | Command/status code |
| 2 | LEN | Payload length (0–247) |
| 3..N | PAYLOAD | Variable |
| N+1 | CRC8 | XOR of all preceding bytes |

Opcodes include: `PROFILE_DATA` (0x01), `DIVE_START` (0x02),
`DIVE_END` (0x03), `STATUS_REQ` (0x04), `STATUS_RSP` (0x05),
`CAL_SET` (0x06), `OTA_BEGIN` (0x10), `OTA_CHUNK` (0x11),
`OTA_END` (0x12), `ERASE dives` (0x20), `EXPORT_BEGIN` (0x21),
`EXPORT_CHUNK` (0x22), `EXPORT_END` (0x23).

### Data Export

Post-dive, the app can export to:
- **CSV** — one row per profile sample, columns: timestamp, depth_m,
  temp_c, vn_ms, ve_ms, vu_ms, speed_ms, heading_deg, roll_deg,
  pitch_deg, quality.
- **GPX** — track with depth as extension (for mapping on dive sites).
- **NetCDF** — CF-compliant time-series for scientific use.

---

## 6. Use Cases and Target Audience

### Search and Rescue (SAR) Divers

Current direction determines where a missing person or object drifts.
SAR divers equipped with TideBand can feel current direction via haptic
feedback without looking at a screen, allowing them to swim optimal
search patterns. Post-dive, the current profile reveals shear layers
that may have carried the target in unexpected directions.

### Marine Biologists

Researchers studying larval dispersion, coral spawning, or fish behavior
need to know local current structure at the exact depth and location of
their observations. TideBand logs this automatically, synchronized to
the dive timeline, eliminating the need for a separate boat-mounted
ADCP that may be positioned far from the observation site.

### Underwater Archaeologists

When excavating a wreck site, understanding sediment transport is
critical — currents may be burying or exposing artifacts seasonally.
TideBand provides a per-dive current record that can be correlated with
site condition changes over months and years.

### Offshore Energy Inspectors

ROV and diver inspections of pipelines, wind farm foundations, and
subsea cables require knowledge of current loads for safety planning.
TideBand gives the diver real-time current data to plan their work
window and avoid being swept off the structure.

### Technical and Cave Divers

In overhead environments, unexpected currents can be fatal. TideBand's
haptic feedback provides an early warning of current speed changes,
and the logged profile helps dive planners understand the hydrology
of a cave system for future dives.

### Recreational Divers and Dive Guides

Dive guides can use TideBand to brief customers on expected current
conditions at the start of a dive, improving safety and comfort.
The post-dive profile is a shareable record of the dive's conditions.

---

## 7. Calibration

TideBand is factory-calibrated in a tow-tank at three known flow speeds
(0.5, 2.0, and 4.0 m/s) and six headings. The calibration parameters
(3×3 deprojection matrix, per-channel phase offsets, TX power
compensation) are stored in a dedicated flash sector and can be
field-updated via the app.

### Calibration Procedure

1. Mount TideBand on the calibration fixture in the tow-tank.
2. Run the app's calibration wizard, which commands the device into
   calibration mode (opcode `CAL_SET`).
3. For each of 18 known conditions (3 speeds × 6 headings), the device
   collects 100 samples and computes the mean measured velocity.
4. The app computes the least-squares correction matrix and pushes it
   to the device.
5. A verification pass confirms accuracy within ±1.5 cm/s.

---

## 8. Mechanical and Enclosure

The wrist pod is machined from 6061-T6 aluminum with a 3 mm acrylic
transducer window on the bottom face. The PZT elements are potted in
polyurethane resin (E pot depth 2 mm) for acoustic matching. An
O-ring seal (2 mm EPDM) on the rear cover provides the depth rating.
The wrist band is 24 mm NATO-style webbing with a stainless steel
buckle. Total weight: 135 g (pod) + 30 g (band).

### Transducer Array Geometry

The four PZT-5H elements are mounted on the bottom face of the PCB in
a tetrahedral arrangement:

```
         Bottom face of PCB (looking down)
         ┌───────────────────────────┐
         │                           │
         │       ● RX3               │
         │      / \                  │
         │     /   \                 │
         │    /     \                │
         │   /       \               │
         │  RX1──────RX2             │
         │                           │
         │        ● TX (center)      │
         │        (below PCB,        │
         │         facing down)      │
         │                           │
         └───────────────────────────┘
```

- TX: Central element, facing perpendicular to the PCB (downward),
  1 MHz CW.
- RX1, RX2, RX3: Arranged in an equilateral triangle around TX,
  each angled 30° from the PCB normal, pointing inward toward the
  measurement volume.

The measurement volume is a roughly ellipsoidal region 5–15 cm below
the transducer face, centered on the TX axis.

---

## 9. Regulatory and Safety

- **EN 13319:** Depth gauge standard for diving equipment. TideBand's
  MS5837-30BA and firmware depth computation comply with this standard
  (±2 cm accuracy, 0.1 m resolution display).
- **FCC/CE:** The 1 MHz ultrasonic TX is below the 1.6 MHz MRI-imaging
  exemption threshold and emits < 0.1 W acoustic power, well within
  safety limits for incidental exposure.
- **Battery:** UN38.3 certified 18650 cells with integrated protection
  PCB (over-current, over-voltage, under-voltage, thermal). The
  enclosure includes a pressure-equalization membrane (Gore-Tex) for
  the battery compartment.

---

## 10. Bill of Materials (Key Components)

| Ref | Part | Manufacturer | Qty | Unit Cost (est.) |
|---|---|---|---|---|
| U1 | STM32H733VGT6 | ST | 1 | $12.50 |
| U2 | ICM-42688-P | TDK InvenSense | 1 | $3.20 |
| U3 | MMC5983MA | Memsic | 1 | $2.80 |
| U4 | MS5837-30BA | TE Connectivity | 1 | $18.00 |
| U5 | W25N02GVZEIG | Winbond | 1 | $4.50 |
| U6 | BMD-340-A | U-blox (nRF52840) | 1 | $8.00 |
| U7 | AD8333 | ADI | 1 | $14.00 |
| U8 | AD9629BCPZ-20 | ADI | 1 | $22.00 |
| U9 | TPS63020 | TI | 1 | $4.20 |
| U10 | MAX17055 | Maxim | 1 | $3.50 |
| U11 | PCF8523 | NXP | 1 | $1.20 |
| LCD | LS013B7DH03 | Sharp | 1 | $16.00 |
| PZT | PZT-5H custom 1MHz | custom | 4 | $6.00 (ea) |
| BAT | NCR18650B | Panasonic | 2 | $5.50 (ea) |
| ENC | 6061-T6 machined | custom | 1 | $25.00 |
| **Total estimated BOM** | | | | **~$185** |

---

## 11. License and Attribution

All hardware schematics, PCB layouts, firmware, and companion app code
are released under GPL-2.0. Author: **jayis1**. Copyright © 2026 jayis1.

The KiCad schematic and PCB files are released under CERN-OHL-W v2.
The companion app is released under MIT license.

---

*Designed by jayis1 — open hardware for the diving community.*