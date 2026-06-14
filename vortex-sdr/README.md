# Vortex-SDR — Portable Software-Defined Radio Spectrum Analyzer

> A handheld dual-channel SDR spectrum analyzer covering 100 kHz to 6 GHz with real-time FFT waterfall display, touch UI, BLE companion app, and sweep/trigger modes — for RF engineers, ham operators, and IoT developers.

## Specifications

| Parameter | Value |
|---|---|
| **Frequency Range** | 100 kHz – 6 GHz (dual-conversion superhet + direct sampling) |
| **Tuning Resolution** | 1 Hz step size (PLL fractional-N synthesizer) |
| **Instantaneous Bandwidth** | 10 MHz (ADC-limited, decimated to user BW) |
| **ADC Resolution** | 14-bit, 61.44 MSPS (AD9645) |
| **Phase Noise (10 kHz offset)** | ≤ -90 dBc/Hz @ 1 GHz carrier |
| **Dynamic Range** | ≥ 70 dB (SFDR), ≥ 60 dB noise floor to peak |
| **Sweep Speed** | ≤ 50 ms full-band (6 GHz), ≤ 5 ms per 100 MHz segment |
| **FFT Sizes** | 256, 512, 1K, 2K, 4K, 8K points |
| **FFT Window** | Rectangular, Hanning, Blackman-Harris, Flat-Top, Kaiser |
| **RF Front-End** | ADL5801 mixer (100 kHz – 6 GHz), LNA: SPF5189Z (NF ≤ 0.6 dB) |
| **LO Synthesizer** | ADF4351 (35 MHz – 4.4 GHz) + ADF5002 prescaler for 4.4–6 GHz |
| **MCU** | STM32H743ZIT6 (Cortex-M7 @ 480 MHz, 1 MB Flash, 1 MB RAM) |
| **FPGA Accelerator** | Lattice iCE40UP5K (5280 LUTs, DSP FFT co-processor) |
| **Display** | ILI9341 2.8" 320×240 TFT with resistive touch (SPI) |
| **Storage** | W25Q256 32 MB SPI NOR flash (sweep logs, screenshots) |
| **BLE** | nRF52832 module (via UART, 115.2 kbps) |
| **USB** | USB-C 2.0 HS (CDC-ACM + bulk transfer, 480 Mbps PHY) |
| **Power** | 3.7 V 3000 mAh LiPo, MCP73871 charger, TPS62A02 3.3V buck |
| **Battery Life** | ≥ 6 hours continuous sweep, ≥ 12 hours standby |
| **Board Size** | 120 × 75 mm (4-layer, 1.6 mm) |
| **Operating Temp** | -20°C to +60°C (extended) |
| **PCB** | 4-layer FR-4 370HR, ENIG, impedance-controlled 50 Ω |
| **BOM Cost (1 qty)** | ≈ $87 |

## Directory Structure

```
vortex-sdr/
├── phase1_conceptual_architecture.md   # System architecture, block diagram, data flow
├── phase2_component_selection_schematics.md  # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md     # Stackup, routing rules, thermal, DFM
├── phase4_software_stack.md            # Boot, drivers, FFT, RTOS, protocol
├── kicad/
│   ├── device.kicad_pro                # KiCad project (net classes, design rules)
│   ├── device.kicad_sch               # Full schematic with RF, MCU, FPGA, display
│   └── device.kicad_pcb               # Board layout, placement, zones
├── firmware/
│   ├── Makefile                        # ARM GCC cross-compile build
│   ├── main.c                          # SPL entry point, init, sweep engine, FFT
│   ├── board.h                         # Pin definitions, constants, config structs
│   ├── registers.h                     # MMIO register map (STM32H743)
│   ├── usb_descriptors.h               # USB CDC-ACM + bulk transfer descriptors
│   └── drivers/
│       ├── adf4351.h / adf4351.c        # PLL synthesizer driver (SPI, fractional-N)
│       ├── ad9645.h / ad9645.c          # ADC driver (SPI config, LVDS data capture)
│       └── ili9341.h / ili9341.c        # TFT display driver (SPI, DMA, font engine)
├── app/
│   ├── package.json                    # React Native dependencies
│   ├── App.js                          # Navigation root
│   ├── screens/
│   │   ├── SpectrumScreen.js           # Live spectrum & waterfall
│   │   ├── SweepScreen.js              # Sweep config, markers, triggers
│   │   └── SettingsScreen.js           # Device, BLE, display, storage
│   ├── components/
│   │   ├── VortexContext.js            # BLE connection state provider
│   │   ├── WaterfallChart.js           # Canvas-based waterfall rendering
│   │   └── StatusCard.js               # Reusable status card component
│   └── utils/
│       └── protocol.js                 # Binary wire protocol (CRC-32, frame parser)
└── README.md                           # This file
```

## Block Diagram

```
 ┌─────────────────────────────────────────────────────────────────────────┐
 │                         VORTEX-SDR BLOCK DIAGRAM                       │
 │                                                                         │
 │  ┌───────────┐   ┌──────────┐   ┌───────────┐   ┌──────────────┐      │
 │  │  SMA RF   │──▶│  LNA     │──▶│  Mixer    │──▶│  IF Filter   │      │
 │  │  Input    │   │ SPF5189Z │   │ ADL5801   │   │  SAW 10.7MHz │      │
 │  └───────────┘   └──────────┘   └─────┬─────┘   └──────┬───────┘      │
 │                                       │ LO             │ IF            │
 │                                 ┌─────▼─────┐   ┌─────▼───────┐      │
 │                                 │   ADF4351  │   │  AD9645     │      │
 │                                 │  PLL+VCO   │   │  ADC 14b    │      │
 │                                 │  35M–4.4G  │   │  61.44 MSPS │      │
 │                                 └─────┬─────┘   └─────┬───────┘      │
 │                                       │               │ LVDS         │
 │  ┌────────────────────────────────────┼───────────────┼──────────┐   │
 │  │                                    │               │          │   │
 │  │  ┌───────────┐   ┌────────────────▼───────────────▼───┐     │   │
 │  │  │ STM32H743 │◀──│  iCE40UP5K FPGA                   │     │   │
 │  │  │  MCU      │   │  (FFT co-processor, decimation,    │     │   │
 │  │  │  480 MHz  │   │   LVDS capture, windowing)         │     │   │
 │  │  └──┬───┬────┘   └────────────────────────────────────┘     │   │
 │  │     │   │              SPI data path to MCU                   │   │
 │  │     │   │                                                       │   │
 │  │  ┌──▼┐ ┌▼──────────────┐  ┌──────────┐  ┌────────────┐      │   │
 │  │  │USB│ │ ILI9341 TFT   │  │ nRF52832 │  │ W25Q256    │      │   │
 │  │  │C  │ │ 2.8" Touch    │  │ BLE Mod  │  │ 32MB Flash │      │   │
 │  │  │   │ │ 320×240       │  │          │  │            │      │   │
 │  │  └───┘ └──────────────┘  └──────────┘  └────────────┘      │   │
 │  │                                                               │   │
 │  │  ┌──────────────────┐  ┌──────────────────────────────────┐  │   │
 │  │  │ MCP73871 Charger │  │ TPS62A02 Buck + TLV75533 LDO      │  │   │
 │  │  │ USB → LiPo       │  │ 5V → 3.3V (MCU/FPGA/RF)         │  │   │
 │  │  └──────────────────┘  │ + TLV75518 LDO → 1.8V (ADC/FPGA)│  │   │
 │  │                         └──────────────────────────────────┘  │   │
 │  └───────────────────────────────────────────────────────────────┘   │
 │                                                                       │
 │  Power: 3.7V LiPo → 3.3V @ 1.5A (buck) + 1.8V @ 500mA (LDO)       │
 │  RF path: SMA → LNA → Mixer → IF SAW → ADC → FPGA FFT → MCU       │
 └─────────────────────────────────────────────────────────────────────────┘
```

## Pin Assignments (STM32H743ZIT6)

### GPIO Pin Map

| Pin | Net | Function | Domain |
|---|---|---|---|
| PA0 | NET_ADC_IRQ | ADC data-ready interrupt | 3V3 |
| PA1 | NET_LNA_EN | LNA enable (active-high) | 3V3 |
| PA2 | NET_USART2_TX | BLE UART TX | 3V3 |
| PA3 | NET_USART2_RX | BLE UART RX | 3V3 |
| PA4 | NET_DAC1_OUT | AGC control voltage (0-3.3V) | 3V3A |
| PA5 | NET_SPI1_SCK | FPGA SPI clock | 3V3 |
| PA6 | NET_SPI1_MISO | FPGA SPI data out | 3V3 |
| PA7 | NET_SPI1_MOSI | FPGA SPI data in | 3V3 |
| PA8 | NET_MCO1 | 8 MHz reference output (to ADF4351) | 3V3 |
| PA9 | NET_USB_OTG_VBUS | USB VBUS detect | 5V0 |
| PA10 | NET_USB_OTG_ID | USB OTG ID | 5V0 |
| PA11 | NET_USB_DM | USB D- | 3V3 |
| PA12 | NET_USB_DP | USB D+ | 3V3 |
| PA13 | NET_JTMS | SWDIO | 3V3 |
| PA14 | NET_JTCK | SWCLK | 3V3 |
| PA15 | NET_FPGA_CDONE | FPGA config done | 3V3 |
| PB0 | NET_MIXER_EN | Mixer enable (active-high) | 3V3 |
| PB1 | NET_ATTEN_SEL | Programmable attenuator select | 3V3 |
| PB2 | NET_SPI2_SCK | Display SPI clock | 3V3 |
| PB3 | NET_SPI2_MOSI | Display SPI data | 3V3 |
| PB4 | NET_LCD_DC | Display data/command | 3V3 |
| PB5 | NET_LCD_CS | Display chip select (active-low) | 3V3 |
| PB6 | NET_LCD_RST | Display reset (active-low) | 3V3 |
| PB7 | NET_TOUCH_IRQ | Resistive touch interrupt | 3V3 |
| PB8 | NET_I2C1_SCL | Touch I2C clock | 3V3 |
| PB9 | NET_I2C1_SDA | Touch I2C data | 3V3 |
| PB10 | NET_PLL_LE | ADF4351 latch enable | 3V3 |
| PB11 | NET_PLL_CE | ADF4351 chip enable | 3V3 |
| PB12 | NET_SPI3_SCK | Flash SPI clock | 3V3 |
| PB13 | NET_SPI3_MISO | Flash SPI data out | 3V3 |
| PB14 | NET_SPI3_MOSI | Flash SPI data in | 3V3 |
| PB15 | NET_FLASH_CS | Flash chip select (active-low) | 3V3 |
| PC0 | NET_ADC_CLK_P | LVDS ADC clock+ | 1V8 |
| PC1 | NET_ADC_CLK_N | LVDS ADC clock- | 1V8 |
| PC2 | NET_ADC_D0_P | LVDS ADC data bit 0+ | 1V8 |
| PC3 | NET_ADC_D0_N | LVDS ADC data bit 0- | 1V8 |
| PC4 | NET_ADC_D1_P | LVDS ADC data bit 1+ | 1V8 |
| PC5 | NET_ADC_D1_N | LVDS ADC data bit 1- | 1V8 |
| PC6 | NET_FPGA_RST | FPGA reset (active-low) | 3V3 |
| PC7 | NET_FPGA_CS | FPGA SPI chip select | 3V3 |
| PC8 | NET_TOUCH_CS | Touch chip select (active-low) | 3V3 |
| PC9 | NET_LED_STATUS | Green status LED | 3V3 |
| PC10 | NET_LED_ERROR | Red error LED | 3V3 |
| PC11 | NET_CHG_STAT1 | Charger status 1 | 3V3 |
| PC12 | NET_CHG_STAT2 | Charger status 2 | 3V3 |
| PD0 | NET_FPGA_SCK | FPGA config SPI clock | 3V3 |
| PD1 | NET_FPGA_SI | FPGA config SPI data | 3V3 |
| PD2 | NET_FPGA_SS | FPGA config SPI select | 3V3 |
| PD3 | NET_BAT_SENSE | Battery voltage ADC input | 3V3A |
| PD4 | NET_TEMP_SENSE | Board temperature ADC input | 3V3A |
| PD5 | NET_MUX_SEL0 | RF path mux select 0 | 3V3 |
| PD6 | NET_MUX_SEL1 | RF path mux select 1 | 3V3 |
| PD7 | NET_MUX_EN | RF path mux enable | 3V3 |
| PD8 | NET_USART1_TX | Debug UART TX | 3V3 |
| PD9 | NET_USART1_RX | Debug UART RX | 3V3 |
| PE0 | NET_BTN_CENTER | Center button (active-low) | 3V3 |
| PE1 | NET_BTN_UP | Up button (active-low) | 3V3 |
| PE2 | NET_BTN_DOWN | Down button (active-low) | 3V3 |
| PE3 | NET_BTN_LEFT | Left button (active-low) | 3V3 |
| PE4 | NET_BTN_RIGHT | Right button (active-low) | 3V3 |
| PE5 | NET_BTN_MENU | Menu button (active-low) | 3V3 |

### Bus Assignments

| Bus | Peripheral | Pins | Clock | Purpose |
|---|---|---|---|---|
| SPI1 | Master, 50 MHz | PA5/PA6/PA7 | PLL1Q (50 MHz) | FPGA data & control |
| SPI2 | Master, 30 MHz | PB2/PB3 + PB5 | PLL1Q (50 MHz) | TFT display |
| SPI3 | Master, 50 MHz | PB12/PB13/PB14 + PB15 | PLL1Q (50 MHz) | Flash storage |
| I2C1 | Master, 400 kHz | PB8/PB9 | HSI (16 MHz) | Touch controller |
| USART1 | 115200 baud | PD8/PD9 | APB1 | Debug console |
| USART2 | 921600 baud | PA2/PA3 | APB1 | BLE module |
| USB_OTG_FS | Device, HS | PA11/PA12 | HSI48 | USB CDC + bulk |
| ADC1 | 12-bit, 1 Msps | PA4/PD3/PD4 | HCLK/4 | AGC, battery, temp |
| DAC1 | 12-bit, 1 Msps | PA4 | HCLK/4 | AGC voltage control |

## Power Architecture

```
 ┌──────────┐     ┌───────────┐     ┌──────────┐
 │  USB-C   │────▶│ MCP73871  │────▶│ 3.7V     │
 │  5V/2A   │     │ Charger   │     │ LiPo     │
 │          │     │           │     │ 3000mAh  │
 └──────────┘     └───────────┘     └────┬─────┘
                                          │ VSRC (3.0–4.2V)
                                    ┌─────▼──────────┐
                                    │  TPS62A02       │
                                    │  Buck 3.3V/2A   │
                                    └─────┬───────────┘
                                          │ VDD_3V3
                              ┌───────────┼───────────────┐
                              │           │               │
                        ┌─────▼────┐ ┌────▼────┐  ┌──────▼──────┐
                        │ MCU +   │ │ FPGA +  │  │ Display +   │
                        │ Periph  │ │ ADC     │  │ BLE + Flash │
                        │ ≤500mA  │ │ ≤600mA  │  │ ≤100mA      │
                        └──────────┘ └────┬────┘  └─────────────┘
                                          │
                                    ┌─────▼──────────┐
                                    │  TLV75518      │
                                    │  LDO 1.8V/500mA│
                                    └─────┬───────────┘
                                          │ VDD_1V8
                                    ┌─────▼──────────┐
                                    │ ADC (LVDS) +  │
                                    │ FPGA I/O Bank │
                                    │ ≤300mA        │
                                    └────────────────┘
```

### Power Budget

| Rail | Voltage | Current (typ) | Current (peak) | Source | Components |
|---|---|---|---|---|---|
| VSRC | 3.0–4.2V | — | 2.0A | LiPo | Battery |
| VDD_3V3 | 3.3V ±3% | 800mA | 1.5A | TPS62A02 buck | MCU, FPGA, display, RF, BLE, flash |
| VDD_1V8 | 1.8V ±2% | 200mA | 500mA | TLV75518 LDO | ADC LVDS, FPGA bank 2 |
| VDD_3V3A | 3.3V (analog) | 50mA | 100mA | LC filter from VDD_3V3 | ADC ref, DAC, LNA bias |
| VDD_RF | 3.3V (RF) | 120mA | 300mA | LC filter from VDD_3V3 | Mixer, PLL, VCO |

### Power Sequencing

1. VSRC rises (battery or USB) — no sequencing required
2. TPS62A02 EN tied to VSRC — VDD_3V3 comes up with ~2ms soft-start
3. TLV75518 EN tied to VDD_3V3 — VDD_1V8 follows ~1ms later
4. FPGA configuration begins after VDD_3V3 + VDD_1V8 stable (~5ms total)
5. MCU starts after POR — ~3ms boot to first instruction
6. RF front-end (LNA, mixer) enabled by MCU GPIO after full init (~50ms)

## BOM Summary

| Ref | Component | Part Number | Package | Qty | Unit Cost | Purpose |
|---|---|---|---|---|---|---|
| U1 | MCU | STM32H743ZIT6 | LQFP-144 | 1 | $18.50 | Main processor |
| U2 | FPGA | iCE40UP5K-SG48 | QFN-48 | 1 | $5.90 | FFT co-processor |
| U3 | ADC | AD9645BCZ-80 | LQFP-64 | 1 | $12.80 | 14-bit 61.44 MSPS |
| U4 | Mixer | ADL5801ACPZ | LFCSP-24 | 1 | $8.40 | 100 kHz–6 GHz mixer |
| U5 | PLL | ADF4351BCPZ | LFCSP-32 | 1 | $9.20 | LO synthesizer |
| U6 | LNA | SPF5189Z | SOT-89 | 1 | $0.80 | Low-noise amplifier |
| U7 | Buck regulator | TPS62A02VLSR | VSON-8 | 1 | $1.20 | 3.3V/2A buck |
| U8 | LDO | TLV75518PDBV | SOT-23-5 | 1 | $0.35 | 1.8V/500mA LDO |
| U9 | BLE module | nRF52832-MK | QFN-48 | 1 | $4.50 | BLE 5.0 radio |
| U10 | Display | ILI9341 TFT | 2.8" module | 1 | $6.00 | 320×240 touch LCD |
| U11 | Flash | W25Q256JVEQ | SOP-8 | 1 | $1.80 | 32 MB SPI NOR |
| U12 | Charger | MCP73871T-2CCI | MSOP-10 | 1 | $1.50 | LiPo charger |
| U13 | SAW filter | TA1099EC8110 | SMD 3×3 | 1 | $2.10 | 10.7 MHz IF filter |
| U14 | Prescaler | ADF5002BCPZ | LFCSP-8 | 1 | $3.80 | 4.4–6 GHz /2 prescaler |
| U15 | Op-amp | OPA364AIDBVT | SOT-23-5 | 1 | $0.55 | ADC driver/buffer |
| U16 | ESD protection | TPD4E05U06DQAR | USON-10 | 1 | $0.45 | USB ESD clamp |
| Y1 | Crystal | 8.000 MHz | HC49/SMD | 1 | $0.30 | MCU HSE oscillator |
| Y2 | Crystal | 32.768 kHz | ABS07 | 1 | $0.25 | MCU LSE oscillator |
| Y3 | TCXO | 61.44 MHz | SMD 5×3.2 | 1 | $2.80 | ADC sample clock |
| Y4 | TCXO | 25.000 MHz | SMD 5×3.2 | 1 | $1.90 | PLL reference |
| **Total** | | | | | **$87.00** | |

## Operating Modes

### Spectrum Analyzer Mode
- Continuous frequency sweep with configurable start/stop frequency
- Real-time FFT display with peak detection and markers
- Waterfall display (time vs. frequency vs. amplitude)
- Resolution bandwidth: 100 Hz to 10 MHz (FFT size dependent)
- Reference level: -120 dBm to +20 dBm
- Scale: 1, 2, 5, 10 dB/division

### Zero-Span Mode
- Fixed-frequency amplitude vs. time
- Sweep time: 10 µs to 10 s
- AM/FM demodulation view

### Tracking Generator Mode (with external directional coupler)
- Scalar network analysis
- Return loss measurement with external bridge

### Signal Identifier Mode
- Harmonic identification
- Image rejection analysis
- Spurious signal hunting

### Data Logger Mode
- Continuous spectrum capture to flash
- Configurable capture interval (1 s to 1 hour)
- Auto-export via USB or BLE

## RF Signal Path

### Receive Path (100 kHz – 6 GHz)

```
SMA Input (50Ω)
    │
    ▼
┌────────────┐     ┌──────────────┐     ┌─────────────────┐
│  RF MUX    │────▶│  LNA         │────▶│  Attenuator     │
│  (3 paths) │     │  SPF5189Z    │     │  0/10/20 dB     │
│  0–6GHz    │     │  NF=0.6dB    │     │  PIN diode      │
│  3–6GHz    │     │  G=+18dB     │     │                 │
│  0–3GHz    │     └──────────────┘     └────────┬────────┘
└────────────┘                                    │
                                                  ▼
                                         ┌─────────────────┐
                                         │  Mixer          │
                                         │  ADL5801        │
                                         │  100kHz–6GHz    │
                                         │  IF=10.7MHz     │
                                         └────────┬────────┘
                                                  │ LO from ADF4351
                                                  ▼
                                         ┌─────────────────┐
                                         │  SAW Filter     │
                                         │  10.7MHz ±BW    │
                                         │  TA1099EC8110   │
                                         └────────┬────────┘
                                                  │
                                                  ▼
                                         ┌─────────────────┐
                                         │  ADC Driver    │
                                         │  OPA364        │
                                         │  Single-ended  │
                                         └────────┬────────┘
                                                  │
                                                  ▼
                                         ┌─────────────────┐
                                         │  ADC            │
                                         │  AD9645 14-bit  │
                                         │  61.44 MSPS    │
                                         └────────┬────────┘
                                                  │ LVDS
                                                  ▼
                                         ┌─────────────────┐
                                         │  FPGA          │
                                         │  iCE40UP5K     │
                                         │  FFT + Window  │
                                         └────────┬────────┘
                                                  │ SPI
                                                  ▼
                                         ┌─────────────────┐
                                         │  MCU           │
                                         │  STM32H743     │
                                         │  Display/USB  │
                                         └─────────────────┘
```

### LO Generation

| Frequency Range | LO Path | Configuration |
|---|---|---|
| 100 kHz – 3 GHz | ADF4351 direct output | IF = f_LO - f_RF, f_LO = f_RF + 10.7 MHz |
| 3 GHz – 4.4 GHz | ADF4351 direct output | IF = f_LO - f_RF, f_LO = f_RF + 10.7 MHz |
| 4.4 GHz – 6 GHz | ADF4351 + ADF5002 /2 prescaler | ADF4351 outputs 2×LO, ADF5002 divides by 2 |

### Frequency Plan (IF = 10.7 MHz)

| Band | RF Range | LO Range | LO Source | Image Frequency |
|---|---|---|---|---|
| VLF/LF | 0.1–0.5 MHz | 10.8–11.2 MHz | ADF4351 | 21.5–22.9 MHz |
| MF/HF | 0.5–30 MHz | 11.2–40.7 MHz | ADF4351 | 22.9–51.4 MHz |
| VHF | 30–300 MHz | 40.7–310.7 MHz | ADF4351 | 51.4–621.4 MHz |
| UHF | 300–3000 MHz | 310.7–3010.7 MHz | ADF4351 | 621.4–6021.4 MHz |
| L/S | 1–3 GHz | 1010.7–3010.7 MHz | ADF4351 | 2021.4–6021.4 MHz |
| C-low | 3–4.4 GHz | 3010.7–4410.7 MHz | ADF4351 | 6021.4–8821.4 MHz |
| C-high | 4.4–6 GHz | 4410.7–6010.7 MHz | ADF4351 + ADF5002 | 8821.4–12021.4 MHz |

## Use Cases

### 1. RF Engineer — Spectrum Monitoring
Scan for interference, identify unauthorized transmitters, verify signal levels. The 6 GHz range covers all major cellular bands, Wi-Fi (2.4/5/6 GHz), and IoT protocols.

### 2. Ham Radio Operator — Band Monitoring
Monitor VHF/UHF bands for activity. Track repeater signals, measure antenna return loss, identify harmonics. The 1 Hz tuning resolution enables precise frequency selection.

### 3. IoT Developer — Regulatory Compliance
Verify transmit power, spurious emissions, and occupied bandwidth for FCC/ETSI compliance. Data logger mode captures long-term spectrum occupancy.

### 4. Security Researcher — RF Surveillance
Detect and locate hidden cameras, GPS trackers, and rogue access points. The waterfall display reveals burst transmissions that swept analyzers miss.

### 5. Educator — RF Fundamentals
Visualize AM/FM modulation, harmonics, and filter responses in real-time. The companion app enables remote classroom demonstrations via BLE.

## Build Instructions

### Prerequisites

```bash
# ARM toolchain for STM32H7
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# iCE40 toolchain for FPGA
sudo apt install fpga-icestorm yosys nextpnr-ice40

# KiCad 7+ for schematic/PCB editing
sudo apt install kicad

# React Native for companion app
npm install -g react-native-cli
```

### Firmware Build

```bash
cd firmware/
make clean && make -j$(nproc)

# Flash via ST-Link
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program build/vortex-sdr.elf verify reset exit"

# Or via USB DFU (hold BOOT0 at reset)
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/vortex-sdr.bin
```

### FPGA Bitstream Build

```bash
cd fpga/
make clean && make -j$(nproc)

# Flash via MCU SPI (automatic at boot)
# The MCU loads the bitstream from flash to FPGA at startup
```

### Companion App Build

```bash
cd app/
npm install

# Android
npx react-native run-android

# iOS
npx react-native run-ios
```

### PCB Fabrication

Order the 4-layer board with these specs:
- FR-4 370HR substrate, 1.6 mm total thickness
- ENIG surface finish (1 µ" Au over 3-5 µ" Ni)
- 50 Ω controlled impedance on L1/L4 (coplanar waveguide for RF)
- Minimum trace/space: 0.10/0.10 mm
- Minimum via: 0.40 mm dia / 0.20 mm drill
- Solder mask: Green, Silkscreen: White

### Assembly Notes

1. **FPGA**: Solder iCE40UP5K first — requires SPI flash configuration
2. **RF path**: Keep SMA-to-LNA trace under 5 mm, 50 Ω impedance matched
3. **ADC**: Place close to SAW filter output, minimize trace length on IF path
4. **Power**: Use star-point grounding from TPS62A02 output
5. **Shield**: Add RF shield cans over mixer + PLL + LNA section if needed

## Connecting via USB (Linux)

The Vortex-SDR enumerates as a composite USB device:
- **CDC-ACM** (ttyACM0): Command interface for configuration
- **Bulk endpoint**: Spectrum data streaming at up to 12 MB/s

```bash
# Connect to command interface
picocom /dev/ttyACM0 -b 921600

# Send a sweep command (binary protocol, see protocol.js)
# Start frequency: 88 MHz, Stop frequency: 108 MHz, RBW: 10 kHz
echo -ne '\x01\x02\x00\x00\x58\x00\x00\x00\x6c\x00\x00\x00\x10\x00' > /dev/ttyACM0
```

## BLE Protocol

The nRF52832 module exposes a GATT server with these characteristics:

| Service | UUID | Characteristic | Description |
|---|---|---|---|
| Vortex Main | 0xFEF1 | 0xFEF2 (write) | Command input |
| Vortex Main | 0xFEF1 | 0xFEF3 (notify) | Spectrum data output |
| Vortex Main | 0xFEF1 | 0xFEF4 (read) | Device status |
| Device Info | 0x180A | Standard | Manufacturer, firmware version |

## Firmware Architecture

### Task Model (bare-metal super-loop with interrupt-driven DMA)

| Priority | Module | Trigger | Period | Description |
|---|---|---|---|---|
| 0 (highest) | ADC DMA | DMA half/full complete | 61.44 MSPS / decimation | Capture ADC samples |
| 1 | FPGA SPI | Timer ISR (10 ms) | 10 ms | Read FFT bins from FPGA |
| 2 | PLL SPI | Command | On-demand | Tune LO frequency |
| 3 | Display | Timer ISR (50 ms) | 50 ms | Update spectrum/waterfall |
| 4 | BLE UART | DMA RX | Async | BLE command/response |
| 5 | USB CDC | USB ISR | Async | USB command/data |
| 6 | Flash | Background | On-demand | Log spectrum data |
| 7 (lowest) | UI | Button/touch ISR | On-event | Handle user input |

### Boot Sequence

1. **Reset** → MCU POR, stack pointer loaded from vector table
2. **clock_init()** → HSE 8 MHz → PLL1 @ 480 MHz, PLL2 @ 200 MHz, PLL3 @ 48 MHz
3. **gpio_init()** → Configure all GPIO pins per pin assignment table
4. **power_init()** → Enable LNA, mixer, PLL power rails (with 10ms ramp)
5. **fpga_init()** → Load bitstream from flash via SPI, wait for CDONE
6. **adc_init()** → Configure AD9645 via SPI, enable LVDS output mode
7. **pll_init()** → Set ADF4351 to default 1 GHz center frequency
8. **display_init()** → Initialize ILI9341, clear screen, draw boot logo
9. **ble_init()** → Reset nRF52832, configure UART at 921600 baud
10. **usb_init()** → Configure USB OTG FS in device mode with CDC+bulk
11. **Main loop** → Sweep engine processes FFT bins, updates display, handles commands

## License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| FPGA bitstream (Verilog) | CERN-OHL-S v2 |
| React Native companion app | MIT |

## Acknowledgments

- ADI (Analog Devices) for AD9645, ADL5801, and ADF4351 documentation
- STMicroelectronics for STM32H7 HAL reference
- Lattice Semiconductor for iCE40 open toolchain support
- u-blox for GNSS RF design application notes

## Contact

For questions, issues, or contributions, open an issue on GitHub or reach out on the community forum.

---

*Vortex-SDR — See the invisible. Hear the silent. Understand the spectrum.*