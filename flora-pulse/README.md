# FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**  
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

---

## Overview

FloraPulse is a novel, open-source hardware platform that monitors plant stress in real-time by measuring **electrical action potentials** in living plant tissue — a field known as *phytosensing*. While visible symptoms of drought, pathogen attack, or herbivore damage appear hours or days after the initial stress event, plants generate characteristic electrical signals within **seconds to minutes** of being stressed. FloraPulse captures these signals using surface electrodes, classifies them by stress type, and transmits alerts via BLE and LoRa — giving growers, researchers, and farmers an **early warning system** for plant health.

Unlike existing plant sensors that only measure soil moisture or ambient conditions, FloraPulse measures the **plant's own physiological response** directly, combining three complementary sensing modalities:

1. **Plant Action Potential (AP) Detection** — 2-channel 24-bit biopotential ADC (ADS1292) measures microvolt-level electrical signals from silver/silver-chloride electrodes inserted into the plant stem or petiole. An adaptive threshold algorithm detects AP events and classifies them as drought stress, wounding, light response, or herbivory based on amplitude and duration morphology.

2. **Heat-Pulse Sap Flow Measurement** — A resistive heater probe and two NTC thermistor probes measure sap flow velocity using the T-max method. A 4-second heat pulse is applied, and the time-to-peak temperature at the downstream sensor gives the flow velocity. This indicates transpiration rate and water stress directly.

3. **Environmental Sensing** — BME280 (temperature, humidity, pressure), APDS-9301 (ambient light/PAR), and a capacitive leaf-wetness sensor correlate plant electrical activity with environmental conditions, enabling VPD (vapor pressure deficit) computation and stress attribution.

---

## Why FloraPulse Is Novel

No existing open-source hardware device combines plant electrophysiology with sap flow and environmental monitoring in a single, field-deployable package. Current approaches to plant stress monitoring fall into three categories, each with limitations:

- **Soil moisture sensors** measure the environment, not the plant's response. They cannot detect herbivory, pathogen attack, or root-zone stress until wilting occurs.
- **Sap flow sensors** measure water transport but are typically standalone instruments costing $1,000+ and require separate data loggers.
- **Research-grade electrophysiology** equipment is laboratory-bound, requires Faraday cages, and costs $5,000–$50,000. No portable, field-deployable plant AP monitor exists.

FloraPulse bridges this gap with a $50 BOM, weatherproof enclosure, BLE+LoRa connectivity, and battery operation lasting 7+ days. It brings plant electrophysiology out of the laboratory and into the field, greenhouse, and farm.

---

## Hardware Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32L476RGT6 — Cortex-M4F @ 80 MHz, 1 MB Flash, 128 KB SRAM |
| **Biopotential ADC** | TI ADS1292 — 2-channel, 24-bit delta-sigma, 250 SPS, PGA gain 1–12 |
| **Sap-Flow ADC** | TI ADS1220 — 24-bit, 4-channel, 2 kSPS, PGA, SPI |
| **Environmental Sensor** | Bosch BME280 — ±1°C, ±3% RH, ±1 hPa, I²C |
| **Light Sensor** | Avago APDS-9301 — Ambient light, 16-bit, I²C, 0–65,535 lux |
| **Fuel Gauge** | Maxim MAX17048 — Li-ion state-of-charge, I²C |
| **BLE** | Nordic nRF52840 — BLE 5.0, 2 Mbps, UART bridge |
| **LoRa** | Semtech SX1276 — 868 MHz, SF7–SF12, +20 dBm, up to 15 km range |
| **Heater Driver** | TI TPS28225 — H-bridge MOSFET gate driver for heat pulse |
| **SD Card** | MicroSD slot (SPI mode, shared SPI2 bus) |
| **RTC** | Internal STM32 RTC with 32.768 kHz LSE crystal |
| **Power** | 18650 Li-ion (2200 mAh), MCP73871 USB-C charger, TPS61023 boost |
| **Battery Life** | 7+ days (1 Hz sampling, 5-min LoRa uplink) |
| **Form Factor** | 80mm × 50mm PCB, IP65 weatherproof enclosure |
| **Operating Temp** | -10°C to +60°C (commercial) |
| **Weight** | 85g (with battery) |

---

## Architecture & Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        FloraPulse Block Diagram                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────┐    Ag/AgCl     ┌──────────┐     SPI1     ┌─────────┐ │
│  │ Plant    │───electrodes──▶│ ADS1292  │──────────────▶│         │ │
│  │ Stem     │    (5-pin J3)  │ 24-bit   │  PA5/6/7     │  STM32  │ │
│  └──────────┘                │ Biopot   │              │  L476    │ │
│                              └──────────┘              │  Cortex  │ │
│  ┌──────────┐    NTC 10kΩ    ┌──────────┐     SPI2     │  -M4F    │ │
│  │ Sap Flow │──thermistors──▶│ ADS1220  │──────────────▶│ 80MHz   │ │
│  │ Probes   │   +heater (J4) │ 24-bit   │  PB13/14/15 │         │ │
│  └──────────┘                └──────────┘              │         │ │
│                                                        │         │ │
│  ┌──────────┐    I²C          ┌──────────┐    I²C1     │         │ │
│  │ BME280   │◀────────────────│  APDS    │──────────────▶│         │ │
│  │ T/H/P    │                 │ 9301    │  PB8/9       │         │ │
│  └──────────┘                 └──────────┘              │         │ │
│                                                        │         │ │
│  ┌──────────┐    555 osc      ┌──────────┐   TIM3_ETR   │         │ │
│  │ Leaf Wet │──frequency────▶│ TIM3     │──────────────▶│         │ │
│  └──────────┘                 └──────────┘              │         │ │
│                                                        │         │ │
│  ┌──────────┐   USART1        ┌──────────┐             │         │ │
│  │ Phone    │◀───BLE 5.0─────│ nRF52840 │──USART1──────▶│         │ │
│  │ (App)    │                 └──────────┘  PA9/10     │         │ │
│  └──────────┘                                           │         │ │
│                                                        │         │ │
│  ┌──────────┐    LoRa         ┌──────────┐   SPI2      │         │ │
│  │ Gateway  │◀──868 MHz──────│ SX1276   │──────────────▶│         │ │
│  │          │                 │ + Antenna│  PB13/14/15  │         │ │
│  └──────────┘                 └──────────┘   PC7 CS    │         │ │
│                                                        │         │ │
│  ┌──────────┐    TIM1 PWM     ┌──────────┐             │         │ │
│  │ Heater   │◀──4s pulse─────│TPS28225  │◀──TIM1_CH1──│         │ │
│  │ 47Ω 3W  │                 │ H-bridge │   PC0        │         │ │
│  └──────────┘                 └──────────┘              └────┬────┘ │
│                                                               │      │
│  ┌──────────┐                ┌──────────┐                      │      │
│  │ MicroSD  │──SPI2─────────│ CS-mux   │──────────────────────┘      │
│  │ Logger   │                └──────────┘                             │
│  └──────────┘                                                          │
│                                                                        │
│  ┌──────────┐    USB-C      ┌──────────┐    ┌──────────┐              │
│  │ 18650    │◀──charging───│MCP73871  │◀──│ TPS61023 │◀─ +5V        │
│  │ 2200mAh  │               │ charger  │   │ boost    │              │
│  └──────────┘               └──────────┘   └──────────┘              │
│       │                                                              │
│       └──── MAX17048 fuel gauge (I²C) ──────────────────────────────│
│                                                                    │
└─────────────────────────────────────────────────────────────────────┘
```

### Bus Topology

| Bus | Peripheral | Devices | Pins |
|---|---|---|---|
| **SPI1** | STM32 → ADS1292 | ADS1292 (biopotential ADC) | PA5 (SCK), PA6 (MISO), PA7 (MOSI), PB0 (CS), PB1 (DRDY), PB2 (START), PB10 (PWDN) |
| **SPI2** | STM32 → ADS1220 / SX1276 / SD | CS-multiplexed | PB13 (SCK), PB14 (MISO), PB15 (MOSI), PB4 (ADS1220 CS), PB6 (SD CS), PC7 (SX1276 CS) |
| **I²C1** | STM32 → BME280 / APDS-9301 / MAX17048 | 3 devices, 100 kHz | PB8 (SCL), PB9 (SDA) |
| **USART1** | STM32 → nRF52840 | BLE module, 115200 baud | PA9 (TX), PA10 (RX) |
| **TIM1** | STM32 → TPS28225 → Heater | One-shot PWM, 4 s pulse | PC0 (CH1), PC1 (EN) |
| **TIM3** | STM32 ← Leaf-wetness oscillator | External clock counter | PD2 (ETR) |
| **USB** | STM32 ← USB-C | DFU / serial console | PA11 (DM), PA12 (DP) |

### Power Domains

| Domain | Voltage | Source | Devices |
|---|---|---|---|
| **VBUS** | 5V | USB-C | MCP73871 charger input |
| **VBAT** | 3.0–4.2V | 18650 Li-ion | MCP73871 output, MAX17048 monitor |
| **+3V3** | 3.3V | TPS61023 boost | STM32, all sensors, BLE, LoRa |
| **GND** | 0V | Common | All devices |
| **AGND** | 0V | Star-ground to GND | ADS1292, ADS1220 analog front-end |

---

## Firmware Design

### Architecture

The firmware follows a **cooperative super-loop** architecture with interrupt-driven I/O. This is simpler than an RTOS for the target application (low sample rates, deterministic timing) and avoids RTOS overhead in the 128 KB SRAM STM32L476.

**Main loop responsibilities (10 ms tick):**
1. Poll BLE UART for incoming commands (frame assembly + CRC)
2. Check for async sap-flow measurement completion
3. Execute 1 Hz sampling cycle (read all sensors, detect AP events, log to SD, stream to BLE)
4. Trigger 15-minute sap-flow heat pulse measurement
5. Transmit 5-minute LoRa telemetry uplink
6. Update LED indicators (stress alert, BLE activity, LoRa activity)
7. Check user button for mode switching

**Interrupt service routines:**
- `SysTick_Handler` — 1 ms system tick (delay functions, timing)
- `USART1_IRQHandler` — BLE module RX byte (ring buffer, frame assembly)
- `EXTI1` — ADS1292 DRDY falling edge (not currently used in poll mode, reserved for DMA)
- `EXTI5` — ADS1220 DRDY falling edge (thermistor data ready)

### State Machine

```
BOOT → IDLE → MONITOR (BLE streaming) / LOGGING (SD only) /
                CALIB / STREAM (waveform) / LORA (low-power) → SLEEP → (wake) → IDLE
```

### Key Firmware Design Decisions

1. **ADS1292 gain 12** — Plant APs attenuate to 10–500 µV at surface electrodes. Gain 12 provides ±201 mV full-scale with 12 nV LSB — 4× headroom above maximum expected signal while resolving sub-µV noise floor.

2. **250 SPS sample rate** — Plant APs last 1–10 seconds (1000× slower than neural APs). 250 SPS gives 4 samples per AP at the fastest duration, sufficient for event detection without excessive data volume.

3. **Adaptive threshold detection** — Baseline wander (from electrode polarization, temperature drift) is tracked via exponential moving average (α=0.001). Threshold = max(fixed, 3×noise_rms). This prevents false triggers from baseline drift while maintaining sensitivity to genuine events.

4. **T-max sap flow method** — Simpler and more robust than the heat-ratio method (HRM) for field deployment. Requires only one downstream sensor (not two at different distances). Computes velocity as v = √(α / (π × t_max)). Trade-off: less accurate at very low flow rates.

5. **SPI2 CS-multiplexing** — The ADS1220, SX1276, and SD card share a single SPI2 bus with separate CS lines. This saves 3 pins on the 64-pin STM32L476 at the cost of bus contention (managed by ensuring no simultaneous access).

6. **LoRa SF12** — Maximum spreading factor for maximum range (up to 15 km LOS). Data rate is only ~290 bps, but the 24-byte telemetry packet fits in under 1 second airtime. The 5-minute uplink interval balances latency with power consumption.

7. **Binary log format (48 bytes/record)** — Compact binary format instead of CSV. At 1 Hz sampling, a 32 GB SD card stores ~600 years of data. Records are packed 10 per 512-byte sector for efficient block I/O.

### Action Potential Classification

The `apdetection.c` module classifies detected events based on amplitude and duration morphology:

| Classification | Amplitude | Duration | Rise Time | Biological Trigger |
|---|---|---|---|---|
| **Drought stress** | 50–200 µV | 2–5 s | Slow (gradual) | ABA hormone signaling, stomatal closure |
| **Wounding** | 200–500 µV | 1–3 s | Rapid (sharp) | Mechanical damage, pruning, herbivore bite |
| **Light response** | 20–80 µV | 5–10 s | Gradual | Photosynthetic induction, circadian |
| **Herbivory** | 100–300 µV | 1–4 s | Rapid + fluctuation | Insect feeding, elicitor response |

---

## Application / Software Interface

### Companion App (React Native)

The companion app provides five tabs:

| Screen | Function |
|---|---|
| **Monitor** | Real-time stress gauge, AP signals (µV), environmental data (temp, humidity, VPD, light), sap flow velocity, leaf wetness. Trigger heat pulse. Start/stop BLE streaming. |
| **Waveform** | Live scrolling oscilloscope display of both AP channels (250 SPS). Adjustable threshold (10–500 µV). Event count and detection statistics. |
| **Sap Flow** | Heat-pulse measurement results: velocity, ΔT, t_max, baseline temperature. Temperature response curve. Measurement history. Manual trigger button. |
| **Events** | Detected AP event log with classification (drought, wounding, light, herbivory). Summary statistics by category. Filter by event type. |
| **Settings** | Device info, firmware version, threshold/rate configuration, adaptive threshold toggle, clock sync, log download/erase, sensor calibration. |

### BLE Protocol

Binary framed protocol over Nordic UART Service (NUS):

```
[SOF=0xAA] [LEN] [OPCODE] [PAYLOAD...] [CRC-XOR] [EOF=0x55]
```

Commands: PING, GET_STATUS, GET_SAMPLE, START/STOP_STREAM, START_CALIB, SET/GET_RATE, SET/GET_THRESHOLD, DOWNLOAD_LOG, ERASE_LOG, SET_TIME, GET_VERSION, GET_WAVEFORM, TRIGGER_HEATER.

### LoRa Telemetry Packet (24 bytes)

Uplink every 5 minutes to a LoRaWAN gateway:

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | Device ID |
| 1–4 | 4 | Timestamp (Unix) |
| 5–6 | 2 | AP average (µV) |
| 7–8 | 2 | Sap flow velocity (cm/h × 10) |
| 9–10 | 2 | Temperature (°C × 100) |
| 11–12 | 2 | Humidity (% × 100) |
| 13–14 | 2 | Pressure (Pa, low 16 bits) |
| 15–16 | 2 | Light (lux) |
| 17 | 1 | Leaf wetness (%) |
| 18 | 1 | Battery (%) |
| 19 | 1 | Flags |
| 20–21 | 2 | AP rate (Hz × 10) |
| 22–23 | 2 | CRC16 |

---

## Use Cases & Target Audience

### 1. Precision Agriculture & Greenhouse Management
Greenhouse operators can deploy FloraPulse nodes across crops to detect drought stress before wilting occurs, optimize irrigation timing, and monitor VPD for climate control. LoRa mesh enables farm-scale deployment without WiFi infrastructure.

### 2. Plant Biology Research
Researchers studying plant electrophysiology, stress signaling, and plant neurobiology can use FloraPulse as an affordable, portable alternative to lab-bound equipment. The open data format and raw waveform streaming support custom analysis.

### 3. Vineyards & High-Value Crops
Wine grape growers can monitor vine water stress in real-time, enabling precision irrigation that optimizes fruit quality. Sap flow data directly indicates transpiration rate, while AP signals detect stress onset 2–4 hours before visible symptoms.

### 4. Urban Forestry & Arboriculture
City arborists can monitor street trees for drought stress, construction damage (wounding APs), and pest infestation (herbivory APs). The LoRa uplink enables monitoring across an urban area from a single gateway.

### 5. Ecological Monitoring
Ecologists can deploy FloraPulse in natural ecosystems to study plant responses to environmental change, drought events, and herbivore pressure. The long battery life (7+ days) and weatherproof enclosure support remote field deployment.

### 6. Educational & Citizen Science
The low cost (~$50 BOM) and open-source design make FloraPulse accessible to schools, makerspaces, and citizen scientists. Students can observe plant electrical activity in real-time and learn about plant physiology, electronics, and data analysis.

---

## File Structure

```
flora-pulse/
├── README.md                          — This document
├── firmware/
│   ├── main.c                         — Main firmware (874 lines)
│   ├── registers.h                    — STM32L476 register definitions (610 lines)
│   ├── board.h                         — Pin assignments & constants (374 lines)
│   ├── Makefile                        — ARM GCC build system
│   └── drivers/
│       ├── ads1292.c / .h             — Biopotential ADC driver
│       ├── bme280.c / .h              — Environmental sensor driver
│       ├── sapflow.c / .h             — Heat-pulse sap-flow driver
│       ├── leafwet.c / .h             — Leaf-wetness capacitive sensor
│       ├── lora.c / .h                — SX1276 LoRa transceiver driver
│       ├── ble.c / .h                  — nRF52840 BLE communication driver
│       ├── sdlog.c / .h                — SD card data logging driver
│       └── apdetection.c / .h         — AP event detection & classification
├── kicad/
│   ├── device.kicad_sch               — Schematic with components & netlist
│   ├── device.kicad_pcb               — PCB layout with footprints & board outline
│   └── device.kicad_pro               — Project settings & net classes
├── app/
│   ├── App.js                         — React Native entry point
│   ├── package.json                   — Dependencies
│   ├── screens/
│   │   ├── MonitorScreen.js           — Real-time stress monitor
│   │   ├── WaveformScreen.js          — AP waveform oscilloscope
│   │   ├── SapFlowScreen.js           — Sap-flow measurement & analysis
│   │   ├── EventsScreen.js            — AP event log & classification
│   │   └── SettingsScreen.js          — Device configuration & data management
│   ├── components/
│   │   ├── StatusBar.js               — Connection & battery status
│   │   ├── StressGauge.js             — Plant stress level indicator
│   │   ├── WaveformChart.js           — Scrolling waveform display
│   │   └── SapFlowChart.js            — Temperature response curve
│   └── utils/
│       ├── BleContext.js              — BLE connection management
│       └── protocol.js                — Binary protocol parser/builder
```

---

## Bill of Materials (Key Components)

| Ref | Part | Description | Qty | Est. Cost |
|---|---|---|---|---|
| U1 | STM32L476RGT6 | MCU Cortex-M4F 80MHz 1MB Flash | 1 | $5.20 |
| U2 | ADS1292IPBS | 2-ch 24-bit biopotential ADC | 1 | $8.50 |
| U3 | ADS1220IRVAT | 24-bit 4-ch delta-sigma ADC | 1 | $4.20 |
| U4 | BME280 | T/H/P sensor | 1 | $3.50 |
| U5 | APDS-9301 | Ambient light sensor | 1 | $2.80 |
| U6 | MAX17048G+T10 | Li-ion fuel gauge | 1 | $2.10 |
| U7 | SX1276 (RFM95W) | LoRa transceiver module | 1 | $7.00 |
| U8 | nRF52840 | BLE 5.0 module | 1 | $6.50 |
| U9 | TPS61023 | 3.3V boost converter | 1 | $1.80 |
| U10 | MCP73871 | USB-C Li-ion charger | 1 | $2.50 |
| U11 | TPS28225 | H-bridge MOSFET driver | 1 | $1.90 |
| Y1 | 8 MHz crystal | HSE oscillator | 1 | $0.30 |
| Y2 | 32.768 kHz crystal | LSE for RTC | 1 | $0.30 |
| J1 | USB-C receptacle | Power + DFU | 1 | $0.80 |
| J2 | MicroSD socket | Data logging | 1 | $0.90 |
| J3 | 5-pin header | Electrode probe cable | 1 | $0.20 |
| J4 | 5-pin header | Sap-flow probe cable | 1 | $0.20 |
| BT1 | 18650 holder | Battery holder | 1 | $0.80 |
| ANT1 | U.FL connector | LoRa antenna | 1 | $0.50 |
| — | Passives | R, C, L (0805 SMD) | ~40 | $2.00 |
| — | PCB | 80×50mm 2-layer ENIG | 1 | $3.00 |
| **Total** | | | | **~$56** |

---

## Key Design Decisions (Summary)

1. **STM32L476 over STM32H7** — Ultra-low-power (1.2 µA standby) is critical for battery operation. 80 MHz is sufficient for 250 SPS sampling and AP detection. Built-in RTC with LSE crystal for timestamping without GPS. Lower cost ($5.20 vs $12+).

2. **ADS1292 over generic ADC** — Purpose-built for biopotential measurement with integrated PGA, right-leg drive, and lead-off detection. 24-bit resolution at 12 nV LSB essential for microvolt-level plant signals. Internal reference eliminates drift.

3. **Separate ADS1220 for thermistors** — Sap-flow measurement requires a different ADC than the biopotential ADC. The ADS1220 provides 24-bit resolution for precise temperature measurement (±0.01°C) at a lower cost than using a second ADS1292.

4. **nRF52840 as BLE bridge** — Rather than integrating BLE into the MCU (which would require an STM32WB or similar), the nRF52840 runs as a UART-to-BLE transparent bridge. This simplifies firmware (standard UART) and allows the BLE module to be upgraded independently.

5. **SX1276 LoRa at SF12** — Maximum range over data rate. A 24-byte packet at SF12/BW125 transmits in ~1 second. The 5-minute interval gives <0.1% duty cycle (EU 868 MHz limit is 1%), well within regulatory limits.

6. **Binary log format over CSV** — 48 bytes/record vs ~150 bytes for CSV. 3× storage efficiency. Also faster to write (no float-to-string conversion) and parse.

---

## Getting Started

### Building the Firmware
```bash
cd flora-pulse/firmware
arm-none-eabi-gcc --version  # Requires ARM GCC toolchain
make                          # Builds flora-pulse.elf, .bin, .hex
make flash                    # Flash via ST-Link + OpenOCD
```

### Running the App
```bash
cd flora-pulse/app
npm install                   # Install dependencies
npx react-native run-android  # Build and install on Android
# or
npx react-native run-ios      # Build and install on iOS
```

### Opening the KiCad Project
Open `kicad/device.kicad_pro` in KiCad 7+ to view the schematic and PCB layout.

---

## License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

All designs, firmware, code, and documentation by **jayis1**.  
Copyright © 2026 jayis1. All rights reserved.