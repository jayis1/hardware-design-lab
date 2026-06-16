# Phase 2: Component Selection & Schematics — Chronos Pulser

## 1. Complete Bill of Materials (BOM)

### 1.1 Major Integrated Circuits

| Designator | Part Number | Manufacturer | Description | Package | Qty | Unit Cost | Ext. Cost |
|------------|-------------|--------------|-------------|---------|-----|-----------|-----------|
| U1 | LFE5U-45F-6BG381C | Lattice Semiconductor | ECP5 FPGA, 43.2K LUTs, 381-caBGA | caBGA-381 | 1 | $22.50 | $22.50 |
| U2 | FT601Q-B-T | FTDI / Bridgetek | USB 3.0 to 32-bit FIFO bridge | QFN-76 | 1 | $8.20 | $8.20 |
| U3 | AD9230BCPZ-250 | Analog Devices | 12-bit 250 MSPS ADC, LVDS output | LFCSP-56 | 1 | $42.00 | $42.00 |
| U4 | LMH6517SQ/NOPB | Texas Instruments | Variable Gain Amplifier, 1.2 GHz BW | WQFN-32 | 1 | $11.50 | $11.50 |
| U5 | MT41K128M16JT-125:K | Micron | DDR3L SDRAM, 128M×16, 256 MB | FBGA-96 | 1 | $4.80 | $4.80 |
| U6 | TPS63070RNMR | Texas Instruments | Buck-Boost converter, 3.3V output | VQFN-15 | 1 | $2.40 | $2.40 |
| U7 | TPS65131RGER | Texas Instruments | Dual-output boost (+5V, -5V) | VQFN-24 | 1 | $3.10 | $3.10 |
| U8 | LP5907SNX-1.2/NOPB | Texas Instruments | LDO 1.2V, 250 mA, ultra-low noise | DSBGA-4 | 1 | $0.65 | $0.65 |
| U9 | ADP7118ARDZ-1.8-R7 | Analog Devices | LDO 1.8V, 200 mA, low noise | SOIC-8 | 1 | $1.80 | $1.80 |
| U10 | TPS51200DRCR | Texas Instruments | DDR3 termination regulator | VSON-10 | 1 | $1.20 | $1.20 |
| U11 | SiT9365AI-2B2-33E250.000000 | SiTime | 250 MHz LVDS oscillator, ±25 ppm | SOT23-5 (5×3.2) | 1 | $6.50 | $6.50 |
| U12 | TMP117AIDRVR | Texas Instruments | ±0.1°C digital temp sensor, I²C | WSON-6 | 1 | $2.80 | $2.80 |
| U13 | W25Q128JVSIQ | Winbond | 128 Mbit SPI NOR flash | SOIC-8 | 1 | $1.20 | $1.20 |
| U14 | TPD4E05U06DQAR | Texas Instruments | 4-ch ESD protection, USB 3.0 | USON-10 | 1 | $0.45 | $0.45 |
| U15 | MAX16054AZT+T | Maxim / ADI | Push-button on/off controller | TSOT-6 | 1 | $1.10 | $1.10 |

### 1.2 RF / Pulse Generation Components

| Designator | Part Number | Manufacturer | Description | Package | Qty | Unit Cost | Ext. Cost |
|------------|-------------|--------------|-------------|---------|-----|-----------|-----------|
| D1 | MA44769-287T | MACOM | Step Recovery Diode, 65 ps transition | SOT-23 | 1 | $18.00 | $18.00 |
| Q1 | BFR92A,215 | Nexperia | NPN wideband transistor, 5 GHz fT | SOT-23 | 1 | $0.80 | $0.80 |
| T1 | TCD-10-1X+ | Mini-Circuits | Directional coupler, 10 dB, 5–1000 MHz | SMT-6 (0.27×0.31") | 1 | $4.95 | $4.95 |
| L1 | 0402CS-2N2XJLW | Coilcraft | 2.2 nH chip inductor, SRD pulse shaping | 0402 | 1 | $0.35 | $0.35 |
| L2 | 0402CS-8N2XJLW | Coilcraft | 8.2 nH chip inductor, SRD pulse shaping | 0402 | 1 | $0.35 | $0.35 |
| C1 | 0402N1R0C500CT | Walsin | 1.0 pF NP0, SRD pulse shaping | 0402 | 1 | $0.08 | $0.08 |
| C2 | 0402N2R2C500CT | Walsin | 2.2 pF NP0, SRD pulse shaping | 0402 | 1 | $0.08 | $0.08 |
| C3 | GCM1555C1H100JA16D | Murata | 10 pF NP0, DC block coupling | 0402 | 1 | $0.10 | $0.10 |
| R1 | ERA-3AEB102V | Panasonic | 1 kΩ 0.1%, SRD bias resistor | 0603 | 1 | $0.15 | $0.15 |
| R2 | ERA-3AEB4990V | Panasonic | 499 Ω 0.1%, pulse gen termination | 0603 | 1 | $0.15 | $0.15 |
| R3 | ERA-3AEB49R9V | Panasonic | 49.9 Ω 0.1%, impedance matching | 0603 | 1 | $0.15 | $0.15 |

### 1.3 Connectors

| Designator | Part Number | Manufacturer | Description | Qty | Unit Cost | Ext. Cost |
|------------|-------------|--------------|-------------|-----|-----------|-----------|
| J1 | 12401598E4#2A | Amphenol ICC | USB 3.1 Type-C receptacle, vertical | 1 | $1.80 | $1.80 |
| J2 | 132255-12 | Amphenol RF | SMA female, edge-launch, 50 Ω | 1 | $4.20 | $4.20 |
| J3 | 132255-12 | Amphenol RF | SMA female, edge-launch (trigger sync) | 1 | $4.20 | $4.20 |
| J4 | 20021121-00010C4LF | Amphenol ICC | 10-pin 1.27mm header (JTAG/debug) | 1 | $0.60 | $0.60 |

### 1.4 Passives (Decoupling, Termination, Filtering)

| Type | Value | Package | Qty | Unit Cost | Ext. Cost |
|------|-------|---------|-----|-----------|-----------|
| MLCC | 100 nF, 6.3V, X7R | 0402 | 45 | $0.02 | $0.90 |
| MLCC | 10 nF, 6.3V, X7R | 0402 | 20 | $0.02 | $0.40 |
| MLCC | 1.0 µF, 6.3V, X5R | 0402 | 12 | $0.04 | $0.48 |
| MLCC | 10 µF, 10V, X5R | 0805 | 8 | $0.12 | $0.96 |
| MLCC | 22 µF, 10V, X5R | 0805 | 4 | $0.18 | $0.72 |
| MLCC | 4.7 µF, 6.3V, X5R | 0603 | 6 | $0.06 | $0.36 |
| MLCC | 0.1 µF, 16V, NP0 | 0402 | 8 | $0.05 | $0.40 |
| Tantalum | 47 µF, 10V | 1210 (TAJ) | 2 | $0.45 | $0.90 |
| Resistor | 10 kΩ, 5% | 0402 | 12 | $0.01 | $0.12 |
| Resistor | 4.7 kΩ, 5% | 0402 | 8 | $0.01 | $0.08 |
| Resistor | 1 kΩ, 5% | 0402 | 6 | $0.01 | $0.06 |
| Resistor | 100 Ω, 5% | 0402 | 4 | $0.01 | $0.04 |
| Resistor | 49.9 Ω, 1% | 0402 | 6 | $0.05 | $0.30 |
| Resistor | 240 Ω, 1% | 0402 | 4 | $0.03 | $0.12 |
| Ferrite bead | BLM18PG121SN1D | 0603 | 6 | $0.08 | $0.48 |
| Inductor | 2.2 µH (power) | 0805 | 3 | $0.15 | $0.45 |
| Inductor | 4.7 µH (power) | 0805 | 2 | $0.18 | $0.36 |
| Inductor | 10 µH (boost) | 1210 | 1 | $0.25 | $0.25 |

### 1.5 LEDs and Miscellaneous

| Designator | Part Number | Manufacturer | Description | Qty | Unit Cost | Ext. Cost |
|------------|-------------|--------------|-------------|-----|-----------|-----------|
| D2,D3,D4 | SK6812-EC15 | Opsco | RGB LED, WS2812-compatible, 1.5×1.5mm | 3 | $0.25 | $0.75 |
| SW1 | KMR241GLFS | C&K | Tactile switch, SPST-NO, SMD | 1 | $0.35 | $0.35 |
| F1 | 1206L050YR | Littelfuse | PTC resettable fuse, 500 mA | 1 | $0.30 | $0.30 |
| Y1 | ABM8G-25.000MHZ-18-D2Y-T | Abracon | 25 MHz crystal, FT601 reference | 1 | $0.55 | $0.55 |

### 1.6 BOM Summary

| Category | Line Items | Total Cost |
|----------|-----------|------------|
| Major ICs | 15 | $110.25 |
| RF / Pulse Gen | 11 | $24.39 |
| Connectors | 4 | $10.80 |
| Passives | 150+ | $7.97 |
| LEDs / Misc | 6 | $2.55 |
| PCB (4-layer, 100×70mm, qty 5) | 1 set | $25.00 |
| Enclosure (CNC aluminum) | 1 | $35.00 |
| **Total BOM (excl. PCB/enclosure)** | | **$155.96** |
| **Total with PCB + enclosure** | | **$215.96** |

## 2. Pinout Tables

### 2.1 FPGA (U1: LFE5U-45F-6BG381C) — Key Pin Assignments

#### Bank 0 (VCCIO=3.3V) — FT601Q Interface
| Ball | Net Name | Direction | FT601 Pin | Function |
|------|----------|-----------|-----------|----------|
| A3 | FT_DATA[0] | I/O | D0 | FIFO data bus bit 0 |
| B3 | FT_DATA[1] | I/O | D1 | FIFO data bus bit 1 |
| C4 | FT_DATA[2] | I/O | D2 | FIFO data bus bit 2 |
| D4 | FT_DATA[3] | I/O | D3 | FIFO data bus bit 3 |
| A4 | FT_DATA[4] | I/O | D4 | FIFO data bus bit 4 |
| B4 | FT_DATA[5] | I/O | D5 | FIFO data bus bit 5 |
| C5 | FT_DATA[6] | I/O | D6 | FIFO data bus bit 6 |
| D5 | FT_DATA[7] | I/O | D7 | FIFO data bus bit 7 |
| A5 | FT_DATA[8] | I/O | D8 | FIFO data bus bit 8 |
| B5 | FT_DATA[9] | I/O | D9 | FIFO data bus bit 9 |
| C6 | FT_DATA[10] | I/O | D10 | FIFO data bus bit 10 |
| D6 | FT_DATA[11] | I/O | D11 | FIFO data bus bit 11 |
| A6 | FT_DATA[12] | I/O | D12 | FIFO data bus bit 12 |
| B6 | FT_DATA[13] | I/O | D13 | FIFO data bus bit 13 |
| C7 | FT_DATA[14] | I/O | D14 | FIFO data bus bit 14 |
| D7 | FT_DATA[15] | I/O | D15 | FIFO data bus bit 15 |
| A7 | FT_DATA[16] | I/O | D16 | FIFO data bus bit 16 |
| B7 | FT_DATA[17] | I/O | D17 | FIFO data bus bit 17 |
| C8 | FT_DATA[18] | I/O | D18 | FIFO data bus bit 18 |
| D8 | FT_DATA[19] | I/O | D19 | FIFO data bus bit 19 |
| A8 | FT_DATA[20] | I/O | D20 | FIFO data bus bit 20 |
| B8 | FT_DATA[21] | I/O | D21 | FIFO data bus bit 21 |
| C9 | FT_DATA[22] | I/O | D22 | FIFO data bus bit 22 |
| D9 | FT_DATA[23] | I/O | D23 | FIFO data bus bit 23 |
| A9 | FT_DATA[24] | I/O | D24 | FIFO data bus bit 24 |
| B9 | FT_DATA[25] | I/O | D25 | FIFO data bus bit 25 |
| C10 | FT_DATA[26] | I/O | D26 | FIFO data bus bit 26 |
| D10 | FT_DATA[27] | I/O | D27 | FIFO data bus bit 27 |
| A10 | FT_DATA[28] | I/O | D28 | FIFO data bus bit 28 |
| B10 | FT_DATA[29] | I/O | D29 | FIFO data bus bit 29 |
| C11 | FT_DATA[30] | I/O | D30 | FIFO data bus bit 30 |
| D11 | FT_DATA[31] | I/O | D31 | FIFO data bus bit 31 |
| A11 | FT_CLK | Input | CLK | 100 MHz FIFO clock |
| B11 | FT_TXE_N | Input | TXE# | Transmit FIFO empty |
| C12 | FT_RXF_N | Input | RXF# | Receive FIFO full |
| D12 | FT_RD_N | Output | RD# | Read strobe |
| A12 | FT_WR_N | Output | WR# | Write strobe |
| B12 | FT_OE_N | Output | OE# | Output enable |
| C13 | FT_SIWU_N | Output | SIWU# | Send immediate / wake up |
| D13 | FT_BE[0] | Output | BE0 | Byte enable 0 |
| A13 | FT_BE[1] | Output | BE1 | Byte enable 1 |
| B13 | FT_BE[2] | Output | BE2 | Byte enable 2 |
| C14 | FT_BE[3] | Output | BE3 | Byte enable 3 |
| D14 | FT_RESET_N | Output | RESET# | FT601 reset |

#### Bank 1 (VCCIO=1.8V) — ADC Interface (LVDS)
| Ball | Net Name | Direction | ADC Pin | Function |
|------|----------|-----------|---------|----------|
| E1 | ADC_D0_P | Input (LVDS) | D0+ | ADC data bit 0 positive |
| F1 | ADC_D0_N | Input (LVDS) | D0- | ADC data bit 0 negative |
| E2 | ADC_D1_P | Input (LVDS) | D1+ | ADC data bit 1 positive |
| F2 | ADC_D1_N | Input (LVDS) | D1- | ADC data bit 1 negative |
| E3 | ADC_D2_P | Input (LVDS) | D2+ | ADC data bit 2 positive |
| F3 | ADC_D2_N | Input (LVDS) | D2- | ADC data bit 2 negative |
| E4 | ADC_D3_P | Input (LVDS) | D3+ | ADC data bit 3 positive |
| F4 | ADC_D3_N | Input (LVDS) | D3- | ADC data bit 3 negative |
| E5 | ADC_D4_P | Input (LVDS) | D4+ | ADC data bit 4 positive |
| F5 | ADC_D4_N | Input (LVDS) | D4- | ADC data bit 4 negative |
| E6 | ADC_D5_P | Input (LVDS) | D5+ | ADC data bit 5 positive |
| F6 | ADC_D5_N | Input (LVDS) | D5- | ADC data bit 5 negative |
| G1 | ADC_D6_P | Input (LVDS) | D6+ | ADC data bit 6 positive |
| H1 | ADC_D6_N | Input (LVDS) | D6- | ADC data bit 6 negative |
| G2 | ADC_D7_P | Input (LVDS) | D7+ | ADC data bit 7 positive |
| H2 | ADC_D7_N | Input (LVDS) | D7- | ADC data bit 7 negative |
| G3 | ADC_D8_P | Input (LVDS) | D8+ | ADC data bit 8 positive |
| H3 | ADC_D8_N | Input (LVDS) | D8- | ADC data bit 8 negative |
| G4 | ADC_D9_P | Input (LVDS) | D9+ | ADC data bit 9 positive |
| H4 | ADC_D9_N | Input (LVDS) | D9- | ADC data bit 9 negative |
| G5 | ADC_D10_P | Input (LVDS) | D10+ | ADC data bit 10 positive |
| H5 | ADC_D10_N | Input (LVDS) | D10- | ADC data bit 10 negative |
| G6 | ADC_D11_P | Input (LVDS) | D11+ | ADC data bit 11 positive |
| H6 | ADC_D11_N | Input (LVDS) | D11- | ADC data bit 11 negative |
| J1 | ADC_OR_P | Input (LVDS) | OR+ | Over-range positive |
| J2 | ADC_OR_N | Input (LVDS) | OR- | Over-range negative |
| J3 | ADC_DCO_P | Input (LVDS) | DCO+ | Data clock out positive |
| J4 | ADC_DCO_N | Input (LVDS) | DCO- | Data clock out negative |
| K1 | ADC_CLK_P | Output (LVDS) | CLK+ | ADC sample clock positive |
| K2 | ADC_CLK_N | Output (LVDS) | CLK- | ADC sample clock negative |
| K3 | ADC_SPI_CS | Output | CSB | ADC SPI chip select |
| K4 | ADC_SPI_CLK | Output | SCLK | ADC SPI clock |
| K5 | ADC_SPI_MOSI | Output | SDIO | ADC SPI data (bidirectional) |

#### Bank 2 (VCCIO=1.35V) — DDR3 Interface
| Ball | Net Name | Direction | DDR3 Pin | Function |
|------|----------|-----------|----------|----------|
| L1 | DDR_A[0] | Output | A0 | Address bit 0 |
| L2 | DDR_A[1] | Output | A1 | Address bit 1 |
| L3 | DDR_A[2] | Output | A2 | Address bit 2 |
| L4 | DDR_A[3] | Output | A3 | Address bit 3 |
| L5 | DDR_A[4] | Output | A4 | Address bit 4 |
| L6 | DDR_A[5] | Output | A5 | Address bit 5 |
| L7 | DDR_A[6] | Output | A6 | Address bit 6 |
| L8 | DDR_A[7] | Output | A7 | Address bit 7 |
| L9 | DDR_A[8] | Output | A8 | Address bit 8 |
| L10 | DDR_A[9] | Output | A9 | Address bit 9 |
| M1 | DDR_A[10] | Output | A10/AP | Address bit 10 / auto-precharge |
| M2 | DDR_A[11] | Output | A11 | Address bit 11 |
| M3 | DDR_A[12] | Output | A12/BC# | Address bit 12 / burst chop |
| M4 | DDR_A[13] | Output | A13 | Address bit 13 |
| M5 | DDR_BA[0] | Output | BA0 | Bank address 0 |
| M6 | DDR_BA[1] | Output | BA1 | Bank address 1 |
| M7 | DDR_BA[2] | Output | BA2 | Bank address 2 |
| M8 | DDR_CKE | Output | CKE | Clock enable |
| M9 | DDR_CLK_P | Output | CK | Clock positive |
| M10 | DDR_CLK_N | Output | CK# | Clock negative |
| N1 | DDR_CS_N | Output | CS# | Chip select |
| N2 | DDR_RAS_N | Output | RAS# | Row address strobe |
| N3 | DDR_CAS_N | Output | CAS# | Column address strobe |
| N4 | DDR_WE_N | Output | WE# | Write enable |
| N5 | DDR_ODT | Output | ODT | On-die termination |
| N6 | DDR_RESET_N | Output | RESET# | DDR3 reset |
| N7 | DDR_DQ[0] | I/O | DQ0 | Data bit 0 |
| N8 | DDR_DQ[1] | I/O | DQ1 | Data bit 1 |
| N9 | DDR_DQ[2] | I/O | DQ2 | Data bit 2 |
| N10 | DDR_DQ[3] | I/O | DQ3 | Data bit 3 |
| P1 | DDR_DQ[4] | I/O | DQ4 | Data bit 4 |
| P2 | DDR_DQ[5] | I/O | DQ5 | Data bit 5 |
| P3 | DDR_DQ[6] | I/O | DQ6 | Data bit 6 |
| P4 | DDR_DQ[7] | I/O | DQ7 | Data bit 7 |
| P5 | DDR_DQS0_P | I/O | UDQS | Upper data strobe positive |
| P6 | DDR_DQS0_N | I/O | UDQS# | Upper data strobe negative |
| P7 | DDR_DM0 | Output | UDM | Upper data mask |
| P8 | DDR_DQ[8] | I/O | DQ8 | Data bit 8 |
| P9 | DDR_DQ[9] | I/O | DQ9 | Data bit 9 |
| P10 | DDR_DQ[10] | I/O | DQ10 | Data bit 10 |
| R1 | DDR_DQ[11] | I/O | DQ11 | Data bit 11 |
| R2 | DDR_DQ[12] | I/O | DQ12 | Data bit 12 |
| R3 | DDR_DQ[13] | I/O | DQ13 | Data bit 13 |
| R4 | DDR_DQ[14] | I/O | DQ14 | Data bit 14 |
| R5 | DDR_DQ[15] | I/O | DQ15 | Data bit 15 |
| R6 | DDR_DQS1_P | I/O | LDQS | Lower data strobe positive |
| R7 | DDR_DQS1_N | I/O | LDQS# | Lower data strobe negative |
| R8 | DDR_DM1 | Output | LDM | Lower data mask |

#### Bank 3 (VCCIO=3.3V) — SPI, I²C, GPIO, Pulse Gen
| Ball | Net Name | Direction | Destination | Function |
|------|----------|-----------|-------------|----------|
| T1 | SPI_FLASH_CS | Output | U13 pin 1 | SPI flash chip select |
| T2 | SPI_FLASH_CLK | Output | U13 pin 6 | SPI flash clock |
| T3 | SPI_FLASH_MOSI | Output | U13 pin 5 | SPI flash MOSI |
| T4 | SPI_FLASH_MISO | Input | U13 pin 2 | SPI flash MISO |
| T5 | VGA_SPI_CS | Output | U4 pin 28 | VGA SPI chip select |
| T6 | VGA_SPI_CLK | Output | U4 pin 29 | VGA SPI clock |
| T7 | VGA_SPI_MOSI | Output | U4 pin 30 | VGA SPI MOSI |
| T8 | VGA_SPI_MISO | Input | U4 pin 31 | VGA SPI MISO |
| U1 | I2C_SCL | I/O (OD) | U12 pin 4 | I²C clock (4.7k pull-up to 3.3V) |
| U2 | I2C_SDA | I/O (OD) | U12 pin 6 | I²C data (4.7k pull-up to 3.3V) |
| U3 | PULSE_TRIG | Output | Q1 base (via 1k) | Avalanche trigger |
| U4 | SRD_BIAS_DAC | Output | R1 (1k to D1) | SRD bias voltage (PWM→filtered) |
| U5 | LED_DATA | Output | D2,D3,D4 DIN | WS2812 LED data |
| U6 | SYNC_OUT | Output | J3 center pin | Trigger sync output |
| U7 | SYNC_IN | Input | J3 center pin (alt) | External trigger input |
| U8 | STATUS_LED_R | Output | LED red channel (alt) | Status indicator |
| U9 | STATUS_LED_G | Output | LED green channel (alt) | Status indicator |
| U10 | STATUS_LED_B | Output | LED blue channel (alt) | Status indicator |
| U11 | ADC_PDWN | Output | U3 pin 45 | ADC power-down control |
| U12 | VGA_PDWN | Output | U4 pin 24 | VGA power-down control |

#### Bank 6 (VCCIO=3.3V) — Configuration & System
| Ball | Net Name | Direction | Destination | Function |
|------|----------|-----------|-------------|----------|
| V1 | CFG_CSN | Output | SPI flash (alt) | FPGA config SPI CS |
| V2 | CFG_CSSPINEN | I/O | — | SPI CS pin enable |
| V3 | CFG_INITN | I/O | 4.7k pull-up | Config init |
| V4 | CFG_PROGRAMN | Input | SW1 (to GND) | Config program (active low) |
| V5 | CFG_DONE | I/O | 4.7k pull-up | Config done |
| V6 | CFG_MCLK | Output | U13 pin 6 (shared) | Master config clock |
| V7 | CFG_MOSI | Output | U13 pin 5 (shared) | Master config MOSI |
| V8 | CFG_MISO | Input | U13 pin 2 (shared) | Master config MISO |
| V9 | JTAG_TCK | Input | J4 pin 4 | JTAG clock |
| V10 | JTAG_TMS | Input | J4 pin 2 | JTAG mode select |
| V11 | JTAG_TDI | Input | J4 pin 8 | JTAG data in |
| V12 | JTAG_TDO | Output | J4 pin 6 | JTAG data out |
| V13 | CLK_REF_P | Input (LVDS) | U11 pin 4 | 250 MHz ref clock positive |
| V14 | CLK_REF_N | Input (LVDS) | U11 pin 5 | 250 MHz ref clock negative |
| V15 | RESET_N | Input | MAX16054 RST output | System reset |

### 2.2 ADC (U3: AD9230BCPZ-250) — Pin Assignments

| Pin | Name | Net | Direction | Description |
|-----|------|-----|-----------|-------------|
| 1,2,5,6,9,10,13,14,17,18,21,22 | D0± through D11± | ADC_D[0:11]_P/N | Output (LVDS) | 12-bit data, DDR |
| 3,4 | OR± | ADC_OR_P/N | Output (LVDS) | Over-range indicator |
| 7,8 | DCO± | ADC_DCO_P/N | Output (LVDS) | Data clock output |
| 11,12 | CLK± | ADC_CLK_P/N | Input (LVDS) | 250 MHz sample clock |
| 23 | AVDD | +1.8V_ANA | Power | Analog supply |
| 24 | AGND | GND_ANA | Power | Analog ground |
| 25 | VREF | VREF_ADC | Bypass | 0.1 µF to AGND |
| 26 | SENSE | GND_ANA | Tie | Reference mode select (internal) |
| 27 | VREFB | Bypass | Bypass | 0.1 µF to AGND |
| 28 | REFT | Bypass | Bypass | 0.1 µF to AGND |
| 29 | REFB | Bypass | Bypass | 0.1 µF to AGND |
| 30 | CML | Bypass | Bypass | 0.1 µF to AGND |
| 31 | RBIAS | RBIAS_ADC | Resistor | 10 kΩ 1% to AGND (sets internal bias) |
| 32,33,34 | VIN+, VIN-, VCM | ADC_IN_P, ADC_IN_N, ADC_VCM | Input | Analog input (from VGA) |
| 35,36,37,38,39,40,41,42 | NC | — | — | No connect |
| 43 | DRVDD | +1.8V_DIG | Power | Digital output supply |
| 44 | DRGND | GND_DIG | Power | Digital ground |
| 45 | PDWN | ADC_PDWN | Input | Power-down (active high) |
| 46 | CSB | ADC_SPI_CS | Input | SPI chip select |
| 47 | SCLK | ADC_SPI_CLK | Input | SPI clock |
| 48 | SDIO | ADC_SPI_MOSI | I/O | SPI data I/O |
| 49 | OEB | GND_DIG | Tie low | Output enable (always enabled) |
| 50,51,52,53,54,55,56 | DNC | — | — | Do not connect |
| PAD | Exposed pad | GND_ANA | Power | Thermal pad, must solder to GND plane |

### 2.3 VGA (U4: LMH6517SQ/NOPB) — Pin Assignments

| Pin | Name | Net | Direction | Description |
|-----|------|-----|-----------|-------------|
| 1,2 | IN+, IN- | VGA_IN_P, VGA_IN_N | Input | Differential input from coupler |
| 3,4,5,6,7,8,9,10,11,12 | NC | — | — | No connect |
| 13,14,15,16 | GND | GND_ANA | Power | Ground (multiple pins) |
| 17,18 | OUT-, OUT+ | VGA_OUT_N, VGA_OUT_P | Output | Differential output to ADC |
| 19,20,21,22 | V+, V+, V+, V+ | +5V_ANA | Power | Positive supply |
| 23,24 | V-, V- | -5V_ANA | Power | Negative supply |
| 25 | AUX_OUT | NC | Output | Auxiliary output (unused) |
| 26 | AUX_IN | GND_ANA | Input | Aux input tied to GND |
| 27 | VCM | VGA_VCM | Bypass | 0.1 µF to GND |
| 28 | CS | VGA_SPI_CS | Input | SPI chip select |
| 29 | SCLK | VGA_SPI_CLK | Input | SPI clock |
| 30 | SDI | VGA_SPI_MOSI | Input | SPI data in |
| 31 | SDO | VGA_SPI_MISO | Output | SPI data out |
| 32 | PD | VGA_PDWN | Input | Power-down (active high) |
| PAD | Exposed pad | GND_ANA | Power | Thermal pad |

### 2.4 FT601Q (U2: FT601Q-B-T) — Key Pins

| Pin | Name | Net | Direction | Description |
|-----|------|-----|-----------|-------------|
| 1–8, 11–18, 21–28, 31–38 | DATA[31:0] | FT_DATA[31:0] | I/O | 32-bit FIFO data |
| 9 | CLK | FT_CLK | Input | 100 MHz clock from FPGA |
| 10 | RESET# | FT_RESET_N | Input | Active-low reset |
| 19 | TXE# | FT_TXE_N | Output | Transmit FIFO empty flag |
| 20 | RXF# | FT_RXF_N | Output | Receive FIFO full flag |
| 29 | RD# | FT_RD_N | Input | Read strobe |
| 30 | WR# | FT_WR_N | Input | Write strobe |
| 39 | OE# | FT_OE_N | Input | Output enable |
| 40 | SIWU# | FT_SIWU_N | Input | Send immediate / wake up |
| 41–44 | BE[3:0] | FT_BE[3:0] | Input | Byte enables |
| 45 | WAKEUP | NC | Output | Wakeup indicator (unused) |
| 46 | SUSPEND | NC | Output | Suspend indicator (unused) |
| 47 | XTALIN | 25 MHz | Input | Crystal input |
| 48 | XTALOUT | 25 MHz | Output | Crystal output |
| 49 | REFCLK | GND | Input | Reference clock (unused, use crystal) |
| 50 | TEST | GND | Input | Test pin, tie low |
| 51 | AVDD12 | +1.2V | Power | Analog 1.2V supply |
| 52 | AGND | GND | Power | Analog ground |
| 53,54 | SSRX± | USB_SSRX_P/N | Input | USB 3.0 SuperSpeed receive |
| 55,56 | SSTX± | USB_SSTX_P/N | Output | USB 3.0 SuperSpeed transmit |
| 57,58 | DP, DM | USB_DP, USB_DM | I/O | USB 2.0 data |
| 59 | VCCIO | +3.3V | Power | I/O supply |
| 60 | VCCD | +1.2V | Power | Digital core supply |
| 61 | VCORE | +1.2V | Power | Core supply |
| 62 | VPHY | +3.3V | Power | PHY supply |
| 63 | VPLL | +1.2V | Power | PLL supply |
| 64–76 | GND, NC | GND, NC | — | Ground and no-connects |
| PAD | Exposed pad | GND | Power | Thermal pad |

## 3. Netlists — Critical Signal Paths

### 3.1 Pulse Generation Chain
```
FPGA(U1.U3) PULSE_TRIG ──► R4(1kΩ 0402) ──► Q1(BFR92A) Base
                                              │
Q1 Collector ──┬── R2(499Ω 0603) ──► +25V_SRD (avalanche bias)
               │
               └── C1(1.0pF NP0) ──┬── L1(2.2nH) ──► GND
                                   │
                                   └── D1(MA44769) Cathode
                                       │
D1 Anode ──┬── L2(8.2nH) ──► GND (pulse shaping)
           │
           └── C2(2.2pF NP0) ──┬── R3(49.9Ω 0603) ──► C3(10pF DC block) ──► T1 RF_IN
                                │
                                └── GND (via 49.9Ω termination)
```

### 3.2 Directional Coupler to ADC Path
```
T1(TCD-10-1X+) RF_OUT ──► VGA_IN_P (U4 pin 1)
T1 COUPLED ──► VGA_IN_N (U4 pin 2) via 49.9Ω termination to GND
T1 RF_IN ──► J2 (SMA output to DUT)

VGA_OUT_P (U4 pin 18) ──► ADC_VIN_P (U3 pin 32) — 50Ω controlled impedance
VGA_OUT_N (U4 pin 17) ──► ADC_VIN_N (U3 pin 33) — 50Ω controlled impedance
```

### 3.3 Clock Distribution
```
U11(SiT9365) OUT_P ──► FPGA CLK_REF_P (U1.V13) — 100Ω differential, 250 MHz LVDS
U11 OUT_N ──► FPGA CLK_REF_N (U1.V14)

FPGA PLL0 ──► ADC_CLK_P (U1.K1) ──► U3 pin 11 — 250 MHz LVDS
FPGA PLL0 ──► ADC_CLK_N (U1.K2) ──► U3 pin 12

FPGA PLL1 ──► FT_CLK (U1.A11) ──► U2 pin 9 — 100 MHz LVCMOS
```

### 3.4 USB 3.0 SuperSpeed Differential Pairs
```
J1(USB-C) A2(TX1+) ──► U2 pin 55 (SSTX+) — 90Ω differential, 5 Gbps
J1 A3(TX1-) ──► U2 pin 56 (SSTX-)
J1 B11(RX1+) ──► U2 pin 53 (SSRX+) — 90Ω differential, 5 Gbps
J1 B10(RX1-) ──► U2 pin 54 (SSRX-)

J1 A6(DP) ──► U2 pin 57 (USB_DP) — 90Ω differential, 480 Mbps
J1 A7(DN) ──► U2 pin 58 (USB_DM)

ESD Protection: TPD4E05U06 (U14) placed within 5 mm of J1
  - U14 pin 1 (CH1) → J1 A2 (TX1+)
  - U14 pin 2 (CH2) → J1 A3 (TX1-)
  - U14 pin 4 (CH3) → J1 B11 (RX1+)
  - U14 pin 5 (CH4) → J1 B10 (RX1-)
```

## 4. Decoupling Networks

### 4.1 FPGA Decoupling (U1: LFE5U-45F)
Per ECP5 guidelines, each power pin pair gets local decoupling:

**VCCINT (1.2V core) — 12 pin pairs:**
- Each pair: 100 nF 0402 X7R (placed within 2 mm of ball)
- Bulk: 2× 10 µF 0805 X5R, 1× 47 µF 1210 tantalum

**VCCIO0–VCCIO7 (3.3V I/O banks):**
- Each bank (4 pin pairs): 4× 100 nF 0402 X7R
- Per bank bulk: 1× 10 µF 0805 X5R

**VCCAUX (3.3V auxiliary):**
- 4× 100 nF 0402 X7R
- 1× 10 µF 0805 X5R

**VCCPLL (1.2V PLL):**
- 2× 100 nF 0402 X7R
- 1× 1.0 µF 0402 X5R
- Ferrite bead (BLM18PG121SN1D) in series from VCCINT rail

### 4.2 ADC Decoupling (U3: AD9230BCPZ-250)
**AVDD (1.8V analog):**
- 1× 100 nF 0402 NP0 (pin 23, within 1 mm)
- 1× 10 nF 0402 NP0
- 1× 1.0 µF 0402 X5R
- Ferrite bead from +1.8V_ANA rail

**DRVDD (1.8V digital):**
- 1× 100 nF 0402 X7R (pin 43)
- 1× 1.0 µF 0402 X5R

**Reference bypasses:**
- VREF (pin 25): 0.1 µF 0402 NP0 + 10 µF 0805 X5R
- VREFB (pin 27): 0.1 µF 0402 NP0
- REFT (pin 28): 0.1 µF 0402 NP0
- REFB (pin 29): 0.1 µF 0402 NP0
- CML (pin 30): 0.1 µF 0402 NP0

### 4.3 VGA Decoupling (U4: LMH6517)
**V+ (+5V analog) — pins 19,20,21,22:**
- Each pin: 100 nF 0402 NP0 (within 1 mm)
- Shared: 1× 1.0 µF 0402 X5R, 1× 10 µF 0805 X5R
- Ferrite bead from +5V_ANA rail

**V- (-5V analog) — pins 23,24:**
- Each pin: 100 nF 0402 NP0
- Shared: 1× 1.0 µF 0402 X5R
- Ferrite bead from -5V_ANA rail

### 4.4 DDR3 Decoupling (U5: MT41K128M16JT-125)
**VDD (1.35V):**
- 4× 100 nF 0402 X7R (one per corner of BGA)
- 2× 1.0 µF 0402 X5R
- 1× 10 µF 0805 X5R

**VREFDQ, VREFCA:**
- Each: 100 nF 0402 NP0 (voltage reference, must be clean)

## 5. Pull-Up / Pull-Down Resistor Table

| Net | Resistor | Value | To Rail | Purpose |
|-----|----------|-------|---------|---------|
| CFG_INITN | R5 | 4.7 kΩ | +3.3V | FPGA config init pull-up |
| CFG_DONE | R6 | 4.7 kΩ | +3.3V | FPGA config done pull-up |
| CFG_PROGRAMN | R7 | 4.7 kΩ | +3.3V | FPGA program pin pull-up (SW1 pulls to GND) |
| I2C_SCL | R8 | 4.7 kΩ | +3.3V | I²C clock pull-up |
| I2C_SDA | R9 | 4.7 kΩ | +3.3V | I²C data pull-up |
| FT_RESET_N | R10 | 10 kΩ | +3.3V | FT601 reset pull-up (FPGA drives low to reset) |
| ADC_PDWN | R11 | 10 kΩ | GND | ADC power-down pull-down (active high, default off) |
| VGA_PDWN | R12 | 10 kΩ | GND | VGA power-down pull-down (active high, default off) |
| ADC_OEB | R13 | 1 kΩ | GND | ADC output enable, tie low (always enabled) |
| ADC_CSB | R14 | 10 kΩ | +1.8V | ADC SPI CS pull-up (active low) |
| VGA_CS | R15 | 10 kΩ | +3.3V | VGA SPI CS pull-up (active low) |
| USB_CC1 | R16 | 5.1 kΩ | GND | USB-C CC1 pulldown (UFP mode, default 500 mA) |
| USB_CC2 | R17 | 5.1 kΩ | GND | USB-C CC2 pulldown |
| MAX16054_EN | R18 | 100 kΩ | +3.3V | Enable pin pull-up |
| JTAG_TMS | R19 | 4.7 kΩ | +3.3V | JTAG TMS pull-up |
| JTAG_TDI | R20 | 4.7 kΩ | +3.3V | JTAG TDI pull-up |
| JTAG_TCK | R21 | 4.7 kΩ | GND | JTAG TCK pull-down (prevent floating clock) |

## 6. Impedance-Matched Pairs

| Signal Group | Impedance | Topology | Length Matching |
|-------------|-----------|----------|-----------------|
| ADC LVDS data (12 pairs) | 100 Ω differential | Point-to-point | ±1 mm within group |
| ADC LVDS clock (1 pair) | 100 Ω differential | Point-to-point | Match to data group ±2 mm |
| USB 3.0 SS pairs (2 pairs) | 90 Ω differential | Point-to-point | ±0.5 mm within pair |
| USB 2.0 DP/DM | 90 Ω differential | Point-to-point | ±0.5 mm |
| DDR3 DQ/DQS (2 byte lanes) | 50 Ω single-ended | Fly-by (addr/ctrl), point-to-point (DQ) | DQ ±1 mm to DQS, byte lane ±5 mm |
| DDR3 CLK | 100 Ω differential | Fly-by | Match to addr/ctrl ±2 mm |
| DDR3 ADDR/CTRL | 50 Ω single-ended | Fly-by | ±5 mm within group |
| FPGA ref clock | 100 Ω differential | Point-to-point | ±0.5 mm |
| VGA output to ADC | 50 Ω single-ended | Point-to-point | ±1 mm |
| SMA output trace | 50 Ω single-ended | Point-to-point | N/A (single trace) |

## 7. Power Tree Detail

```
USB VBUS (5V, 900 mA max)
│
├── F1 (1206L050, 500 mA PTC fuse)
│   └── VBUS_FUSED
│       │
│       ├── U6 (TPS63070) Buck-Boost ──► +3.3V (2A max)
│       │   ├── FPGA VCCIO banks 0,2,3,6,7
│       │   ├── FPGA VCCAUX
│       │   ├── FT601 VCCIO, VPHY
│       │   ├── SPI Flash (U13)
│       │   ├── TMP117 (U12)
│       │   ├── SiT9365 (U11)
│       │   └── LEDs (D2,D3,D4)
│       │
│       ├── U8 (LP5907) LDO ──► +1.2V (250 mA)
│       │   ├── FPGA VCCINT
│       │   ├── FPGA VCCPLL (via ferrite bead)
│       │   ├── FT601 VCCD, VCORE, VPLL, AVDD12
│       │   └── U10 (TPS51200) VIN
│       │
│       ├── U9 (ADP7118) LDO ──► +1.8V_DIG (200 mA)
│       │   ├── ADC DRVDD
│       │   └── FPGA Bank 1 VCCIO
│       │
│       ├── U9b (ADP7118) LDO ──► +1.8V_ANA (200 mA)
│       │   └── ADC AVDD (via ferrite bead)
│       │
│       ├── U7 (TPS65131) Boost ──► +5V_ANA (300 mA)
│       │   └── VGA V+ (via ferrite bead)
│       │
│       ├── U7 (TPS65131) Inverter ──► -5V_ANA (150 mA)
│       │   └── VGA V- (via ferrite bead)
│       │
│       ├── U10 (TPS51200) DDR Term ──► +1.35V (VDD), +0.675V (VTT)
│       │   ├── DDR3 VDD
│       │   ├── DDR3 VREFDQ, VREFCA (via voltage divider)
│       │   └── FPGA Bank 4,5 VCCIO
│       │
│       └── SRD Boost (discrete) ──► +25V_SRD (5 mA)
│           └── D1 bias (via R1 1kΩ)
│
└── U15 (MAX16054) On/Off Controller
    ├── SW1 (KMR241GLFS) ──► ON/OFF input
    └── EN output ──► U6, U7 enable pins
```

## 8. Power Sequencing Requirements

1. **+3.3V** (U6) ramps first — enables FPGA I/O banks and configuration
2. **+1.2V** (U8) ramps second — FPGA core voltage
3. **+1.8V** (U9) ramps with +1.2V — ADC and FPGA bank 1
4. **+1.35V** (U10) ramps after +3.3V stable — DDR3 supply
5. **+5V / -5V** (U7) ramps last — analog frontend

Sequencing enforced by:
- U6 PG (power good) output → U8 EN input (via 1 ms RC delay)
- U8 PG → U9 EN
- U6 PG → U10 EN (via 2 ms RC delay)
- U6 PG → U7 EN (via 5 ms RC delay)

---

*End of Phase 2 — Component Selection & Schematics*
