# Vortex-SDR Phase 1: Conceptual Architecture

## 1. Purpose

The Vortex-SDR is a portable, handheld software-defined radio spectrum analyzer covering 100 kHz to 6 GHz. It provides professional-grade RF analysis capabilities in a battery-powered form factor, enabling field engineers, ham radio operators, IoT developers, and security researchers to visualize, measure, and characterize RF signals without a benchtop instrument.

### Problem Statement

Existing portable spectrum analyzers (e.g., TinySA, HackRF) are either limited in frequency range, lack real-time FFT processing, or require a host PC for display. The Vortex-SDR closes this gap with:

- **6 GHz coverage** in a single handheld device
- **On-board real-time FFT** via FPGA co-processor (no host PC required)
- **Touch display** with waterfall for visual signal hunting
- **BLE + USB** for remote control and data export
- **Battery-powered** operation (6+ hours continuous sweep)

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority |
|---|---|---|
| FR-01 | Receive RF signals from 100 kHz to 6 GHz with ≤ 1 Hz tuning step | Must |
| FR-02 | Display real-time FFT spectrum (256 to 8K points) on integrated TFT | Must |
| FR-03 | Waterfall display with configurable time depth (1–60 seconds) | Must |
| FR-04 | Peak detection with up to 10 markers | Must |
| FR-05 | Sweep speed ≤ 50 ms for full 6 GHz band, ≤ 5 ms per 100 MHz | Must |
| FR-06 | BLE 5.0 for companion app control and data streaming | Must |
| FR-07 | USB-C CDC-ACM for command interface + bulk transfer for data | Must |
| FR-08 | Store up to 1000 spectrum captures to on-board flash | Should |
| FR-09 | Battery-powered operation ≥ 6 hours continuous | Must |
| FR-10 | AM/FM demodulation audio output via BLE | Should |
| FR-11 | Zero-span mode for time-domain analysis | Must |
| FR-12 | Touch screen UI with 5-button navigation backup | Must |
| FR-13 | Programmable attenuator (0/10/20 dB) for high-power signals | Must |
| FR-14 | AGC with manual override (0–60 dB range) | Should |
| FR-15 | Export captures as CSV/JSON via USB | Should |

### 2.2 Non-Functional Requirements

| ID | Requirement | Target |
|---|---|---|
| NFR-01 | BOM cost (1 qty) | ≤ $90 |
| NFR-02 | Board size | ≤ 120 × 75 mm |
| NFR-03 | Operating temperature | -20°C to +60°C |
| NFR-04 | Power consumption (continuous sweep) | ≤ 2.5W |
| NFR-05 | Power consumption (standby) | ≤ 50 mW |
| NFR-06 | Phase noise (10 kHz offset, 1 GHz) | ≤ -90 dBc/Hz |
| NFR-07 | Noise figure (system) | ≤ 8 dB |
| NFR-08 | Spurious-free dynamic range | ≥ 70 dB |
| NFR-09 | Boot time (cold start) | ≤ 3 seconds |
| NFR-10 | USB connection time | ≤ 2 seconds |

## 3. Block Diagram

```
 ┌───────────────────────────────────────────────────────────────────────┐
 │                    VORTEX-SDR TOP-LEVEL ARCHITECTURE                  │
 │                                                                       │
 │  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌────────────────┐ │
 │  │ SMA RF   │    │  LNA     │    │  Mixer   │    │   SAW Filter  │ │
 │  │ Input    │───▶│SPF5189Z │───▶│ ADL5801  │───▶│ 10.7 MHz IF   │ │
 │  │ 50Ω      │    │ NF 0.6dB│    │100k-6GHz │    │ BW 200kHz     │ │
 │  └──────────┘    └──────────┘    └────┬─────┘    └──────┬─────────┘ │
 │                                        │                  │           │
 │                                  ┌─────▼─────┐    ┌──────▼───────┐   │
 │                                  │  ADF4351   │    │  AD9645      │   │
 │                                  │  PLL+VCO   │    │  14b ADC     │   │
 │                                  │  LO Gen    │    │  61.44 MSPS  │   │
 │                                  │            │    │              │   │
 │                                  │  ADF5002   │    │  OPA364      │   │
 │                                  │  /2 Pre    │    │  ADC driver  │   │
 │                                  │  4.4-6G    │    │              │   │
 │                                  └─────┬─────┘    └──────┬───────┘   │
 │                                        │                  │ LVDS      │
 │  ┌─────────────────────────────────────┼──────────────────┼────────┐  │
 │  │             MAIN PROCESSING DOMAIN  │                  │        │  │
 │  │                                     │            ┌─────▼──────┐ │  │
 │  │  ┌───────────┐   ┌─────────────────┘            │ iCE40UP5K  │ │  │
 │  │  │ STM32H743 │◀──┘                              │ FPGA       │ │  │
 │  │  │ 480 MHz   │                                  │            │ │  │
 │  │  │ Cortex-M7 │◀──SPI1──▶│ FFT engine           │ │  │
 │  │  │           │            │ Windowing             │ │  │
 │  │  │           │            │ Decimation            │ │  │
 │  │  │           │            │ Peak detect           │ │  │
 │  │  └──┬───┬────┘            └──────────────────────┘ │  │
 │  │     │   │                                           │  │
 │  │  ┌──▼┐ ┌▼──────────┐  ┌──────────┐  ┌───────────┐ │  │
 │  │  │USB│ │ILI9341    │  │nRF52832  │  │W25Q256    │ │  │
 │  │  │C  │ │2.8" TFT   │  │BLE 5.0  │  │32MB Flash │ │  │
 │  │  │HS │ │320×240    │  │UART 921k │  │SPI3       │ │  │
 │  │  └───┘ │Touch SPI  │  │          │  │           │ │  │
 │  │        └──────────┘  └──────────┘  └───────────┘ │  │
 │  │                                                  │  │
 │  │  ┌──────────────┐  ┌────────────────────────┐    │  │
 │  │  │ MCP73871     │  │ TPS62A02 Buck (3.3V)   │    │  │
 │  │  │ LiPo Charger │  │ + TLV75518 LDO (1.8V)  │    │  │
 │  │  └──────────────┘  └────────────────────────┘    │  │
 │  └──────────────────────────────────────────────────┘  │
 └───────────────────────────────────────────────────────────────────────┘
```

## 4. Bus Topology

### 4.1 SPI Bus Map

| Bus | Master | Slaves | Clock | Data Rate | Purpose |
|---|---|---|---|---|---|
| SPI1 | STM32H743 | iCE40UP5K FPGA | 50 MHz | 25 MB/s | FFT bin readback, FPGA control |
| SPI2 | STM32H743 | ILI9341 TFT, XPT2046 touch | 30 MHz | 3.75 MB/s | Display + touch |
| SPI3 | STM32H743 | W25Q256 flash | 50 MHz | 25 MB/s | Spectrum log storage |
| SPI (bit-bang) | STM32H743 | ADF4351 PLL | ≤ 25 MHz | Config only | LO frequency programming |
| SPI (config) | STM32H743 | iCE40UP5K config port | ≤ 25 MHz | Bitstream load | FPGA configuration |

### 4.2 UART Bus Map

| Bus | Baud Rate | Pins | Purpose |
|---|---|---|---|
| USART1 | 115200 | PD8/PD9 | Debug console |
| USART2 | 921600 | PA2/PA3 | BLE module (nRF52832) |

### 4.3 I2C Bus Map

| Bus | Speed | Pins | Slaves | Purpose |
|---|---|---|---|---|
| I2C1 | 400 kHz | PB8/PB9 | XPT2046 (touch) | Touch coordinate readback |

### 4.4 USB Interface

| Endpoint | Type | Direction | Max Packet | Purpose |
|---|---|---|---|---|
| EP0 | Control | Both | 64 | Standard USB requests |
| EP1 | Bulk IN | Device→Host | 512 | Spectrum data streaming |
| EP2 | Bulk OUT | Host→Device | 512 | Command input |
| EP3 | Interrupt IN | Device→Host | 64 | Status notifications |

### 4.5 Memory Map

| Region | Address Range | Size | Purpose |
|---|---|---|---|
| Flash | 0x08000000–0x081FFFFF | 1 MB | Firmware code + constants |
| ITCM | 0x00000000–0x0000FFFF | 64 KB | Time-critical code |
| DTCM | 0x20000000–0x2000FFFF | 64 KB | Stack + critical data |
| AXI SRAM | 0x24000000–0x2407FFFF | 512 KB | FFT buffers, display |
| SRAM1 | 0x30000000–0x30017FFF | 96 KB | Driver buffers |
| SRAM2 | 0x30020000–0x30027FFF | 32 KB | BLE/USB buffers |
| SRAM4 | 0x38000000–0x38000FFF | 4 KB | Backup domain |

## 5. Power Domains

### 5.1 Voltage Rails

| Rail | Voltage | Source | Current | Decoupling | Load |
|---|---|---|---|---|---|
| VSRC | 3.0–4.2V | LiPo battery | ≤ 2A | 100µF + 10µF | Charger input, buck input |
| VDD_3V3 | 3.3V ±3% | TPS62A02 buck | ≤ 1.5A | 4×100µF + 2×10µF | MCU, FPGA bank1, display, BLE, flash |
| VDD_1V8 | 1.8V ±2% | TLV75518 LDO | ≤ 500mA | 2×10µF + 100nF | ADC, FPGA bank2 |
| VDD_3V3A | 3.3V (filtered) | LC from VDD_3V3 | ≤ 100mA | 10µH + 10µF | ADC reference, DAC, LNA bias |
| VDD_RF | 3.3V (filtered) | LC from VDD_3V3 | ≤ 300mA | 10µH + 10µF | Mixer, PLL, VCO |

### 5.2 Power Sequencing Requirements

| Step | Rail | Enable | Delay | Monitor |
|---|---|---|---|---|
| 1 | VSRC | Battery insert / USB connect | — | Voltage > 3.0V |
| 2 | VDD_3V3 | TPS62A02 EN = VSRC | 2ms | PGOOD = high |
| 3 | VDD_1V8 | TLV75518 EN = VDD_3V3 | 1ms | Voltage > 1.7V |
| 4 | VDD_3V3A | MCU GPIO (LNA_EN) | 10ms | ADC ref stable |
| 5 | VDD_RF | MCU GPIO (MIXER_EN) | 5ms | PLL lock detect |
| 6 | FPGA config | MCU SPI after rail stable | 50ms | CDONE = high |

### 5.3 Current Budget by Mode

| Mode | VDD_3V3 | VDD_1V8 | Total | Duration |
|---|---|---|---|---|
| Sleep | 5 mA | 0.1 mA | 5.1 mA | Indefinite |
| Standby (display off) | 50 mA | 30 mA | 80 mA | Hours |
| Idle (display on, no sweep) | 120 mA | 30 mA | 150 mA | Hours |
| Sweeping (continuous) | 500 mA | 200 mA | 700 mA | 6+ hours |
| Peak (all on, max RF) | 1200 mA | 400 mA | 1600 mA | Minutes |

## 6. Constraints

### 6.1 Physical Constraints

- **PCB area**: 120 × 75 mm maximum (4-layer, 1.6 mm FR-4)
- **Component height**: ≤ 8 mm (to fit within handheld enclosure)
- **RF connector**: SMA edge-launch (50 Ω, female)
- **Display**: 2.8" TFT with touch (module mounted on PCB reverse side)
- **Battery**: 3.7V 3000 mAh LiPo (connected via JST-PH 2-pin)
- **USB**: USB-C 2.0 (also provides charging power)

### 6.2 Thermal Constraints

- **Maximum junction temperature**: 105°C (STM32H743), 85°C (iCE40UP5K)
- **Maximum ambient**: 60°C
- **Thermal solution**: Passive cooling only (no fan)
- **Power dissipation**: ≤ 2.5W at 60°C ambient
- **Key thermal paths**:
  - STM32H743: Package pad → inner ground plane → board edge
  - iCE40UP5K: Package pad → inner ground plane
  - AD9645: Package pad → copper pour → thermal vias
  - ADF4351: Ground paddle → via array → ground plane

### 6.3 RF Constraints

- **Input power**: ≤ +10 dBm (30 mW) maximum, LNA damage threshold +20 dBm
- **Input VSWR**: ≤ 2.0:1 (50 Ω nominal)
- **LO leakage**: ≤ -60 dBm at RF input port
- **Image rejection**: ≥ 50 dB (in-band)
- **Phase noise**: ≤ -90 dBc/Hz @ 10 kHz offset from carrier
- **Spurious**: ≤ -60 dBc in-band

### 6.4 EMC Constraints

- **FCC Part 15B**: Intentional radiator compliance (conducted + radiated)
- **ESD**: IEC 61000-4-2, ±8 kV contact, ±15 kV air (on USB/SMA ports)
- **EMI suppression**: Ferrite beads on all digital lines leaving RF section
- **Shielding**: RF section ground pour with via fence, optional shield can

### 6.5 Manufacturing Constraints

- **Minimum trace/space**: 0.10 mm (4 mil)
- **Minimum via**: 0.40 mm dia / 0.20 mm drill
- **Impedance control**: 50 Ω ±10% on RF traces (coplanar waveguide)
- **Assembly**: SMT only (no through-hole except connectors)
- **Stenciling**: 0.10 mm stencil for QFN/LGA packages
- **Inspection**: AOI required for BGA and QFN devices

## 7. Interface Specification

### 7.1 SMA RF Input

| Parameter | Value |
|---|---|
| Connector | SMA female, edge-launch, 50 Ω |
| Frequency range | 100 kHz – 6 GHz |
| Maximum input power | +10 dBm (continuous), +20 dBm (1 min) |
| Input VSWR | ≤ 2.0:1 |
| Return loss | ≤ -10 dB |

### 7.2 USB-C

| Parameter | Value |
|---|---|
| Connector | USB-C 2.0 (16-pin) |
| Data rate | HS (480 Mbps) |
| Power | 5V @ 500 mA (VBUS), also charges battery |
| ESD | TPD4E05U06 on D+/D-/CC1/CC2 |

### 7.3 User Interface

| Input | Type | Function |
|---|---|---|
| 2.8" TFT | SPI resistive touch | Primary display + touch input |
| Button UP | Tactile, active-low | Navigate up / increase value |
| Button DOWN | Tactile, active-low | Navigate down / decrease value |
| Button LEFT | Tactile, active-low | Navigate left / shift |
| Button RIGHT | Tactile, active-low | Navigate right / shift |
| Button CENTER | Tactile, active-low | Select / confirm |
| Button MENU | Tactile, active-low | Open menu / back |

### 7.4 Debug Port

| Pin | Function |
|---|---|
| SWDIO | ARM Serial Wire Debug data |
| SWCLK | ARM Serial Wire Debug clock |
| RESET | Active-low system reset |
| VREF | 3.3V reference output |
| GND | Ground |

## 8. System States

```
                    ┌──────────┐
                    │  POWER   │
                    │  OFF     │
                    └────┬─────┘
                         │ Battery insert / USB connect
                         ▼
                    ┌──────────┐
                    │  BOOT    │
                    │  INIT    │──────────────────┐
                    └────┬─────┘                  │
                         │ Init complete           │ Error
                         ▼                         │
                    ┌──────────┐              ┌─────▼─────┐
                    │  IDLE    │              │  ERROR    │
                    │  (standby│              │  (display │
                    │  display │              │  error    │
                    │  on)     │              │  message) │
                    └────┬─────┘              └───────────┘
                         │ Start sweep
                         ▼
                    ┌──────────┐
                    │  SWEEP   │
                    │  (active  │
                    │  FFT)    │
                    └────┬─────┘
                    ┌────┴────┐
                    │         │
                    ▼         ▼
              ┌──────────┐  ┌──────────┐
              │  ZERO    │  │  DATA    │
              │  SPAN    │  │  LOG     │
              │  (time   │  │  (record │
              │  domain) │  │  to flash│
              └──────────┘  └──────────┘
```

## 9. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| ADF4351 phase noise insufficient | Low | High | Add external VCO filter, use narrow loop bandwidth |
| ADC LVDS signal integrity | Medium | High | Short traces, controlled impedance, FPGA LVDS receiver |
| FPGA bitstream load failure | Low | Medium | Dual-image fallback in flash, hardware reset capability |
| Thermal throttling at 60°C ambient | Medium | Medium | Reduce FPGA clock during high-temp, add thermal pad |
| Display bandwidth limitation | Low | Low | DMA-driven SPI at 30 MHz, partial screen updates |
| BLE throughput for waterfall | Medium | Low | Compress spectrum data, use notification batching |