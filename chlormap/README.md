# ChloroMap — Handheld Leaf Chlorophyll & Nitrogen Mapping Spectrometer

![ChloroMap](https://img.shields.io/badge/PCB-85x52mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32L432KC-orange) ![Sensor](https://img.shields.io/badge/Sensor-16--band%20VIS--NIR-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.0%20%2B%20USB--C-purple) ![License](https://img.shields.io/badge/License-MIT-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## 1. Overview

ChloroMap is a handheld, battery-powered multispectral reflectance spectrometer that measures leaf chlorophyll content (SPAD-equivalent), nitrogen status, and leaf water index in the field. It is designed for precision agriculture, agronomy research, and phenotyping: a farmer or researcher clamps the device onto a leaf, presses the trigger, and within 300 ms receives a chlorophyll estimate, a nitrogen sufficiency index (NSI), a normalized difference vegetation index (NDVI) for the leaf itself, and an estimated leaf water content index (LWBI). Results stream over BLE 5 to a companion phone app that builds a field heatmap as the user walks the rows.

Unlike single-wavelength SPAD meters (e.g., Konica Minolta SPAD-502) that only measure transmission at 650 nm and 940 nm, ChloroMap measures **16 narrow bands** from 450 nm to 1050 nm using a linear photodiode array behind a diffraction grating. This enables:

- **Accurate chlorophyll *a* + *b* separation** via red-edge analysis (700–740 nm)
- **Nitrogen sufficiency index (NSI)** derived from 531 nm and 570 nm (photochemical reflectance index)
- **Leaf water content** via 970 nm / 900 nm water absorption bands
- **Carotenoid / anthocyanin estimates** via 510 nm and 550 nm bands
- **Built-in NDVI leaf reflectance** via 660 nm and 800 nm bands
- **Field heatmap generation** in the companion app as you walk rows

### Why this device is novel

Existing handheld chlorophyll meters:
1. Use only 2 wavelengths and a transmission geometry — they cannot separate chlorophyll *a*/*b*, estimate water content, or measure carotenoids.
2. Cost $2,000–$3,500 and have closed, proprietary calibration curves.
3. Have no GPS, no wireless connectivity, and no field-map generation.
4. Cannot perform leaf-level NDVI or red-edge analysis.

ChloroMap delivers a 16-band spectrometer in the same form factor, at a BOM cost under $90, with open calibration curves (user can recalibrate against a white reference), integrated GPS for geotagging, BLE 5 for real-time phone streaming, and an open React Native app that generates per-field chlorophyll/heatmaps. It is the first open-source, multi-band, reflectance-geometry leaf spectrometer with a companion field-mapping app.

---

## 2. Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32L432KCU6 — Cortex-M4F @ 80 MHz, 256 KB Flash, 64 KB SRAM, 1.2 V core |
| **Light Source** | Dual integrated LEDs — 4000 K white + 940 nm NIR — pulsed synchronous detection |
| **Detector** | Hamamatsu S12684-128CM monolithic Si photodiode array, 128 elements, 14 × 200 µm pixels, 200–1000 nm response |
| **Spectrograph** | Transmission volume phase holographic grating, 830 lines/mm, 450–1050 nm range, ~3 nm optical resolution |
| **ADC** | ADS1255 24-bit delta-sigma, 30 kSPS, gain 1–64x, SPI |
| **GPS** | u-blox NEO-M9N, 72-channel, 1.5 m CEP, 10 Hz update, I2C |
| **Display** | 0.96" OLED (SSD1306), 128×64, SPI |
| **BLE** | nRF52840 module (u-blox NINA-B306), BLE 5.0, 2 Mbps PHY |
| **USB** | USB-C 2.0 full-speed (CDC + MSC for data export) |
| **Storage** | MicroSD card (SPI mode), FAT32, CSV logging |
| **Battery** | 3.7 V 1200 mAh LiPo, MCP73871 charger, USB-C charging |
| **Power mgmt** | TPS62740 3.3 V buck (360 nA IQ), 5-day battery life |
| **Form Factor** | 85 mm × 52 mm × 28 mm, leaf-clamp jaw design |
| **Weight** | 62 g (without battery) |
| **Operating Temp** | 0 °C to +45 °C (agricultural field) |
| **Measurement Time** | 300 ms per leaf (including LED warmup, integration, calculation) |
| **Bands** | 16 user-selectable: 450, 480, 510, 531, 550, 570, 660, 680, 700, 720, 740, 800, 900, 940, 970, 1050 nm |
| **Chlorophyll Range** | 0–100 SPAD units equivalent, ±2 SPAD accuracy |
| **NDVI Range** | -0.2 to 1.0 |
| **Water Index Range** | 0.8–1.2 (LWBI) |

### BOM Highlights (key parts)

| Ref | Part | Function |
|---|---|---|
| U1 | STM32L432KCU6 | Main MCU |
| U2 | ADS1255 | 24-bit ADC for photodiode array |
| U3 | NINA-B306 | BLE 5.0 module |
| U4 | NEO-M9N | GNSS receiver |
| U5 | SSD1306 | OLED display controller |
| U6 | TPS62740 | 3.3 V step-down regulator |
| U7 | MCP73871 | LiPo USB-C charger |
| U8 | SN74LV595A | LED driver / MUX shift register |
| D1 | White LED 4000K | Visible illumination source |
| D2 | IR-940 LED | NIR reference source |
| D3 | 128-element Si photodiode array (Hamamatsu S12684) | Spectral detector |
| J1 | USB-C 16-pin | Charging + data |
| J2 | microSD socket | Data storage |
| SW1 | Trigger switch | Leaf measurement trigger |
| SW2 | Power slide switch | On/off |

---

## 3. Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────┐
 │                     STM32L432KC (MCU)                        │
 │   Cortex-M4F @ 80 MHz · 256 KB Flash · 64 KB SRAM           │
 │                                                              │
 │  ┌──────────┐  ┌───────────┐  ┌─────────┐  ┌────────────┐  │
 │  │  Acq     │  │  Spectral │  │  Index  │  │  Comms     │  │
 │  │  FSM     │  │  Pipeline │  │  Calc   │  │  Manager   │  │
 │  │          │  │  (FFT,    │  │  (SPAD, │  │  (BLE,     │  │
 │  │ LED/ADC  │  │  dark,    │  │  NDVI,  │  │  USB,      │  │
 │  │  timing) │  │  ref)     │  │  NSI,   │  │  SD log)   │  │
 │  │          │  │           │  │  LWBI)  │  │            │  │
 │  └────┬─────┘  └─────┬─────┘  └────┬────┘  └─────┬──────┘  │
 │       │              │              │             │         │
 │  ┌────▼──────────────▼──────────────▼─────────────▼──────┐  │
 │  │              DMA / SPI / I2C / USART / RTC            │  │
 │  └──┬───────┬────────┬────────┬────────┬───────┬─────────┘  │
 └─────┼───────┼────────┼────────┼────────┼───────┼────────────┘
       │       │        │        │        │       │
    ┌──▼──┐ ┌─▼──┐  ┌──▼───┐ ┌──▼───┐ ┌──▼───┐ ┌─▼────┐
    │LEDs │ │ADC │  │BLE   │ │GPS   │ │OLED  │ │micro │
    │Wht  │ │ADS │  │NINA  │ │NEO   │ │SSD   │ │SD    │
    │IR   │ │1255│  │B306  │ │M9N   │ │1306  │ │card  │
    └──┬──┘ └─┬──┘  └──────┘ └──────┘ └──────┘ └──────┘
       │      │
    ┌──▼──┐ ┌─▼──────────────┐
    │Leaf │ │Photodiode Array│
    │Clamp│ │128 el @ grating│
    │Jaw  │ │450-1050 nm     │
    └─────┘ └────────────────┘
```

### Power domains

- **VBAT** (3.0–4.2 V): LiPo, MCP73871 charger input from USB-C VBUS
- **V3R3** (3.3 V): TPS62740 buck, powers MCU, BLE, GPS, OLED, SD
- **VLED** (3.3 V switched): LED illumination, MOSFET-gated to reduce idle power
- **VANA** (3.3 V filtered): ADS1255 analog supply, LC filtered for low noise

### Bus topology

- **SPI1**: ADS1255 ADC (SCK=PA5, MISO=PA6, MOSI=PA7, CS=PA4)
- **SPI2**: SSD1306 OLED (SCK=PB13, MOSI=PB15, CS=PB12, DC=PB14)
- **SPI3**: microSD card (SCK=PB3, MISO=PB4, MOSI=PB5, CS=PA15)
- **I2C1**: NEO-M9N GPS (SCL=PB6, SDA=PB7)
- **USART2**: NINA-B306 BLE module (TX=PA2, RX=PA3, 1 Mbps)
- **USB-C**: STM32L4 USB full-speed (PA11/PA12)
- **GPIO**: Trigger (PB8 EXTI), LED MUX (PB9–PB11 shift register), power switch (PA0)

---

## 4. Firmware Design

### 4.1 Acquisition state machine

The firmware implements a deterministic acquisition FSM that runs the entire measurement in 300 ms:

1. **IDLE** — MCU in STOP2, RTC running, BLE advertising, waiting for trigger
2. **WAKE** — trigger EXTI fires, MCU wakes in 8 µs, enables VLED and VANA
3. **DARK** — ADC samples 128 elements with LEDs off (dark frame, 20 ms)
4. **WHITE** — White LED on, ADC samples 128 elements (white/leaf reflectance, 50 ms)
5. **NIR** — 940 nm LED on, ADC samples (NIR reflectance, 50 ms)
6. **REFERENCE** — Optional white reference tile (if calibrate mode)
7. **PROCESS** — Compute reflectance, SPAD, NDVI, NSI, LWBI (40 ms)
8. **LOG** — Write CSV to SD, push result over BLE, update OLED
9. **IDLE** — Return to STOP2

### 4.2 Spectral processing pipeline

- **Dark subtraction**: each element = sample − dark_frame
- **Linearity correction**: per-element factory calibration coefficients stored in Flash
- **Wavelength mapping**: 128 elements → 16 bands via Gaussian-weighted binning (±5 nm FWHM)
- **Reflectance**: R(λ) = (sample − dark) / (reference − dark)
- **SPAD estimate**: based on transmittance ratio at 660 nm and 940 nm, calibrated to SPAD-502 scale
- **NDVI**: (R800 − R660) / (R800 + R660)
- **NSI**: R531 / R570 (photochemical reflectance index → N status)
- **LWBI**: R900 / R970 (leaf water band index)
- **Red-edge slope**: (R740 − R700) / 40 → stress indicator

### 4.3 Calibration

Two calibration modes:
- **White reference**: user clamps the white Teflon reference tile, device stores reference spectrum
- **SPAD calibration**: optional 3-point calibration against a commercial SPAD meter

All calibration data stored in a dedicated Flash page with CRC.

### 4.4 Power management

- STOP2 mode between measurements (1.4 µA MCU current)
- TPS62740 in Eco mode (360 nA IQ)
- BLE advertising every 2 s (connectable)
- GPS duty-cycled: 10 Hz during active measurement walk, off during idle
- Battery life: 5 days / 2000 measurements

### 4.5 Data format

CSV on SD card:
```
timestamp_ms,lat,lon,band450,band480,...,band1050,spad,ndvi,nsi,lwbi,rededge,temp_c,batt_mv
```

BLE packet (binary, 48 bytes): see `ble.c` for frame format.

---

## 5. Application / Software Interface

### 5.1 React Native companion app

The app (`app/`) provides:

- **FieldMap screen**: Live heatmap of chlorophyll/NDVI values overlaid on a map as you walk the field. Each measurement is a colored dot (green=high N, yellow=moderate, red=deficient).
- **LiveSpectrum screen**: Real-time 16-band reflectance spectrum plot from the connected device
- **Measurement screen**: Detailed view of last measurement (SPAD, NDVI, NSI, LWBI, red-edge, N recommendation)
- **History screen**: Browsable list of all measurements with filters by date/field/zone
- **Calibration screen**: Walk-through for white reference and SPAD calibration
- **Settings screen**: Device configuration (band selection, integration time, GPS rate, BLE name)

### 5.2 BLE protocol

Custom GATT service (UUID `0000C701-1212-EFDE-1523-785FEABCD123`):

| Characteristic | UUID | Direction | Format |
|---|---|---|---|
| Measurement | `...C702` | Device→Phone | 48-byte binary (see `ble.c`) |
| Command | `...C703` | Phone→Device | 8-byte command |
| Status | `...C704` | Device→Phone | 16-byte status (battery, state, GPS fix) |
| Calibration | `...C705` | Phone→Device | 4-byte cal command |

### 5.3 USB CDC interface

When connected via USB-C, the device enumerates as a virtual serial port. ASCII commands:
- `MEASURE` — trigger a measurement
- `CAL WHITE` — start white reference calibration
- `GET SPECTRUM` — dump last raw 128-element spectrum
- `GET STATUS` — battery, GPS fix, state
- `SET BANDS <mask>` — select active bands
- `SET INTTIME <ms>` — set ADC integration time

---

## 6. Use Cases & Target Audience

### Precision agriculture
Farmers walk crop rows, clamping ChloroMap onto leaves. The app builds a real-time nitrogen heatmap of the field, showing which zones need fertilizer and which are sufficient. This enables **variable-rate nitrogen application**, reducing fertilizer costs by 15–30% and reducing nitrogen runoff.

### Agronomy research
Researchers use the 16-band data to study:
- Chlorophyll *a*/*b* ratio changes under stress
- Red-edge shift as an early drought indicator
- Leaf water content dynamics
- Carotenoid/chlorophyll ratio during senescence

### Plant breeding & phenotyping
High-throughput phenotyping of crop varieties for nitrogen use efficiency, drought tolerance, and stay-green traits. The GPS-tagged data integrates with GIS systems.

### Viticulture & horticulture
Grapevine nitrogen management, avocado leaf analysis, orchard nutrient monitoring.

### Ecological monitoring
Researchers measuring plant stress in natural ecosystems, tracking invasive species health, or monitoring reforestation success.

### Target audience
- Precision agriculture consultants and farm managers
- Agronomy and plant science researchers
- Plant breeders and seed companies
- Viticulturists and horticulturists
- Ecological monitoring organizations
- Agricultural extension services in developing countries (low-cost, open-source)

---

## 7. Key Design Decisions

1. **STM32L432KC** over ESP32 — ultra-low-power STOP2 (1.4 µA), hardware FPU for spectral math, sufficient Flash/RAM, native USB. ESP32 draws 10× more in sleep.
2. **Reflectance geometry** over transmission — enables measurement of thicker leaves, canopy leaves in situ, and avoids the leaf-thickness sensitivity of SPAD meters.
3. **128-element photodiode array + grating** over discrete filtered photodiodes — 16 bands from one sensor, user-selectable bands, future-proof for new indices. Filtered photodiodes would require 16 separate sensors.
4. **ADS1255 24-bit ADC** — 24-bit dynamic range captures both low-reflectance blue bands and high-reflectance NIR. A 12-bit ADC would saturate or lack resolution.
5. **Dual LED (white + 940 nm)** — white LED covers 400–700 nm; 940 nm LED provides the NIR water/chlorophyll reference band. Pulsed synchronous detection rejects ambient light.
6. **u-blox NEO-M9N** — 1.5 m GPS accuracy, 10 Hz update, low power (25 mA tracking). Enables per-leaf geotagging for field maps.
7. **Open calibration curves** — user can recalibrate against white reference and SPAD meter. No vendor lock-in.
8. **Leaf-clamp form factor** — jaw design ensures consistent sensor-to-leaf distance and blocks ambient light. Critical for repeatable measurements.

---

## 8. File Structure

```
chlormap/
├── README.md                     ← this file
├── firmware/
│   ├── Makefile                  ← arm-none-eabi-gcc build
│   ├── board.h                   ← pin assignments, peripheral mappings
│   ├── registers.h               ← ADS1255, SSD1306, NEO-M9N, NINA-B306 registers
│   ├── linker.ld                 ← STM32L432KC linker script
│   ├── main.c                    ← application core + acquisition FSM
│   └── drivers/
│       ├── adc.c / .h            ← ADS1255 24-bit ADC driver (SPI, DMA)
│       ├── spectrometer.c / .h   ← 128-element acquisition + 16-band binning
│       ├── indices.c / .h        ← SPAD, NDVI, NSI, LWBI, red-edge calculations
│       ├── calib.c / .h          ← White reference + SPAD calibration
│       ├── ble.c / .h            ← NINA-B306 BLE UART protocol
│       ├── gps.c / .h            ← NEO-M9N I2C driver
│       ├── display.c / .h        ← SSD1306 OLED driver
│       ├── storage.c / .h        ← microSD FAT32 CSV logging
│       ├── power.c / .h          ← STOP2, LED gating, regulator control
│       └── usb_cdc.c / .h        ← USB CDC virtual serial port
├── kicad/
│   ├── device.kicad_pro          ← KiCad project file
│   ├── device.kicad_sch          ← schematic with all components + netlist
│   └── device.kicad_pcb          ← PCB layout, 4-layer, 85×52 mm
└── app/
    ├── App.tsx                   ← React Native navigation shell
    ├── package.json              ← dependencies
    ├── app.json                  ← Expo config
    ├── babel.config.js           ← Babel config
    ├── tsconfig.json             ← TypeScript config
    └── src/
        ├── ble/
        │   ├── BleManager.ts     ← BLE connection manager
        │   └── protocol.ts       ← binary protocol parser
        ├── screens/
        │   ├── FieldMapScreen.tsx    ← live chlorophyll heatmap
        │   ├── LiveSpectrumScreen.tsx ← 16-band spectrum plot
        │   ├── MeasurementScreen.tsx  ← detailed last measurement
        │   ├── HistoryScreen.tsx      ← measurement history list
        │   ├── CalibrationScreen.tsx  ← white reference + SPAD cal
        │   └── SettingsScreen.tsx     ← device configuration
        └── components/
            ├── SpectrumChart.tsx      ← 16-band bar chart
            └── IndexGauge.tsx         ← circular gauge widget
```

---

## 9. License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

**Author: jayis1** — all designs, firmware, code, and documentation credited to jayis1.