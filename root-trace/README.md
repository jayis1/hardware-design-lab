# RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)**

![Device](https://img.shields.io/badge/PCB-130×95mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H743%20Cortex--M7-orange) ![AFE](https://img.shields.io/badge/AFE-AD5940%20Bioimpedance-green) ![Electrodes](https://img.shields.io/badge/Electrodes-16--ch%20ring%20probe-purple) ![Comms](https://img.shields.io/badge/Comms-BLE%205.0%20%2B%20USB--C-teal) ![Power](https://img.shields.io/badge/Power-18650%20Li--ion-red) ![Author](https://img.shields.io/badge/Author-jayis1-orange)

---

## Overview

RootTrace is a portable, battery-powered, open-source electrical impedance tomography (EIT) instrument that non-invasively images plant root systems in soil. By inserting a ring of 16 stainless-steel electrodes into the ground around a plant and injecting low-amplitude AC current through successive electrode pairs while measuring the resulting voltage distribution on all other electrodes, RootTrace reconstructs a 2D cross-sectional map of subsurface electrical impedance — revealing root biomass density, root distribution, water uptake zones, and soil moisture heterogeneity without excavating or disturbing the plant.

EIT is well-established in medical imaging (lung ventilation monitoring, geophysical surveying) but has never been packaged into a low-cost, field-portable, open-source device for root science. RootTrace fills that gap. It integrates an Analog Devices AD5940 bioimpedance analog front-end, a precision 16-electrode multiplexer network, an STM32H743 Cortex-M7 host that runs real-time on-device reconstruction using a regularized Gauss-Newton solver, a 2.4-inch OLED display for live field preview, BLE 5.0 wireless data streaming, and a React Native companion app for 3D volumetric reconstruction from multiple depth scans and longitudinal growth tracking.

The device is designed for agricultural researchers, crop breeders, soil scientists, and precision-agriculture practitioners who need to monitor root development over time — a critical capability for breeding drought-resistant cultivars, optimizing irrigation, understanding carbon sequestration in root biomass, and studying rhizosphere dynamics. Current methods for root phenotyping (minirhizotrons, root windows, excavation, MRI/CT) are either destructive, expensive, or require fixed installations. RootTrace provides a scalable, repeatable, non-destructive alternative at a fraction of the cost.

---

## Hardware Specifications

### Core Processing

| Component | Part | Role |
|-----------|------|------|
| MCU | STM32H743ZIT6 | 480 MHz Cortex-M7, 2 MB Flash, 1 MB SRAM — runs EIT acquisition loop, real-time reconstruction solver, display, BLE stack |
| Bioimpedance AFE | AD5940BCPZ | Programmable impedance measurement IC: frequency generation 0.1 Hz–200 kHz, 16-bit ADC at 800 kSPS, dual 12-bit DACs, TIA, PGA, on-chip DFT |
| Display | 2.4" OLED (SSD1327, SPI) | 128×64 grayscale, for live EIT reconstruction preview and UI |
| Storage | microSD (SPI mode) | Raw frame logging, reconstruction snapshots, time-series datasets |
| BLE | STM32WB55CGU6 co-processor | BLE 5.0 mesh-capable radio for wireless streaming to companion app |
| USB | USB-C (USB 2.0 FS) | Data download, firmware updates, power/charging |

### Analog Front-End & Electrode Network

| Subsystem | Details |
|-----------|---------|
| Excitation current | AD5940 internal 12-bit DAC → Howland voltage-to-current converter, 1 µA–500 µA programmable, 0.1 Hz–200 kHz sweep |
| Measurement | AD5940 internal PGA + 16-bit ΣΔ ADC, DFT magnitude/phase extraction on-chip, 0.1 Ω–10 MΩ range |
| Electrode switching | Four ADG725 16:1 analog multiplexers (two for excitation, two for measurement), <1 Ω on-resistance, break-before-make |
| Electrodes | 16× stainless-steel 316L rods, 4 mm diameter × 150 mm length, gold-plated contacts, spring-loaded insertion tool |
| Reference resistor | 100 Ω 0.1% precision resistor for calibrated current measurement |
| Guard/shield | Driven shield on measurement leads to reduce capacitive leakage at high frequencies |

### Environmental Sensors

| Sensor | Part | Parameter |
|--------|------|-----------|
| Soil moisture | Capacitive (FDR) | Volumetric water content 0–100%, ±3% |
| Soil temperature | DS18B20 | -10°C to +85°C, ±0.5°C |
| Ambient T/RH | SHT41 | -40°C to +125°C, ±0.2°C / ±1.8% RH |
| RTC | PCF85263A | Timestamp for longitudinal scans, 1 ppm accuracy |

### Power

| Parameter | Value |
|-----------|-------|
| Battery | 1× 18650 Li-ion, 3500 mAh, removable |
| Runtime | ~8 hours continuous scanning, ~2 weeks standby |
| Charging | USB-C PD (5V/2A), MCP73871 linear charger |
| Fuel gauge | MAX17048 (I²C, 1% accuracy) |
| Power management | TPS62740 buck (3.3V, 400 mA), load switch for electrode/AFE domain |
| Low-power modes | STOP2 mode: 12 µA; standby with RTC wake: <5 µA |

### Physical

| Parameter | Value |
|-----------|-------|
| PCB dimensions | 130 mm × 95 mm (4-layer, 2 oz Cu) |
| Enclosure | IP54 weather-resistant ABS, 180×120×60 mm |
| Electrode probe | 16-electrode ring, adjustable diameter 150–400 mm |
| Cable | 16-conductor shielded ribbon, 1.5 m, Dsub-25 connector |
| Weight | 420 g (device) + 380 g (probe) |
| Operating temp | -10°C to +50°C |

---

## Architecture

### System Block Diagram

```
 ┌─────────────────────────────────────────────────────────────────┐
 │                        RootTrace Device                          │
 │                                                                  │
 │  ┌───────────┐     SPI     ┌──────────────┐    GPIO/SPI         │
 │  │ STM32H743 │◄──────────►│   AD5940     │─────────────────┐   │
 │  │ 480MHz    │            │  Bioimp AFE  │                 │   │
 │  │ Cortex-M7 │     UART   │  DAC/ADC/DFT │    ┌──────────┐ │   │
 │  │           │◄──────────►│  PGA/TIA     │◄──►│ ADG725×4 │ │   │
 │  │  - EIT    │            └──────────────┘    │ 16:1 MUX │ │   │
 │  │  solver   │     SPI     ┌──────────────┐   │ network  │ │   │
 │  │  - UI     │───────────►│ SSD1327 OLED │   └────┬─────┘ │   │
 │  │  - FS     │     SPI     │ 2.4" display │        │       │   │
 │  │  - BLE    │───────────►└──────────────┘   ┌────▼─────┐ │   │
 │  │           │     SDIO    ┌──────────────┐   │ Dsub-25  │ │   │
 │  │           │◄──────────►│ microSD slot │   │ connector│ │   │
 │  │           │     I2C     └──────────────┘   └────┬─────┘ │   │
 │  │           │◄──────────►┌──────────────┐        │       │   │
 │  │           │     I2C     │ SHT41 T/RH   │   ┌────▼─────┐ │   │
 │  │           │◄──────────►│ DS18B20      │   │ Shielded │ │   │
 │  │           │     OW     │ PCF85263 RTC │   │ cable   │ │   │
 │  │           │◄──────────►│ MAX17048 FG  │   │ 1.5 m    │ │   │
 │  │           │     I2C    └──────────────┘   └────┬─────┘ │   │
 │  │           │     UART   ┌──────────────┐        │       │   │
 │  │           │◄──────────►│ STM32WB55    │   ┌────▼─────┐ │   │
 │  │           │            │ BLE 5.0      │   │ 16× 316L │ │   │
 │  └───────────┘            └──────────────┘   │ electrodes│ │   │
 │                                               │ in soil   │ │   │
 │  ┌───────────┐    USB-C   ┌──────────────┐   └──────────┘ │   │
 │  │ MCP73871  │◄───────────│ USB-C PD     │                  │   │
 │  │ charger   │            │ connector    │                  │   │
 │  └─────┬─────┘            └──────────────┘                  │   │
 │        │                                                    │   │
 │  ┌─────▼─────┐    TPS62740  ┌──────────────┐               │   │
 │  │ 18650 Li  │─────────────►│ 3.3V rail    │───────────────┘   │
 │  │ 3500 mAh  │    buck      │ + AFE domain │                    │
 │  └───────────┘              └──────────────┘                    │
 └─────────────────────────────────────────────────────────────────┘
                                   │ BLE 5.0
                                   ▼
                        ┌────────────────────┐
                        │  Companion App     │
                        │  (React Native)    │
                        │  - 3D volumetric   │
                        │    reconstruction  │
                        │  - Growth tracking│
                        │  - Export & share  │
                        └────────────────────┘
```

### EIT Measurement Principle

RootTrace uses the **adjacent stimulation / adjacent measurement** (also called "neighboring") protocol:

1. **Stimulate**: Current is injected through electrode pair (1,2). All 16 electrodes are stimulated in sequence of adjacent pairs: (1,2), (2,3), (3,4), ..., (16,1) — yielding 16 stimulation patterns.

2. **Measure**: For each stimulation pattern, the voltage difference is measured across all adjacent non-stimulating electrode pairs. This produces 13 measurements per stimulation (excluding the two stimulated pairs), giving 16×13 = 208 independent measurements per frame.

3. **Reconstruct**: The 208-element measurement vector is fed into a regularized Gauss-Newton inverse solver that minimizes the difference between the forward-model-predicted voltages and the measured voltages, subject to a Tikhonov regularization penalty. The solver produces a 2D conductivity distribution map (typically 576-element FEM mesh) that is rendered as a false-color image on the OLED display and streamed to the companion app.

4. **Frequency sweep**: Optionally, the excitation frequency can be swept (1 kHz, 10 kHz, 50 kHz) and conductivity/permittivity at each frequency are separated via the AD5940's on-chip DFT, enabling frequency-difference EIT (fd-EIT) to distinguish root biomass (conductive) from air-filled pores (resistive) and moisture content.

### Acquisition Timing

| Step | Duration |
|------|----------|
| Electrode multiplexer settle | 200 µs |
| AD5940 frequency sweep + DFT (4 freq × 4 cycles) | 8 ms |
| ADC settling + measurement | 2 ms |
| Per-stimulation frame | ~10 ms |
| Full 16-stimulation frame | ~160 ms |
| On-device reconstruction (3 Gauss-Newton iterations) | ~240 ms |
| Total frame time | ~400 ms (2.5 fps) |

---

## Firmware Design

### Software Architecture

The firmware is structured as a cooperative real-time system with three execution contexts:

1. **Interrupt context**: AD5940 conversion-complete interrupt, DMA completion, RTC alarm, BLE UART RX.
2. **Main loop**: State machine driving the acquisition sequence — multiplexer configuration → AD5940 trigger → measurement collection → reconstruction → display update → BLE/SD logging.
3. **BLE co-processor**: STM32WB55 runs an independent BLE stack with a custom GATT service exposing scan commands, reconstruction frames, and configuration.

### Key Firmware Modules

| Module | File | Responsibility |
|--------|------|----------------|
| Board configuration | `board.h` | Pin assignments, peripheral mappings, constants |
| Register definitions | `registers.h` | STM32H7 register addresses, AD5940 register map |
| AD5940 driver | `drivers/ad5940.c/h` | SPI configuration, frequency sweep, DFT control, calibration |
| Multiplexer driver | `drivers/mux.c/h` | ADG725 switching, excitation/measurement routing |
| EIT acquisition | `drivers/eit_acq.c/h` | Stimulation protocol, frame assembly, calibration |
| Reconstruction solver | `drivers/reconstruct.c/h` | FEM forward model, Jacobian, Gauss-Newton inverse, Tikhonov |
| OLED display | `drivers/display.c/h` | SSD1327 SPI driver, false-color rendering, UI menus |
| SD card logging | `drivers/sdlog.c/h` | FAT32 file system, raw frame + reconstruction CSV logging |
| BLE protocol | `drivers/ble_proto.c/h` | UART framing, GATT service emulation, packet protocol |
| Power management | `drivers/power.c/h` | Battery monitoring, STOP2 entry, peripheral domain switching |
| Environmental sensors | `drivers/env_sens.c/h` | SHT41, DS18B20, soil moisture, RTC |
| Main application | `main.c` | State machine, UI, scan orchestration, command dispatch |

### Design Decisions

**Why STM32H743 over a lower-cost MCU?**
The on-device EIT reconstruction requires solving a 576×576 sparse linear system (3 Gauss-Newton iterations) within ~250 ms. The H743's Cortex-M7 with hardware FPU and DSP instructions (CMSIS-DSP) achieves this with margin. A Cortex-M4 (e.g., STM32L4) would require 2-3 seconds per frame — too slow for field use.

**Why AD5940 instead of a discrete signal chain?**
The AD5940 integrates waveform generation, current injection, TIA, PGA, 16-bit ADC, and on-chip DFT in a single WLCSP package. A discrete solution (DDS + Howland pump + IA + ADC + FPGA DSP) would be 10× the PCB area and cost, and would require FPGA-level firmware complexity. The AD5940's register-programmable architecture lets the MCU control all measurement parameters via a simple SPI interface.

**Why adjacent stimulation protocol?**
The neighboring method minimizes common-mode voltage at the measurement electrodes (critical for the AD5940's input range) and is the most widely validated protocol in soil EIT literature. Alternative protocols (opposite, trigonometric) offer better sensitivity distribution but require higher dynamic range — a trade-off we accept for robustness.

**Why a separate BLE co-processor (STM32WB55)?**
Running the BLE stack on the main MCU would steal cycles from the reconstruction solver and complicate real-time scheduling. The WB55 handles BLE independently, communicating with the H743 via a simple UART command protocol, keeping the acquisition loop deterministic.

**Why Tikhonov regularization?**
EIT is a severely ill-posed inverse problem — small measurement noise produces large image artifacts. Tikhonov regularization (L2 penalty on the solution norm) is the simplest, most robust method for real-time use. We pre-compute the Jacobian and its pseudo-inverse at calibration time, reducing the online solve to matrix-vector multiplications. The regularization parameter λ is auto-tuned via the L-curve method at startup.

---

## Companion App

The React Native companion app provides:

- **Live scan view**: Real-time 2D conductivity map streamed over BLE, with configurable color scale and electrode overlay
- **3D volumetric reconstruction**: Stack multiple 2D scans at different depths (by repositioning the electrode ring) and reconstruct a 3D root volume using filtered back-projection
- **Longitudinal tracking**: Schedule scan sessions, compare root biomass over days/weeks/months, overlay growth curves and environmental data (soil moisture, temperature)
- **Frequency-difference analysis**: Compare conductivity maps at different excitation frequencies to separate root biomass, soil moisture, and air-gap signatures
- **Calibration and diagnostics**: Electrode contact quality check, calibration resistor verification, AD5940 self-test
- **Data export**: CSV (raw frames), PNG (reconstruction images), NetCDF (volumetric data), with metadata (GPS location, timestamp, plant ID, operator notes)
- **Study management**: Create studies, assign plants, record interventions (irrigation, fertilization), tag observations

---

## Use Cases

### 1. Crop Breeding — Drought-Resistant Cultivar Selection
Root system architecture is a key trait for drought resistance, but traditional root phenotyping requires destructive excavation. With RootTrace, breeders can non-invasively screen hundreds of cultivars for deep root systems and high root biomass density, tracking root development over the growing season without sacrificing plants.

### 2. Precision Irrigation Optimization
By mapping soil moisture heterogeneity around crop plants (the fd-EIT mode separates moisture from root biomass), irrigation managers can identify dry zones and optimize drip irrigation placement and timing. The device reveals how water is being taken up by roots in 2D, not just surface moisture.

### 3. Carbon Sequestration Research
Root biomass is a major carbon sink. RootTrace enables long-term monitoring of root growth and decomposition in situ, providing data for carbon accounting models in regenerative agriculture and soil carbon credit verification.

### 4. Rhizosphere Ecology
Study how root systems interact with soil microbial communities, mycorrhizal fungi networks, and nutrient gradients. The non-invasive nature allows repeated measurements at the same location, capturing dynamic processes.

### 5. Forest Ecology — Tree Root Mapping
Map the root systems of trees (using larger electrode spacing) to understand belowground competition, root grafting between trees, and root response to drought stress in forest ecosystems.

### 6. Phytoremediation Monitoring
Monitor root development of plants used for phytoremediation (e.g., sunflowers in contaminated soil, willows in landfill caps) to assess whether root systems are reaching target contamination depths.

### 7. Educational and Citizen Science
The open-source design makes EIT technology accessible to university labs, high schools, and citizen-science projects. Students can visualize the hidden half of plants — the root system — in real time.

---

## Target Audience

| Audience | Application |
|----------|-------------|
| Agricultural researchers | Root phenotyping, cultivar screening |
| Crop breeders | Non-destructive root trait selection |
| Soil scientists | Rhizosphere imaging, moisture mapping |
| Precision agriculture | Irrigation optimization, variable-rate management |
| Carbon credit verifiers | Root biomass quantification for soil carbon |
| Ecologists | Forest/grassland root dynamics |
| University educators | Teaching EIT, root biology, geophysics |
| Citizen scientists | Community garden monitoring, biodiversity |

---

## Bill of Materials (Key Components)

| Ref | Part | Package | Qty | Est. Cost |
|-----|------|---------|-----|----------|
| U1 | STM32H743ZIT6 | LQFP-144 | 1 | $12.50 |
| U2 | AD5940BCPZ | WLCSP-36 | 1 | $9.80 |
| U3 | STM32WB55CGU6 | QFN-48 | 1 | $5.20 |
| U4-U7 | ADG725BRUZ | TSSOP-28 | 4 | $3.20 |
| U8 | MAX17048G+T10 | TDFN-8 | 1 | $2.10 |
| U9 | MCP73871T-2CCI/ML | QFN-20 | 1 | $2.40 |
| U10 | TPS62740DSJR | SON-8 | 1 | $1.80 |
| U11 | SHT41-AD1F-R2 | DFN-4 | 1 | $1.50 |
| U12 | PCF85263ATL/AX | TSSOP-14 | 1 | $1.20 |
| DISP1 | SSD1327 2.4" OLED module | — | 1 | $4.50 |
| J1 | USB-C 16-pin receptacle | — | 1 | $0.80 |
| J2 | Dsub-25 female | — | 1 | $1.20 |
| BT1 | 18650 battery holder | — | 1 | $0.50 |
| — | 16× 316L stainless electrodes + spring contacts | — | 16 | $8.00 |
| — | PCB (4-layer, 130×95mm) | — | 1 | $4.00 |
| — | Misc passives, connectors, enclosure | — | — | $6.00 |
| **Total** | | | | **~$65** |

---

## File Structure

```
root-trace/
├── README.md                  # This file
├── firmware/
│   ├── Makefile               # ARM GCC build system
│   ├── linker.ld              # Linker script for STM32H743
│   ├── board.h                # Pin assignments, peripheral config
│   ├── registers.h            # STM32H7 + AD5940 register definitions
│   ├── main.c                 # State machine, UI, scan orchestration
│   └── drivers/
│       ├── ad5940.c/.h        # Bioimpedance AFE driver
│       ├── mux.c/.h           # 16-electrode multiplexer control
│       ├── eit_acq.c/.h       # EIT frame acquisition & calibration
│       ├── reconstruct.c/.h   # Gauss-Newton inverse solver
│       ├── display.c/.h       # SSD1327 OLED driver + UI
│       ├── sdlog.c/.h         # SD card FAT32 logging
│       ├── ble_proto.c/.h     # BLE UART protocol
│       ├── power.c/.h         # Battery & power management
│       └── env_sens.c/.h      # Environmental sensors
├── kicad/
│   ├── device.kicad_sch       # Schematic
│   ├── device.kicad_pcb       # PCB layout
│   └── device.kicad_pro       # KiCad project
└── app/
    ├── App.js                 # React Native main app
    ├── package.json           # Dependencies
    └── ...
```

---

## Calibration

RootTrace requires two calibration steps before field use:

1. **System calibration**: Measure a known 100 Ω precision resistor connected across all 16 electrode pairs. This calibrates the AD5940 current source amplitude, PGA gain, and DFT phase reference. Results are stored in flash.

2. **Field reference scan**: Before inserting the electrode ring into soil, perform an "air scan" (open-circuit, all electrodes free) and a "saline reference scan" (electrode ring submerged in 0.1 S/m NaCl solution). These provide the reference frame for normalized measurements and compensate for electrode contact impedance drift.

---

## Safety

- Excitation current is limited to **< 500 µA** (IEC 60601-1 patient safety limit for wearable medical devices is 100 µA at DC; RootTrace operates at ≥1 kHz where tissue stimulation threshold is much higher)
- Electrodes are isolated from the main battery via a **galvanic isolation barrier** (ISO7740 digital isolator) between the MCU domain and the AFE/electrode domain
- The electrode domain has its own **isolated DC-DC converter** (5V/500 mW) to prevent any ground path from soil to the battery
- Hardware current limit: PTC resettable fuse + series resistor in the excitation path

---

## Acknowledgments

RootTrace is an original design by **jayis1**, released as open-source hardware under CERN-OHL-S v2, firmware under GPL-3.0, and companion app under MIT license. The EIT reconstruction algorithm is based on the regularized Gauss-Newton method described in *Electrical Impedance Tomography: Methods, History and Applications* (Holder, 2004) and adapted for soil root imaging per the methods of Werban et al. (2008) and Mary et al. (2016).

---

*Copyright © 2026 jayis1. All rights reserved.*
*CERN-OHL-S v2 (hardware) · GPL-3.0 (firmware) · MIT (app)*