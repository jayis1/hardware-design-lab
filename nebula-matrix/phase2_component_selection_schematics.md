# Phase 2: Component Selection & Schematics — Nebula Matrix
## Professional LED Matrix Display Engine

---

## 1. Bill of Materials (BOM) — Major Components

| RefDes | Manufacturer | Part Number | Description | Qty | Unit Cost (1k) | Ext Cost |
|--------|-------------|-------------|-------------|-----|-----------------|----------|
| U1 | Xilinx/AMD | XC7A100T-2FGG484C | Artix-7 FPGA, 101K LUTs, 484-FBGA | 1 | $89.00 | $89.00 |
| U2 | STMicro | STM32H743VIT6 | Cortex-M7, 480 MHz, 2MB Flash, 1MB SRAM, LQFP-100 | 1 | $14.50 | $14.50 |
| U3 | TI | TPS65218D0RSLR | PMIC, 6 regulators, VQFN-48 | 1 | $4.20 | $4.20 |
| U4 | Skyworks | Si5351A-B-GT | I2C Clock Gen, 8 outputs, MSOP-10 | 1 | $2.80 | $2.80 |
| U5 | Microchip | KSZ9031RNXIC | GbE PHY, RGMII, QFN-48 | 1 | $4.50 | $4.50 |
| U6 | Microchip | USB3320C-EZK | USB 2.0 ULPI PHY, QFN-32 | 1 | $2.10 | $2.10 |
| U7 | Analog Devices | ADV7611BSWZ-P | HDMI 1.4 Rx, 165 MHz, LQFP-64 | 1 | $8.90 | $8.90 |
| U8, U9 | Alliance Memory | AS4C256M16D3LC-12BCN | DDR3L, 4Gb (256M×16), 1.35V, 96-BGA | 2 | $5.80 | $11.60 |
| U10-U25 | TI | SN74LVCH16T245DGGR | 16-bit dual-supply bus transceiver, TSSOP-48 | 16 | $1.20 | $19.20 |
| U26 | Winbond | W25Q128JVSIQ | 128Mb SPI NOR Flash, SOIC-8 | 1 | $0.80 | $0.80 |
| U27 | TI | TMP117AIDRVR | ±0.1°C digital temp sensor, WSON-6 | 4 | $1.50 | $6.00 |
| U28 | TI | INA219BIDGSR | I2C current/power monitor, VSSOP-10 | 1 | $1.10 | $1.10 |
| J1 | Amphenol | RJHSE-5381 | RJ45 with magnetics, 10/100/1000 | 1 | $3.50 | $3.50 |
| J2 | Molex | 1054500101 | USB-C receptacle, 24-pin, SMT | 1 | $1.80 | $1.80 |
| J3 | Molex | 5025700893 | microSD push-push socket | 1 | $1.20 | $1.20 |
| J4 | CUI Devices | PJ-002A | DC barrel jack, 5.5×2.1mm | 1 | $0.60 | $0.60 |
| J5-J20 | Samtec | TSW-107-08-G-D-RA | 2×7 pin header, right-angle, HUB75E | 16 | $1.40 | $22.40 |
| J21 | Amphenol | 10029449-001RLF | HDMI Type-A receptacle, SMT | 1 | $1.30 | $1.30 |
| J22 | Tag-Connect | TC2050-IDC | 10-pin ARM SWD cable-less header | 1 | $0.50 | $0.50 |
| J23 | Digilent | 410-251 | 14-pin JTAG header (FPGA) | 1 | $0.80 | $0.80 |
| Y1 | Abracon | ABM8G-25.000MHZ-18-D2Y-T | 25 MHz crystal, 18pF, 4-SMD | 1 | $0.45 | $0.45 |
| Y2 | Epson | FA-20H 24.000000MHz | 24 MHz crystal, 12pF, 4-SMD | 1 | $0.50 | $0.50 |
| FAN1,2 | Sunon | MF40101V1-1000U-A99 | 40×40×10mm, 12V, Vapo bearing | 2 | $4.50 | $9.00 |
| LED1-4 | Kingbright | APT2012SECK/J4-PRV | 0805 SMD LED, various colors | 4 | $0.10 | $0.40 |
| SW1,2 | C&K | PTS645SM43SMTR92 | SMD tactile switch, 6mm | 2 | $0.15 | $0.30 |

**Total BOM Cost (1k qty): ~$195.00**

---

## 2. Passive Components — Decoupling & Termination

### 2.1 FPGA Decoupling Network (XC7A100T)

Per Xilinx UG483 (7 Series PCB Design Guide), each supply rail requires:

| Rail | Voltage | Bulk Caps | Per-Bank Caps | Total Caps |
|------|---------|-----------|---------------|------------|
| VCCINT | 1.0V | 4× 100µF tantalum + 2× 10µF MLCC | 1× 0.47µF + 1× 0.01µF per ball pair | ~60 MLCC |
| VCCBRAM | 1.2V | 2× 47µF tantalum | 1× 0.47µF per 2 balls | ~20 MLCC |
| VCCAUX | 1.8V | 2× 47µF tantalum | 1× 0.47µF per 2 balls | ~15 MLCC |
| VCCO_35 | 3.3V | 2× 47µF tantalum | 1× 0.1µF per 2 balls | ~25 MLCC |
| VCCO_DDR | 1.35V | 2× 47µF tantalum | 1× 0.1µF per 2 balls | ~20 MLCC |

**MLCC Spec**: 0402, X7R, 10V rating (Murata GRM155R71A104KA01D for 0.1µF)
**Tantalum Spec**: 1210, 10V (AVX TAJB107M010RNJ for 100µF)

### 2.2 DDR3L Termination

SSTL-135 termination scheme per JEDEC JESD79-3E:

| Signal Group | Termination | Value | Location |
|--------------|-------------|-------|----------|
| Address/Command | Fly-by, 39.2Ω to VTT (0.675V) | 39.2Ω ±1% | End of fly-by chain |
| Clock (CK/CK#) | Differential, 100Ω diff | — | FPGA output |
| DQ/DQS/DM | ODT on DRAM die | 40Ω (RZQ=240Ω to GND) | DRAM internal |
| VTT Regulator | TP51200DGQR | VTT = VDDQ/2, 3A sink/source | Near DRAM |

**RZQ Resistor**: 240Ω ±1% (Panasonic ERA-3AEB241V) from DRAM ZQ pin to GND.

### 2.3 HUB75E Output Termination

Each of the 224 output signals:

| Signal | Source Impedance | Series Term | Receiver |
|--------|-----------------|-------------|----------|
| R1,G1,B1,R2,G2,B2 | 25Ω (FPGA LVCMOS33, 8mA drive) | 22Ω ±5% | SN74LVCH16T245 input |
| A,B,C,D,E | 25Ω (FPGA LVCMOS33, 8mA drive) | 22Ω ±5% | SN74LVCH16T245 input |
| CLK | 25Ω (FPGA LVCMOS33, 12mA drive) | 10Ω ±5% | SN74LVCH16T245 input |
| LAT | 25Ω (FPGA LVCMOS33, 12mA drive) | 10Ω ±5% | SN74LVCH16T245 input |
| OE | 25Ω (FPGA LVCMOS33, 12mA drive) | 10Ω ±5% | SN74LVCH16T245 input |

Series resistors: 0402, ±5% (Yageo RC0402FR-0722RL for 22Ω, RC0402FR-0710RL for 10Ω)

### 2.4 Level Shifter Output Protection

Each SN74LVCH16T245 output to HUB75E connector:
- 10kΩ pull-down to GND on each output (prevents floating during startup)
- ESD protection: Nexperia PESD5V0S1BA (SOD-323, 5V bidirectional TVS) on each output pin

---

## 3. Complete Pinout Tables

### 3.1 FPGA (XC7A100T-2FGG484C) Pin Assignments

#### Bank 13 (HR, VCCO=3.3V) — HUB75E Ports 0-3

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| F4 | HUB0_R1 | LVCMOS33 | U10 (SN74LVCH16T245) pin 1A1 |
| G3 | HUB0_G1 | LVCMOS33 | U10 pin 1A2 |
| H4 | HUB0_B1 | LVCMOS33 | U10 pin 1A3 |
| J3 | HUB0_R2 | LVCMOS33 | U10 pin 1A4 |
| K4 | HUB0_G2 | LVCMOS33 | U10 pin 1A5 |
| L3 | HUB0_B2 | LVCMOS33 | U10 pin 1A6 |
| M4 | HUB0_A | LVCMOS33 | U10 pin 1A7 |
| N3 | HUB0_B | LVCMOS33 | U10 pin 1A8 |
| P4 | HUB0_C | LVCMOS33 | U10 pin 2A1 |
| R3 | HUB0_D | LVCMOS33 | U10 pin 2A2 |
| T4 | HUB0_E | LVCMOS33 | U10 pin 2A3 |
| U3 | HUB0_CLK | LVCMOS33 | U10 pin 2A4 |
| V4 | HUB0_LAT | LVCMOS33 | U10 pin 2A5 |
| W3 | HUB0_OE | LVCMOS33 | U10 pin 2A6 |
| F5 | HUB1_R1 | LVCMOS33 | U11 pin 1A1 |
| G4 | HUB1_G1 | LVCMOS33 | U11 pin 1A2 |
| H5 | HUB1_B1 | LVCMOS33 | U11 pin 1A3 |
| J4 | HUB1_R2 | LVCMOS33 | U11 pin 1A4 |
| K5 | HUB1_G2 | LVCMOS33 | U11 pin 1A5 |
| L4 | HUB1_B2 | LVCMOS33 | U11 pin 1A6 |
| M5 | HUB1_A | LVCMOS33 | U11 pin 1A7 |
| N4 | HUB1_B | LVCMOS33 | U11 pin 1A8 |
| P5 | HUB1_C | LVCMOS33 | U11 pin 2A1 |
| R4 | HUB1_D | LVCMOS33 | U11 pin 2A2 |
| T5 | HUB1_E | LVCMOS33 | U11 pin 2A3 |
| U4 | HUB1_CLK | LVCMOS33 | U11 pin 2A4 |
| V5 | HUB1_LAT | LVCMOS33 | U11 pin 2A5 |
| W4 | HUB1_OE | LVCMOS33 | U11 pin 2A6 |
| F6 | HUB2_R1 | LVCMOS33 | U12 pin 1A1 |
| G5 | HUB2_G1 | LVCMOS33 | U12 pin 1A2 |
| H6 | HUB2_B1 | LVCMOS33 | U12 pin 1A3 |
| J5 | HUB2_R2 | LVCMOS33 | U12 pin 1A4 |
| K6 | HUB2_G2 | LVCMOS33 | U12 pin 1A5 |
| L5 | HUB2_B2 | LVCMOS33 | U12 pin 1A6 |
| M6 | HUB2_A | LVCMOS33 | U12 pin 1A7 |
| N5 | HUB2_B | LVCMOS33 | U12 pin 1A8 |
| P6 | HUB2_C | LVCMOS33 | U12 pin 2A1 |
| R5 | HUB2_D | LVCMOS33 | U12 pin 2A2 |
| T6 | HUB2_E | LVCMOS33 | U12 pin 2A3 |
| U5 | HUB2_CLK | LVCMOS33 | U12 pin 2A4 |
| V6 | HUB2_LAT | LVCMOS33 | U12 pin 2A5 |
| W5 | HUB2_OE | LVCMOS33 | U12 pin 2A6 |
| F7 | HUB3_R1 | LVCMOS33 | U13 pin 1A1 |
| G6 | HUB3_G1 | LVCMOS33 | U13 pin 1A2 |
| H7 | HUB3_B1 | LVCMOS33 | U13 pin 1A3 |
| J6 | HUB3_R2 | LVCMOS33 | U13 pin 1A4 |
| K7 | HUB3_G2 | LVCMOS33 | U13 pin 1A5 |
| L6 | HUB3_B2 | LVCMOS33 | U13 pin 1A6 |
| M7 | HUB3_A | LVCMOS33 | U13 pin 1A7 |
| N6 | HUB3_B | LVCMOS33 | U13 pin 1A8 |
| P7 | HUB3_C | LVCMOS33 | U13 pin 2A1 |
| R6 | HUB3_D | LVCMOS33 | U13 pin 2A2 |
| T7 | HUB3_E | LVCMOS33 | U13 pin 2A3 |
| U6 | HUB3_CLK | LVCMOS33 | U13 pin 2A4 |
| V7 | HUB3_LAT | LVCMOS33 | U13 pin 2A5 |
| W6 | HUB3_OE | LVCMOS33 | U13 pin 2A6 |

#### Bank 14 (HR, VCCO=3.3V) — HUB75E Ports 4-7

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| AA3 | HUB4_R1 | LVCMOS33 | U14 pin 1A1 |
| AB4 | HUB4_G1 | LVCMOS33 | U14 pin 1A2 |
| AC3 | HUB4_B1 | LVCMOS33 | U14 pin 1A3 |
| AD4 | HUB4_R2 | LVCMOS33 | U14 pin 1A4 |
| AE3 | HUB4_G2 | LVCMOS33 | U14 pin 1A5 |
| AF4 | HUB4_B2 | LVCMOS33 | U14 pin 1A6 |
| AG3 | HUB4_A | LVCMOS33 | U14 pin 1A7 |
| AH4 | HUB4_B | LVCMOS33 | U14 pin 1A8 |
| AJ3 | HUB4_C | LVCMOS33 | U14 pin 2A1 |
| AK4 | HUB4_D | LVCMOS33 | U14 pin 2A2 |
| AL3 | HUB4_E | LVCMOS33 | U14 pin 2A3 |
| AM4 | HUB4_CLK | LVCMOS33 | U14 pin 2A4 |
| AN3 | HUB4_LAT | LVCMOS33 | U14 pin 2A5 |
| AP4 | HUB4_OE | LVCMOS33 | U14 pin 2A6 |
| AA4 | HUB5_R1 | LVCMOS33 | U15 pin 1A1 |
| AB5 | HUB5_G1 | LVCMOS33 | U15 pin 1A2 |
| AC4 | HUB5_B1 | LVCMOS33 | U15 pin 1A3 |
| AD5 | HUB5_R2 | LVCMOS33 | U15 pin 1A4 |
| AE4 | HUB5_G2 | LVCMOS33 | U15 pin 1A5 |
| AF5 | HUB5_B2 | LVCMOS33 | U15 pin 1A6 |
| AG4 | HUB5_A | LVCMOS33 | U15 pin 1A7 |
| AH5 | HUB5_B | LVCMOS33 | U15 pin 1A8 |
| AJ4 | HUB5_C | LVCMOS33 | U15 pin 2A1 |
| AK5 | HUB5_D | LVCMOS33 | U15 pin 2A2 |
| AL4 | HUB5_E | LVCMOS33 | U15 pin 2A3 |
| AM5 | HUB5_CLK | LVCMOS33 | U15 pin 2A4 |
| AN4 | HUB5_LAT | LVCMOS33 | U15 pin 2A5 |
| AP5 | HUB5_OE | LVCMOS33 | U15 pin 2A6 |
| AA5 | HUB6_R1 | LVCMOS33 | U16 pin 1A1 |
| AB6 | HUB6_G1 | LVCMOS33 | U16 pin 1A2 |
| AC5 | HUB6_B1 | LVCMOS33 | U16 pin 1A3 |
| AD6 | HUB6_R2 | LVCMOS33 | U16 pin 1A4 |
| AE5 | HUB6_G2 | LVCMOS33 | U16 pin 1A5 |
| AF6 | HUB6_B2 | LVCMOS33 | U16 pin 1A6 |
| AG5 | HUB6_A | LVCMOS33 | U16 pin 1A7 |
| AH6 | HUB6_B | LVCMOS33 | U16 pin 1A8 |
| AJ5 | HUB6_C | LVCMOS33 | U16 pin 2A1 |
| AK6 | HUB6_D | LVCMOS33 | U16 pin 2A2 |
| AL5 | HUB6_E | LVCMOS33 | U16 pin 2A3 |
| AM6 | HUB6_CLK | LVCMOS33 | U16 pin 2A4 |
| AN5 | HUB6_LAT | LVCMOS33 | U16 pin 2A5 |
| AP6 | HUB6_OE | LVCMOS33 | U16 pin 2A6 |
| AA6 | HUB7_R1 | LVCMOS33 | U17 pin 1A1 |
| AB7 | HUB7_G1 | LVCMOS33 | U17 pin 1A2 |
| AC6 | HUB7_B1 | LVCMOS33 | U17 pin 1A3 |
| AD7 | HUB7_R2 | LVCMOS33 | U17 pin 1A4 |
| AE6 | HUB7_G2 | LVCMOS33 | U17 pin 1A5 |
| AF7 | HUB7_B2 | LVCMOS33 | U17 pin 1A6 |
| AG6 | HUB7_A | LVCMOS33 | U17 pin 1A7 |
| AH7 | HUB7_B | LVCMOS33 | U17 pin 1A8 |
| AJ6 | HUB7_C | LVCMOS33 | U17 pin 2A1 |
| AK7 | HUB7_D | LVCMOS33 | U17 pin 2A2 |
| AL6 | HUB7_E | LVCMOS33 | U17 pin 2A3 |
| AM7 | HUB7_CLK | LVCMOS33 | U17 pin 2A4 |
| AN6 | HUB7_LAT | LVCMOS33 | U17 pin 2A5 |
| AP7 | HUB7_OE | LVCMOS33 | U17 pin 2A6 |

#### Bank 15 (HR, VCCO=3.3V) — HUB75E Ports 8-11

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| B1 | HUB8_R1 | LVCMOS33 | U18 pin 1A1 |
| C2 | HUB8_G1 | LVCMOS33 | U18 pin 1A2 |
| D1 | HUB8_B1 | LVCMOS33 | U18 pin 1A3 |
| E2 | HUB8_R2 | LVCMOS33 | U18 pin 1A4 |
| F1 | HUB8_G2 | LVCMOS33 | U18 pin 1A5 |
| G2 | HUB8_B2 | LVCMOS33 | U18 pin 1A6 |
| H1 | HUB8_A | LVCMOS33 | U18 pin 1A7 |
| J2 | HUB8_B | LVCMOS33 | U18 pin 1A8 |
| K1 | HUB8_C | LVCMOS33 | U18 pin 2A1 |
| L2 | HUB8_D | LVCMOS33 | U18 pin 2A2 |
| M1 | HUB8_E | LVCMOS33 | U18 pin 2A3 |
| N2 | HUB8_CLK | LVCMOS33 | U18 pin 2A4 |
| P1 | HUB8_LAT | LVCMOS33 | U18 pin 2A5 |
| R2 | HUB8_OE | LVCMOS33 | U18 pin 2A6 |
| B2 | HUB9_R1 | LVCMOS33 | U19 pin 1A1 |
| C3 | HUB9_G1 | LVCMOS33 | U19 pin 1A2 |
| D2 | HUB9_B1 | LVCMOS33 | U19 pin 1A3 |
| E3 | HUB9_R2 | LVCMOS33 | U19 pin 1A4 |
| F2 | HUB9_G2 | LVCMOS33 | U19 pin 1A5 |
| G3 | HUB9_B2 | LVCMOS33 | U19 pin 1A6 |
| H2 | HUB9_A | LVCMOS33 | U19 pin 1A7 |
| J3 | HUB9_B | LVCMOS33 | U19 pin 1A8 |
| K2 | HUB9_C | LVCMOS33 | U19 pin 2A1 |
| L3 | HUB9_D | LVCMOS33 | U19 pin 2A2 |
| M2 | HUB9_E | LVCMOS33 | U19 pin 2A3 |
| N3 | HUB9_CLK | LVCMOS33 | U19 pin 2A4 |
| P2 | HUB9_LAT | LVCMOS33 | U19 pin 2A5 |
| R3 | HUB9_OE | LVCMOS33 | U19 pin 2A6 |
| B3 | HUB10_R1 | LVCMOS33 | U20 pin 1A1 |
| C4 | HUB10_G1 | LVCMOS33 | U20 pin 1A2 |
| D3 | HUB10_B1 | LVCMOS33 | U20 pin 1A3 |
| E4 | HUB10_R2 | LVCMOS33 | U20 pin 1A4 |
| F3 | HUB10_G2 | LVCMOS33 | U20 pin 1A5 |
| G4 | HUB10_B2 | LVCMOS33 | U20 pin 1A6 |
| H3 | HUB10_A | LVCMOS33 | U20 pin 1A7 |
| J4 | HUB10_B | LVCMOS33 | U20 pin 1A8 |
| K3 | HUB10_C | LVCMOS33 | U20 pin 2A1 |
| L4 | HUB10_D | LVCMOS33 | U20 pin 2A2 |
| M3 | HUB10_E | LVCMOS33 | U20 pin 2A3 |
| N4 | HUB10_CLK | LVCMOS33 | U20 pin 2A4 |
| P3 | HUB10_LAT | LVCMOS33 | U20 pin 2A5 |
| R4 | HUB10_OE | LVCMOS33 | U20 pin 2A6 |
| B4 | HUB11_R1 | LVCMOS33 | U21 pin 1A1 |
| C5 | HUB11_G1 | LVCMOS33 | U21 pin 1A2 |
| D4 | HUB11_B1 | LVCMOS33 | U21 pin 1A3 |
| E5 | HUB11_R2 | LVCMOS33 | U21 pin 1A4 |
| F4 | HUB11_G2 | LVCMOS33 | U21 pin 1A5 |
| G5 | HUB11_B2 | LVCMOS33 | U21 pin 1A6 |
| H4 | HUB11_A | LVCMOS33 | U21 pin 1A7 |
| J5 | HUB11_B | LVCMOS33 | U21 pin 1A8 |
| K4 | HUB11_C | LVCMOS33 | U21 pin 2A1 |
| L5 | HUB11_D | LVCMOS33 | U21 pin 2A2 |
| M4 | HUB11_E | LVCMOS33 | U21 pin 2A3 |
| N5 | HUB11_CLK | LVCMOS33 | U21 pin 2A4 |
| P4 | HUB11_LAT | LVCMOS33 | U21 pin 2A5 |
| R5 | HUB11_OE | LVCMOS33 | U21 pin 2A6 |

#### Bank 16 (HR, VCCO=3.3V) — HUB75E Ports 12-15

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| B5 | HUB12_R1 | LVCMOS33 | U22 pin 1A1 |
| C6 | HUB12_G1 | LVCMOS33 | U22 pin 1A2 |
| D5 | HUB12_B1 | LVCMOS33 | U22 pin 1A3 |
| E6 | HUB12_R2 | LVCMOS33 | U22 pin 1A4 |
| F5 | HUB12_G2 | LVCMOS33 | U22 pin 1A5 |
| G6 | HUB12_B2 | LVCMOS33 | U22 pin 1A6 |
| H5 | HUB12_A | LVCMOS33 | U22 pin 1A7 |
| J6 | HUB12_B | LVCMOS33 | U22 pin 1A8 |
| K5 | HUB12_C | LVCMOS33 | U22 pin 2A1 |
| L6 | HUB12_D | LVCMOS33 | U22 pin 2A2 |
| M5 | HUB12_E | LVCMOS33 | U22 pin 2A3 |
| N6 | HUB12_CLK | LVCMOS33 | U22 pin 2A4 |
| P5 | HUB12_LAT | LVCMOS33 | U22 pin 2A5 |
| R6 | HUB12_OE | LVCMOS33 | U22 pin 2A6 |
| B6 | HUB13_R1 | LVCMOS33 | U23 pin 1A1 |
| C7 | HUB13_G1 | LVCMOS33 | U23 pin 1A2 |
| D6 | HUB13_B1 | LVCMOS33 | U23 pin 1A3 |
| E7 | HUB13_R2 | LVCMOS33 | U23 pin 1A4 |
| F6 | HUB13_G2 | LVCMOS33 | U23 pin 1A5 |
| G7 | HUB13_B2 | LVCMOS33 | U23 pin 1A6 |
| H6 | HUB13_A | LVCMOS33 | U23 pin 1A7 |
| J7 | HUB13_B | LVCMOS33 | U23 pin 1A8 |
| K6 | HUB13_C | LVCMOS33 | U23 pin 2A1 |
| L7 | HUB13_D | LVCMOS33 | U23 pin 2A2 |
| M6 | HUB13_E | LVCMOS33 | U23 pin 2A3 |
| N7 | HUB13_CLK | LVCMOS33 | U23 pin 2A4 |
| P6 | HUB13_LAT | LVCMOS33 | U23 pin 2A5 |
| R7 | HUB13_OE | LVCMOS33 | U23 pin 2A6 |
| B7 | HUB14_R1 | LVCMOS33 | U24 pin 1A1 |
| C8 | HUB14_G1 | LVCMOS33 | U24 pin 1A2 |
| D7 | HUB14_B1 | LVCMOS33 | U24 pin 1A3 |
| E8 | HUB14_R2 | LVCMOS33 | U24 pin 1A4 |
| F7 | HUB14_G2 | LVCMOS33 | U24 pin 1A5 |
| G8 | HUB14_B2 | LVCMOS33 | U24 pin 1A6 |
| H7 | HUB14_A | LVCMOS33 | U24 pin 1A7 |
| J8 | HUB14_B | LVCMOS33 | U24 pin 1A8 |
| K7 | HUB14_C | LVCMOS33 | U24 pin 2A1 |
| L8 | HUB14_D | LVCMOS33 | U24 pin 2A2 |
| M7 | HUB14_E | LVCMOS33 | U24 pin 2A3 |
| N8 | HUB14_CLK | LVCMOS33 | U24 pin 2A4 |
| P7 | HUB14_LAT | LVCMOS33 | U24 pin 2A5 |
| R8 | HUB14_OE | LVCMOS33 | U24 pin 2A6 |
| B8 | HUB15_R1 | LVCMOS33 | U25 pin 1A1 |
| C9 | HUB15_G1 | LVCMOS33 | U25 pin 1A2 |
| D8 | HUB15_B1 | LVCMOS33 | U25 pin 1A3 |
| E9 | HUB15_R2 | LVCMOS33 | U25 pin 1A4 |
| F8 | HUB15_G2 | LVCMOS33 | U25 pin 1A5 |
| G9 | HUB15_B2 | LVCMOS33 | U25 pin 1A6 |
| H8 | HUB15_A | LVCMOS33 | U25 pin 1A7 |
| J9 | HUB15_B | LVCMOS33 | U25 pin 1A8 |
| K8 | HUB15_C | LVCMOS33 | U25 pin 2A1 |
| L9 | HUB15_D | LVCMOS33 | U25 pin 2A2 |
| M8 | HUB15_E | LVCMOS33 | U25 pin 2A3 |
| N9 | HUB15_CLK | LVCMOS33 | U25 pin 2A4 |
| P8 | HUB15_LAT | LVCMOS33 | U25 pin 2A5 |
| R9 | HUB15_OE | LVCMOS33 | U25 pin 2A6 |

#### Bank 34 (HP, VCCO=1.35V) — DDR3L Channel A (U8)

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| AA18 | DDRA_A0 | SSTL135 | U8 pin N7 (A0) |
| AB18 | DDRA_A1 | SSTL135 | U8 pin N2 (A1) |
| AC18 | DDRA_A2 | SSTL135 | U8 pin N3 (A2) |
| AD18 | DDRA_A3 | SSTL135 | U8 pin P8 (A3) |
| AE18 | DDRA_A4 | SSTL135 | U8 pin P3 (A4) |
| AF18 | DDRA_A5 | SSTL135 | U8 pin P2 (A5) |
| AG18 | DDRA_A6 | SSTL135 | U8 pin R8 (A6) |
| AH18 | DDRA_A7 | SSTL135 | U8 pin R3 (A7) |
| AJ18 | DDRA_A8 | SSTL135 | U8 pin R2 (A8) |
| AK18 | DDRA_A9 | SSTL135 | U8 pin T8 (A9) |
| AL18 | DDRA_A10 | SSTL135 | U8 pin T3 (A10) |
| AM18 | DDRA_A11 | SSTL135 | U8 pin T7 (A11) |
| AN18 | DDRA_A12 | SSTL135 | U8 pin M7 (A12/BC#) |
| AP18 | DDRA_A13 | SSTL135 | U8 pin N8 (A13) |
| AR18 | DDRA_A14 | SSTL135 | U8 pin M2 (A14) |
| AT18 | DDRA_A15 | SSTL135 | U8 pin M3 (A15) |
| AU18 | DDRA_BA0 | SSTL135 | U8 pin L7 (BA0) |
| AV18 | DDRA_BA1 | SSTL135 | U8 pin L3 (BA1) |
| AW18 | DDRA_BA2 | SSTL135 | U8 pin L2 (BA2) |
| AY18 | DDRA_CK_P | DIFF_SSTL135 | U8 pin K7 (CK) |
| BA18 | DDRA_CK_N | DIFF_SSTL135 | U8 pin K8 (CK#) |
| BB18 | DDRA_CKE | SSTL135 | U8 pin K2 (CKE) |
| BC18 | DDRA_CS_N | SSTL135 | U8 pin L8 (CS#) |
| BD18 | DDRA_RAS_N | SSTL135 | U8 pin J7 (RAS#) |
| BE18 | DDRA_CAS_N | SSTL135 | U8 pin J3 (CAS#) |
| BF18 | DDRA_WE_N | SSTL135 | U8 pin J2 (WE#) |
| BG18 | DDRA_ODT | SSTL135 | U8 pin K3 (ODT) |
| BH18 | DDRA_RESET_N | SSTL135 | U8 pin T2 (RESET#) |
| BJ18 | DDRA_DQ0 | SSTL135 | U8 pin G8 (DQ0) |
| BK18 | DDRA_DQ1 | SSTL135 | U8 pin G2 (DQ1) |
| BL18 | DDRA_DQ2 | SSTL135 | U8 pin H7 (DQ2) |
| BM18 | DDRA_DQ3 | SSTL135 | U8 pin H3 (DQ3) |
| BN18 | DDRA_DQ4 | SSTL135 | U8 pin H2 (DQ4) |
| BP18 | DDRA_DQ5 | SSTL135 | U8 pin F8 (DQ5) |
| BR18 | DDRA_DQ6 | SSTL135 | U8 pin F7 (DQ6) |
| BS18 | DDRA_DQ7 | SSTL135 | U8 pin F2 (DQ7) |
| BT18 | DDRA_DQ8 | SSTL135 | U8 pin F3 (DQ8) |
| BU18 | DDRA_DQ9 | SSTL135 | U8 pin D8 (DQ9) |
| BV18 | DDRA_DQ10 | SSTL135 | U8 pin D7 (DQ10) |
| BW18 | DDRA_DQ11 | SSTL135 | U8 pin D3 (DQ11) |
| BY18 | DDRA_DQ12 | SSTL135 | U8 pin C8 (DQ12) |
| C18  | DDRA_DQ13 | SSTL135 | U8 pin C3 (DQ13) |
| D18  | DDRA_DQ14 | SSTL135 | U8 pin C2 (DQ14) |
| E18  | DDRA_DQ15 | SSTL135 | U8 pin B8 (DQ15) |
| F18  | DDRA_DQS0_P | DIFF_SSTL135 | U8 pin G7 (UDQS) |
| G18  | DDRA_DQS0_N | DIFF_SSTL135 | U8 pin F7 (UDQS#) |
| H18  | DDRA_DQS1_P | DIFF_SSTL135 | U8 pin E7 (LDQS) |
| J18  | DDRA_DQS1_N | DIFF_SSTL135 | U8 pin D7 (LDQS#) |
| K18  | DDRA_DM0 | SSTL135 | U8 pin G3 (UDM) |
| L18  | DDRA_DM1 | SSTL135 | U8 pin E3 (LDM) |

#### Bank 35 (HP, VCCO=1.35V) — DDR3L Channel B (U9)

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| M18  | DDRB_A0 | SSTL135 | U9 pin N7 (A0) |
| N18  | DDRB_A1 | SSTL135 | U9 pin N2 (A1) |
| P18  | DDRB_A2 | SSTL135 | U9 pin N3 (A2) |
| R18  | DDRB_A3 | SSTL135 | U9 pin P8 (A3) |
| T18  | DDRB_A4 | SSTL135 | U9 pin P3 (A4) |
| U18  | DDRB_A5 | SSTL135 | U9 pin P2 (A5) |
| V18  | DDRB_A6 | SSTL135 | U9 pin R8 (A6) |
| W18  | DDRB_A7 | SSTL135 | U9 pin R3 (A7) |
| Y18  | DDRB_A8 | SSTL135 | U9 pin R2 (A8) |
| AA19 | DDRB_A9 | SSTL135 | U9 pin T8 (A9) |
| AB19 | DDRB_A10 | SSTL135 | U9 pin T3 (A10) |
| AC19 | DDRB_A11 | SSTL135 | U9 pin T7 (A11) |
| AD19 | DDRB_A12 | SSTL135 | U9 pin M7 (A12/BC#) |
| AE19 | DDRB_A13 | SSTL135 | U9 pin N8 (A13) |
| AF19 | DDRB_A14 | SSTL135 | U9 pin M2 (A14) |
| AG19 | DDRB_A15 | SSTL135 | U9 pin M3 (A15) |
| AH19 | DDRB_BA0 | SSTL135 | U9 pin L7 (BA0) |
| AJ19 | DDRB_BA1 | SSTL135 | U9 pin L3 (BA1) |
| AK19 | DDRB_BA2 | SSTL135 | U9 pin L2 (BA2) |
| AL19 | DDRB_CK_P | DIFF_SSTL135 | U9 pin K7 (CK) |
| AM19 | DDRB_CK_N | DIFF_SSTL135 | U9 pin K8 (CK#) |
| AN19 | DDRB_CKE | SSTL135 | U9 pin K2 (CKE) |
| AP19 | DDRB_CS_N | SSTL135 | U9 pin L8 (CS#) |
| AR19 | DDRB_RAS_N | SSTL135 | U9 pin J7 (RAS#) |
| AT19 | DDRB_CAS_N | SSTL135 | U9 pin J3 (CAS#) |
| AU19 | DDRB_WE_N | SSTL135 | U9 pin J2 (WE#) |
| AV19 | DDRB_ODT | SSTL135 | U9 pin K3 (ODT) |
| AW19 | DDRB_RESET_N | SSTL135 | U9 pin T2 (RESET#) |
| AY19 | DDRB_DQ0 | SSTL135 | U9 pin G8 (DQ0) |
| BA19 | DDRB_DQ1 | SSTL135 | U9 pin G2 (DQ1) |
| BB19 | DDRB_DQ2 | SSTL135 | U9 pin H7 (DQ2) |
| BC19 | DDRB_DQ3 | SSTL135 | U9 pin H3 (DQ3) |
| BD19 | DDRB_DQ4 | SSTL135 | U9 pin H2 (DQ4) |
| BE19 | DDRB_DQ5 | SSTL135 | U9 pin F8 (DQ5) |
| BF19 | DDRB_DQ6 | SSTL135 | U9 pin F7 (DQ6) |
| BG19 | DDRB_DQ7 | SSTL135 | U9 pin F2 (DQ7) |
| BH19 | DDRB_DQ8 | SSTL135 | U9 pin F3 (DQ8) |
| BJ19 | DDRB_DQ9 | SSTL135 | U9 pin D8 (DQ9) |
| BK19 | DDRB_DQ10 | SSTL135 | U9 pin D7 (DQ10) |
| BL19 | DDRB_DQ11 | SSTL135 | U9 pin D3 (DQ11) |
| BM19 | DDRB_DQ12 | SSTL135 | U9 pin C8 (DQ12) |
| BN19 | DDRB_DQ13 | SSTL135 | U9 pin C3 (DQ13) |
| BP19 | DDRB_DQ14 | SSTL135 | U9 pin C2 (DQ14) |
| BR19 | DDRB_DQ15 | SSTL135 | U9 pin B8 (DQ15) |
| BS19 | DDRB_DQS0_P | DIFF_SSTL135 | U9 pin G7 (UDQS) |
| BT19 | DDRB_DQS0_N | DIFF_SSTL135 | U9 pin F7 (UDQS#) |
| BU19 | DDRB_DQS1_P | DIFF_SSTL135 | U9 pin E7 (LDQS) |
| BV19 | DDRB_DQS1_N | DIFF_SSTL135 | U9 pin D7 (LDQS#) |
| BW19 | DDRB_DM0 | SSTL135 | U9 pin G3 (UDM) |
| BY19 | DDRB_DM1 | SSTL135 | U9 pin E3 (LDM) |

#### Bank 12 (HR, VCCO=3.3V) — HDMI Input (ADV7611)

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| Y8  | HDMI_D0 | LVCMOS33 | U7 pin 49 (P0) |
| AA8 | HDMI_D1 | LVCMOS33 | U7 pin 48 (P1) |
| AB8 | HDMI_D2 | LVCMOS33 | U7 pin 47 (P2) |
| AC8 | HDMI_D3 | LVCMOS33 | U7 pin 46 (P3) |
| AD8 | HDMI_D4 | LVCMOS33 | U7 pin 45 (P4) |
| AE8 | HDMI_D5 | LVCMOS33 | U7 pin 44 (P5) |
| AF8 | HDMI_D6 | LVCMOS33 | U7 pin 43 (P6) |
| AG8 | HDMI_D7 | LVCMOS33 | U7 pin 42 (P7) |
| AH8 | HDMI_D8 | LVCMOS33 | U7 pin 41 (P8) |
| AJ8 | HDMI_D9 | LVCMOS33 | U7 pin 40 (P9) |
| AK8 | HDMI_D10 | LVCMOS33 | U7 pin 39 (P10) |
| AL8 | HDMI_D11 | LVCMOS33 | U7 pin 38 (P11) |
| AM8 | HDMI_D12 | LVCMOS33 | U7 pin 37 (P12) |
| AN8 | HDMI_D13 | LVCMOS33 | U7 pin 36 (P13) |
| AP8 | HDMI_D14 | LVCMOS33 | U7 pin 35 (P14) |
| AR8 | HDMI_D15 | LVCMOS33 | U7 pin 34 (P15) |
| AT8 | HDMI_D16 | LVCMOS33 | U7 pin 33 (P16) |
| AU8 | HDMI_D17 | LVCMOS33 | U7 pin 32 (P17) |
| AV8 | HDMI_D18 | LVCMOS33 | U7 pin 31 (P18) |
| AW8 | HDMI_D19 | LVCMOS33 | U7 pin 30 (P19) |
| AY8 | HDMI_D20 | LVCMOS33 | U7 pin 29 (P20) |
| BA8 | HDMI_D21 | LVCMOS33 | U7 pin 28 (P21) |
| BB8 | HDMI_D22 | LVCMOS33 | U7 pin 27 (P22) |
| BC8 | HDMI_D23 | LVCMOS33 | U7 pin 26 (P23) |
| BD8 | HDMI_HS | LVCMOS33 | U7 pin 25 (HSYNC) |
| BE8 | HDMI_VS | LVCMOS33 | U7 pin 24 (VSYNC) |
| BF8 | HDMI_DE | LVCMOS33 | U7 pin 23 (DE) |
| BG8 | HDMI_CLK | LVCMOS33 | U7 pin 22 (ODCK) |
| BH8 | HDMI_SCL | LVCMOS33 | U7 pin 55 (SCL) |
| BJ8 | HDMI_SDA | LVCMOS33 | U7 pin 54 (SDA) |
| BK8 | HDMI_INT1 | LVCMOS33 | U7 pin 19 (INT1) |
| BL8 | HDMI_RESET | LVCMOS33 | U7 pin 17 (RESET) |

#### Bank 13 (remaining) — STM32H7 SPI/UART Interface

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| W7  | FPGA_SPI_SCK | LVCMOS33 | U2 pin 26 (SPI2_SCK, PB13) |
| V8  | FPGA_SPI_MOSI | LVCMOS33 | U2 pin 27 (SPI2_MOSI, PB15) |
| U7  | FPGA_SPI_MISO | LVCMOS33 | U2 pin 28 (SPI2_MISO, PB14) |
| T8  | FPGA_SPI_CS | LVCMOS33 | U2 pin 29 (SPI2_NSS, PB12) |
| R7  | FPGA_UART_TX | LVCMOS33 | U2 pin 68 (USART1_RX, PA10) |
| P8  | FPGA_UART_RX | LVCMOS33 | U2 pin 67 (USART1_TX, PA9) |
| N7  | FPGA_IRQ | LVCMOS33 | U2 pin 95 (PE0) |
| M8  | FPGA_READY | LVCMOS33 | U2 pin 96 (PE1) |
| L7  | FPGA_RESET_N | LVCMOS33 | U2 pin 97 (PE2) |
| K8  | FPGA_DONE | LVCMOS33 | U2 pin 98 (PE3) |

#### Bank 0 (HR, VCCO=3.3V) — FPGA Configuration

| FPGA Pin | Net Name | Function | Destination |
|----------|----------|----------|-------------|
| Y3  | FPGA_CFG_M0 | LVCMOS33 | 4.7kΩ pull-up to 3.3V (M[1:0]=01, SPI Master) |
| AA3 | FPGA_CFG_M1 | LVCMOS33 | 4.7kΩ pull-down to GND |
| AB3 | FPGA_CFG_M2 | LVCMOS33 | 4.7kΩ pull-up to 3.3V |
| AC3 | FPGA_CCLK | LVCMOS33 | U26 pin 6 (SCK) |
| AD3 | FPGA_DIN | LVCMOS33 | U26 pin 2 (SO/IO1) |
| AE3 | FPGA_DOUT | LVCMOS33 | U26 pin 5 (SI/IO0) |
| AF3 | FPGA_CSO_B | LVCMOS33 | U26 pin 1 (CS#) |
| AG3 | FPGA_PROGRAM_B | LVCMOS33 | 4.7kΩ pull-up to 3.3V, SW2 to GND |
| AH3 | FPGA_INIT_B | LVCMOS33 | 4.7kΩ pull-up to 3.3V |
| AJ3 | FPGA_DONE | LVCMOS33 | 4.7kΩ pull-up to 3.3V, LED1 (green) |
| AK3 | FPGA_TCK | LVCMOS33 | J23 pin 9 (JTAG TCK) |
| AL3 | FPGA_TDI | LVCMOS33 | J23 pin 5 (JTAG TDI) |
| AM3 | FPGA_TDO | LVCMOS33 | J23 pin 13 (JTAG TDO) |
| AN3 | FPGA_TMS | LVCMOS33 | J23 pin 7 (JTAG TMS) |

### 3.2 STM32H743VIT6 (U2) Pin Assignments

| Pin | Net Name | Function | Destination |
|-----|----------|----------|-------------|
| 8  | STM32_NRST | RESET | SW1 (reset button), 10kΩ pull-up to 3.3V |
| 9  | STM32_OSC_IN | HSE-OSC-IN | Si5351A CLK0 (25 MHz) |
| 10 | STM32_OSC_OUT | HSE-OSC-OUT | NC (leave floating for external clock) |
| 14 | STM32_SWDIO | SWDIO | J22 pin 2 (TC2050 SWDIO) |
| 15 | STM32_SWCLK | SWCLK | J22 pin 4 (TC2050 SWCLK) |
| 16 | STM32_SWO | SWO | J22 pin 6 (TC2050 SWO) |
| 20 | STM32_PA0 | TIM1_CH1 | FAN1 PWM (via 2N7002 MOSFET) |
| 21 | STM32_PA1 | TIM1_CH2 | FAN2 PWM (via 2N7002 MOSFET) |
| 22 | STM32_PA2 | ADC1_IN2 | INA219 Vshunt (current sense) |
| 23 | STM32_PA3 | ADC1_IN3 | 12V input voltage divider (10k/3.3k) |
| 24 | STM32_PA4 | SPI1_NSS | U26 pin 1 (FPGA flash, shared CS) |
| 25 | STM32_PA5 | SPI1_SCK | U26 pin 6 (FPGA flash SCK) |
| 26 | STM32_PA6 | SPI1_MISO | U26 pin 2 (FPGA flash SO) |
| 27 | STM32_PA7 | SPI1_MOSI | U26 pin 5 (FPGA flash SI) |
| 28 | STM32_PB0 | GPIO_OUT | LED2 (amber, status) |
| 29 | STM32_PB1 | GPIO_OUT | LED3 (red, error) |
| 30 | STM32_PB2 | GPIO_OUT | LED4 (blue, Ethernet activity) |
| 35 | STM32_PC4 | ETH_RMII_RXD0 | U5 pin 33 (RXD0) |
| 36 | STM32_PC5 | ETH_RMII_RXD1 | U5 pin 32 (RXD1) |
| 37 | STM32_PB13 | SPI2_SCK | FPGA pin W7 (FPGA_SPI_SCK) |
| 38 | STM32_PB14 | SPI2_MISO | FPGA pin U7 (FPGA_SPI_MISO) |
| 39 | STM32_PB15 | SPI2_MOSI | FPGA pin V8 (FPGA_SPI_MOSI) |
| 40 | STM32_PB12 | SPI2_NSS | FPGA pin T8 (FPGA_SPI_CS) |
| 44 | STM32_PA8 | ETH_RMII_CRS_DV | U5 pin 38 (CRS_DV) |
| 45 | STM32_PC1 | ETH_RMII_MDC | U5 pin 25 (MDC) |
| 46 | STM32_PA9 | USART1_TX | FPGA pin P8 (FPGA_UART_RX) |
| 47 | STM32_PA10 | USART1_RX | FPGA pin R7 (FPGA_UART_TX) |
| 48 | STM32_PC2 | ETH_RMII_TXD0 | U5 pin 29 (TXD0) |
| 49 | STM32_PC3 | ETH_RMII_TXD1 | U5 pin 30 (TXD1) |
| 50 | STM32_PA11 | USB_OTG_FS_DM | U6 pin 22 (DM) |
| 51 | STM32_PA12 | USB_OTG_FS_DP | U6 pin 23 (DP) |
| 52 | STM32_PC0 | ETH_RMII_TX_EN | U5 pin 28 (TXEN) |
| 58 | STM32_PB5 | ETH_RMII_RX_ER | U5 pin 37 (RXER) |
| 59 | STM32_PB6 | I2C2_SCL | U27 (TMP117 #1) pin 4 (SCL) |
| 60 | STM32_PB7 | I2C2_SDA | U27 (TMP117 #1) pin 3 (SDA) |
| 61 | STM32_PB8 | I2C3_SCL | U4 (Si5351A) pin 9 (SCL) |
| 62 | STM32_PB9 | I2C3_SDA | U4 (Si5351A) pin 10 (SDA) |
| 63 | STM32_PE0 | GPIO_IN | FPGA pin N7 (FPGA_IRQ) |
| 64 | STM32_PE1 | GPIO_IN | FPGA pin M8 (FPGA_READY) |
| 65 | STM32_PE2 | GPIO_OUT | FPGA pin L7 (FPGA_RESET_N) |
| 66 | STM32_PE3 | GPIO_IN | FPGA pin K8 (FPGA_DONE) |
| 67 | STM32_PB10 | I2C4_SCL | U3 (TPS65218) pin 43 (SCL) |
| 68 | STM32_PB11 | I2C4_SDA | U3 (TPS65218) pin 44 (SDA) |
| 69 | STM32_PC6 | SDMMC1_D6 | J3 pin 6 (DAT2) |
| 70 | STM32_PC7 | SDMMC1_D7 | J3 pin 1 (DAT3/CD) |
| 71 | STM32_PC8 | SDMMC1_D0 | J3 pin 7 (DAT0) |
| 72 | STM32_PC9 | SDMMC1_D1 | J3 pin 8 (DAT1) |
| 73 | STM32_PC10 | SDMMC1_D2 | J3 pin 2 (CMD) |
| 74 | STM32_PC11 | SDMMC1_D3 | J3 pin 5 (CLK) |
| 75 | STM32_PC12 | SDMMC1_CK | J3 pin 3 (VSS) |
| 76 | STM32_PD2 | SDMMC1_CMD | J3 pin 4 (VDD) |
| 78 | STM32_PB3 | GPIO_OUT | U7 (ADV7611) pin 17 (HDMI_RESET) |
| 79 | STM32_PB4 | GPIO_IN | U7 (ADV7611) pin 19 (HDMI_INT1) |
| 80 | STM32_PE4 | I2C2_SCL2 | U27 (TMP117 #2) pin 4 (SCL) |
| 81 | STM32_PE5 | I2C2_SDA2 | U27 (TMP117 #2) pin 3 (SDA) |
| 82 | STM32_PE6 | I2C2_SCL3 | U27 (TMP117 #3) pin 4 (SCL) |
| 83 | STM32_PE7 | I2C2_SDA3 | U27 (TMP117 #3) pin 3 (SDA) |
| 84 | STM32_PE8 | I2C2_SCL4 | U27 (TMP117 #4) pin 4 (SCL) |
| 85 | STM32_PE9 | I2C2_SDA4 | U27 (TMP117 #4) pin 3 (SDA) |
| 88 | STM32_PH0 | ETH_RMII_REF_CLK | U5 pin 26 (REF_CLK) |
| 89 | STM32_PH1 | ETH_RMII_MDIO | U5 pin 24 (MDIO) |
| 90 | STM32_PD8 | USART3_TX | Debug header (optional) |
| 91 | STM32_PD9 | USART3_RX | Debug header (optional) |
| 95 | STM32_PE10 | GPIO_OUT | U5 pin 23 (ETH_RESET_N) |
| 96 | STM32_PE11 | GPIO_OUT | U6 pin 27 (USB_RESET_N) |
| 97 | STM32_PE12 | GPIO_IN | U5 pin 39 (ETH_INT_N) |
| 98 | STM32_PE13 | GPIO_IN | U6 pin 3 (USB_INT_N) |
| 99 | STM32_PE14 | GPIO_OUT | U3 pin 46 (PMIC_EN) |
| 100| STM32_PE15 | GPIO_IN | U3 pin 47 (PMIC_INT) |

### 3.3 ADV7611 (U7) HDMI Receiver Pin Assignments

| Pin | Net Name | Function | Destination |
|-----|----------|----------|-------------|
| 1  | HDMI_RXC_2+ | TMDS Clock+ | J21 pin 10 (HDMI CLK+) |
| 2  | HDMI_RXC_2- | TMDS Clock- | J21 pin 12 (HDMI CLK-) |
| 3  | HDMI_RX0_2+ | TMDS Data2+ | J21 pin 1 (HDMI D2+) |
| 4  | HDMI_RX0_2- | TMDS Data2- | J21 pin 3 (HDMI D2-) |
| 5  | HDMI_RX1_2+ | TMDS Data1+ | J21 pin 4 (HDMI D1+) |
| 6  | HDMI_RX1_2- | TMDS Data1- | J21 pin 6 (HDMI D1-) |
| 7  | HDMI_RX2_2+ | TMDS Data0+ | J21 pin 7 (HDMI D0+) |
| 8  | HDMI_RX2_2- | TMDS Data0- | J21 pin 9 (HDMI D0-) |
| 9  | HDMI_DDC_SCL | DDC I2C | J21 pin 15 (HDMI SCL), 47kΩ pull-up to 5V |
| 10 | HDMI_DDC_SDA | DDC I2C | J21 pin 16 (HDMI SDA), 47kΩ pull-up to 5V |
| 11 | HDMI_HPA_D | Hotplug Detect | J21 pin 19 (HDMI HPD), 1kΩ to 5V |
| 12 | HDMI_CEC | CEC | J21 pin 14 (HDMI CEC), 27kΩ pull-up to 3.3V |
| 17 | HDMI_RESET | RESET (active low) | STM32 pin 78 (PB3) |
| 19 | HDMI_INT1 | Interrupt | STM32 pin 79 (PB4) |
| 22 | HDMI_ODCK | Output Data Clock | FPGA pin BG8 |
| 23 | HDMI_DE | Data Enable | FPGA pin BF8 |
| 24 | HDMI_VS | Vertical Sync | FPGA pin BE8 |
| 25 | HDMI_HS | Horizontal Sync | FPGA pin BD8 |
| 26-33 | HDMI_P23-P16 | Pixel Data [23:16] | FPGA pins BC8-AU8 |
| 34-41 | HDMI_P15-P8 | Pixel Data [15:8] | FPGA pins AT8-AL8 |
| 42-49 | HDMI_P7-P0 | Pixel Data [7:0] | FPGA pins AK8-Y8 |
| 54 | HDMI_SDA | Config I2C Data | FPGA pin BJ8 |
| 55 | HDMI_SCL | Config I2C Clock | FPGA pin BH8 |
| 56 | HDMI_XTALP | 27 MHz Crystal | Si5351A CLK2 (27 MHz) |
| 57 | HDMI_XTALN | 27 MHz Crystal | GND via 22pF cap |

### 3.4 KSZ9031RNX (U5) GbE PHY Pin Assignments

| Pin | Net Name | Function | Destination |
|-----|----------|----------|-------------|
| 23 | ETH_RESET_N | Reset | STM32 pin 95 (PE10) |
| 24 | ETH_MDIO | MDIO | STM32 pin 89 (PH1) |
| 25 | ETH_MDC | MDC | STM32 pin 45 (PC1) |
| 26 | ETH_REF_CLK | 25 MHz RMII Ref | STM32 pin 88 (PH0) |
| 28 | ETH_TXEN | TX Enable | STM32 pin 52 (PC0) |
| 29 | ETH_TXD0 | TX Data 0 | STM32 pin 48 (PC2) |
| 30 | ETH_TXD1 | TX Data 1 | STM32 pin 49 (PC3) |
| 32 | ETH_RXD1 | RX Data 1 | STM32 pin 36 (PC5) |
| 33 | ETH_RXD0 | RX Data 0 | STM32 pin 35 (PC4) |
| 37 | ETH_RXER | RX Error | STM32 pin 58 (PB5) |
| 38 | ETH_CRS_DV | Carrier Sense | STM32 pin 44 (PA8) |
| 39 | ETH_INT_N | Interrupt | STM32 pin 97 (PE12) |
| 40 | ETH_LED0 | Link LED | LED4 (blue) via 220Ω |
| 41 | ETH_LED1 | Activity LED | LED4 (blue) via 220Ω |
| 1,8 | ETH_MDI0_P/N | MDI Pair 0 | J1 pins 1,2 (RJ45 TPTX+) |
| 2,9 | ETH_MDI1_P/N | MDI Pair 1 | J1 pins 3,4 (RJ45 TPTX-) |
| 3,10| ETH_MDI2_P/N | MDI Pair 2 | J1 pins 5,6 (RJ45 TPRX+) |
| 4,11| ETH_MDI3_P/N | MDI Pair 3 | J1 pins 7,8 (RJ45 TPRX-) |

### 3.5 USB3320C (U6) ULPI PHY Pin Assignments

| Pin | Net Name | Function | Destination |
|-----|----------|----------|-------------|
| 3  | USB_INT_N | Interrupt | STM32 pin 98 (PE13) |
| 22 | USB_DM | USB D- | STM32 pin 50 (PA11) |
| 23 | USB_DP | USB D+ | STM32 pin 51 (PA12) |
| 24 | USB_VBUS | VBUS detect | J2 pin A4 (USB-C VBUS) via 30k/10k divider |
| 25 | USB_ID | ID pin | J2 pin A5 (USB-C CC1) |
| 26 | USB_CLKOUT | 60 MHz ULPI CLK | STM32 pin PA5 (ULPI_CLK) |
| 27 | USB_RESET_N | Reset | STM32 pin 96 (PE11) |
| 28 | USB_STP | ULPI STP | STM32 pin PC0 (ULPI_STP) |
| 29 | USB_DIR | ULPI DIR | STM32 pin PC1 (ULPI_DIR) |
| 30 | USB_NXT | ULPI NXT | STM32 pin PC2 (ULPI_NXT) |
| 31 | USB_DATA0 | ULPI DATA0 | STM32 pin PC3 (ULPI_D0) |
| 32 | USB_DATA1 | ULPI DATA1 | STM32 pin PA0 (ULPI_D1) |
| 1  | USB_DATA2 | ULPI DATA2 | STM32 pin PA1 (ULPI_D2) |
| 2  | USB_DATA3 | ULPI DATA3 | STM32 pin PA2 (ULPI_D3) |
| 4  | USB_DATA4 | ULPI DATA4 | STM32 pin PA3 (ULPI_D4) |
| 5  | USB_DATA5 | ULPI DATA5 | STM32 pin PA4 (ULPI_D5) |
| 6  | USB_DATA6 | ULPI DATA6 | STM32 pin PA6 (ULPI_D6) |
| 7  | USB_DATA7 | ULPI DATA7 | STM32 pin PA7 (ULPI_D7) |
| 8  | USB_REFSEL0 | Clock source | GND (internal oscillator) |
| 9  | USB_REFSEL1 | Clock source | GND |
| 10 | USB_XTALIN | 60 MHz ref | Si5351A CLK4 (60 MHz) |
| 11 | USB_XTALOUT | NC | Float |

---

## 4. Netlists — Key Signal Paths

### 4.1 Pixel Data Path (Ethernet → LED Panel)

```
J1(RJ45) → Magnetics → U5(KSZ9031) MDI pairs
U5 RGMII → STM32H7 ETH MAC
STM32H7 LWIP → UDP packet buffer (SRAM)
STM32H7 SPI2 DMA → FPGA SPI Slave
  Net: STM32_PB13(SPI2_SCK) → R22(22Ω) → FPGA_W7(FPGA_SPI_SCK)
  Net: STM32_PB15(SPI2_MOSI) → R23(22Ω) → FPGA_V8(FPGA_SPI_MOSI)
  Net: STM32_PB14(SPI2_MISO) → R24(22Ω) → FPGA_U7(FPGA_SPI_MISO)
  Net: STM32_PB12(SPI2_NSS) → R25(22Ω) → FPGA_T8(FPGA_SPI_CS)
FPGA Pixel FIFO → AXI Stream → Frame Buffer Write DMA
FPGA DDR3 MIG → U8(AS4C256M16D3LC) Channel A
FPGA DDR3 MIG → U9(AS4C256M16D3LC) Channel B
FPGA Frame Buffer Read → Pixel Processing Pipeline
FPGA HUB75E Waveform Gen → Bank 13/14/15/16 I/O
  Net: FPGA_F4(HUB0_R1) → R26(22Ω) → U10(SN74LVCH16T245) 1A1
  U10 1B1 → R27(10kΩ pulldown) → D1(PESD5V0S1BA) → J5(HUB75E Port 0) pin 1
  ... (repeat for all 224 signals)
```

### 4.2 HDMI Capture Path

```
J21(HDMI Type-A) → TMDS pairs → U7(ADV7611)
  Net: J21_pin1(D2+) → U7_pin3(RX0_2+)
  Net: J21_pin3(D2-) → U7_pin4(RX0_2-)
  Net: J21_pin4(D1+) → U7_pin5(RX1_2+)
  Net: J21_pin6(D1-) → U7_pin6(RX1_2-)
  Net: J21_pin7(D0+) → U7_pin7(RX2_2+)
  Net: J21_pin9(D0-) → U7_pin8(RX2_2-)
  Net: J21_pin10(CLK+) → U7_pin1(RXC_2+)
  Net: J21_pin12(CLK-) → U7_pin2(RXC_2-)
U7 Parallel Output → FPGA Bank 12
  Net: U7_pin22(ODCK) → FPGA_BG8(HDMI_CLK)
  Net: U7_pin23(DE) → FPGA_BF8(HDMI_DE)
  Net: U7_pin24(VS) → FPGA_BE8(HDMI_VS)
  Net: U7_pin25(HS) → FPGA_BD8(HDMI_HS)
  Net: U7_pin26-49(P23-P0) → FPGA_BC8-Y8(HDMI_D23-D0)
```

### 4.3 Power Distribution Netlist

```
J4(DC Jack) pin 1 (+12V) → F1(Polyfuse 3A) → Net: +12V_IN
+12V_IN → U3(TPS65218) pin 33,34 (DCDC1_IN)
+12V_IN → U3 pin 35,36 (DCDC2_IN)
+12V_IN → U3 pin 37,38 (DCDC3_IN)
+12V_IN → U3 pin 39,40 (DCDC4_IN)
+12V_IN → U3 pin 41 (LDO1_IN)
+12V_IN → U3 pin 42 (LDO2_IN)
+12V_IN → FAN1, FAN2 (via MOSFET switches)

U3 DCDC1 (5.0V) → Net: +5V0
  +5V0 → U10-U25 VCCB (SN74LVCH16T245, 16×)
  +5V0 → J5-J20 pin 13 (HUB75E panel power passthrough, optional)
  +5V0 → J2 VBUS (via 500mA current limiter AP2331)

U3 DCDC2 (3.3V) → Net: +3V3
  +3V3 → FPGA VCCO Banks 0,12,13,14,15,16
  +3V3 → U2 VDD (STM32H7)
  +3V3 → U5 DVDDH (KSZ9031)
  +3V3 → U6 VDDIO (USB3320C)
  +3V3 → U7 DVDDIO (ADV7611)
  +3V3 → U4 VDD (Si5351A)
  +3V3 → U26 VCC (W25Q128)
  +3V3 → J3 VDD (microSD)
  +3V3 → U27 VDD (TMP117 ×4)
  +3V3 → U28 VDD (INA219)

U3 DCDC3 (1.8V) → Net: +1V8
  +1V8 → FPGA VCCAUX
  +1V8 → U7 DVDD (ADV7611)
  +1V8 → U5 DVDDL (KSZ9031)

U3 DCDC4 (1.35V) → Net: +1V35
  +1V35 → FPGA VCCO_DDR (Banks 34,35)
  +1V35 → U8 VDDQ (DDR3L Ch A)
  +1V35 → U9 VDDQ (DDR3L Ch B)
  +1V35 → U30(TP51200) VDDQ sense → VTT = 0.675V

U3 LDO1 (1.0V) → Net: +1V0
  +1V0 → FPGA VCCINT

U3 LDO2 (1.2V) → Net: +1V2
  +1V2 → FPGA VCCBRAM

U3 LDO3 (3.3V) → Net: +3V3_MGT (unused, for future GTP)
```

---

## 5. Impedance-Matched Pairs

### 5.1 Differential Pairs

| Pair Name | Impedance | Length Match | Layer |
|-----------|-----------|--------------|-------|
| DDRA_CK_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| DDRB_CK_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| DDRA_DQS0_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| DDRA_DQS1_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| DDRB_DQS0_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| DDRB_DQS1_P/N | 100Ω diff | ±0.5mm within pair | L1 (top) |
| HDMI_TMDS_CLK | 100Ω diff | ±0.25mm within pair | L1 (top) |
| HDMI_TMDS_D0 | 100Ω diff | ±0.25mm within pair | L1 (top) |
| HDMI_TMDS_D1 | 100Ω diff | ±0.25mm within pair | L1 (top) |
| HDMI_TMDS_D2 | 100Ω diff | ±0.25mm within pair | L1 (top) |
| USB_DP/DM | 90Ω diff | ±0.25mm within pair | L1 (top) |
| ETH_MDI0-MDI3 | 100Ω diff | ±0.25mm within pair | L1 (top) |

### 5.2 Single-Ended Controlled Impedance

| Net Group | Impedance | Trace Width | Layer |
|-----------|-----------|-------------|-------|
| DDR3 Address/Command | 40Ω ±10% | Calculated per stackup | L1 (top) |
| DDR3 DQ/DQS/DM | 40Ω ±10% | Calculated per stackup | L1 (top) |
| HUB75E Data/Addr | 50Ω ±10% | Calculated per stackup | L1 (top) |
| SPI (50 MHz) | 50Ω ±10% | Calculated per stackup | L1 (top) |
| HDMI Parallel Bus | 50Ω ±10% | Calculated per stackup | L1 (top) |

---

## 6. Pull-Up / Pull-Down Resistor Summary

| Net | Resistor | Value | To Rail | Purpose |
|-----|----------|-------|---------|---------|
| FPGA_PROGRAM_B | R1 | 4.7kΩ | +3V3 | Default inactive |
| FPGA_INIT_B | R2 | 4.7kΩ | +3V3 | Default inactive |
| FPGA_DONE | R3 | 4.7kΩ | +3V3 | Default inactive, LED indicator |
| FPGA_CFG_M0 | R4 | 4.7kΩ | +3V3 | SPI Master mode |
| FPGA_CFG_M1 | R5 | 4.7kΩ | GND | SPI Master mode |
| FPGA_CFG_M2 | R6 | 4.7kΩ | +3V3 | SPI Master mode |
| STM32_NRST | R7 | 10kΩ | +3V3 | Default not reset |
| HDMI_DDC_SCL | R8 | 47kΩ | +5V0 | I2C open-drain |
| HDMI_DDC_SDA | R9 | 47kΩ | +5V0 | I2C open-drain |
| HDMI_HPA_D | R10 | 1kΩ | +5V0 | Hotplug assert |
| HDMI_CEC | R11 | 27kΩ | +3V3 | CEC line |
| I2C2_SCL (all 4) | R12-R15 | 4.7kΩ | +3V3 | I2C open-drain |
| I2C2_SDA (all 4) | R16-R19 | 4.7kΩ | +3V3 | I2C open-drain |
| I2C3_SCL | R20 | 4.7kΩ | +3V3 | Si5351A I2C |
| I2C3_SDA | R21 | 4.7kΩ | +3V3 | Si5351A I2C |
| I2C4_SCL | R22 | 4.7kΩ | +3V3 | TPS65218 I2C |
| I2C4_SDA | R23 | 4.7kΩ | +3V3 | TPS65218 I2C |
| HUB75E outputs (×224) | R30-R253 | 10kΩ | GND | Prevent floating |
| DDR3 RZQ (U8) | R254 | 240Ω ±1% | GND | ODT calibration |
| DDR3 RZQ (U9) | R255 | 240Ω ±1% | GND | ODT calibration |
| USB_CC1 | R256 | 5.1kΩ | GND | USB-C downstream |
| USB_CC2 | R257 | 5.1kΩ | GND | USB-C downstream |

---

## 7. Decoupling Capacitor Placement Map

### 7.1 FPGA VCCINT (1.0V) — 484-BGA Center Region

Place 0402 MLCCs on bottom side directly under BGA ball field:
- 0.47µF: One per 4 VCCINT balls (~30 caps), placed in grid under BGA
- 0.01µF: One per 2 VCCINT balls (~15 caps), interleaved with 0.47µF
- 100µF tantalum: 4× at BGA perimeter, top side

### 7.2 FPGA VCCO Banks (3.3V)

Per bank (Banks 0,12,13,14,15,16):
- 0.1µF 0402: One per 2 VCCO balls, placed on bottom side
- 47µF tantalum: 2× per bank, top side near bank edge

### 7.3 DDR3L Decoupling

Per DRAM chip (U8, U9):
- 0.1µF 0402: 6× around BGA perimeter (VDD, VDDQ, VREF)
- 10µF 0603: 2× near VDDQ balls
- 47µF tantalum: 1× near chip

### 7.4 STM32H7 Decoupling

Per STM32H7 reference manual (AN4488):
- 0.1µF 0402: One per VDD/VDDIO pin pair (10×)
- 4.7µF 0603: 2× on VDD rail
- 10µF 0603: 1× on VDDA (analog supply)

---

## 8. Connector Pinouts

### 8.1 HUB75E Output Connector (J5-J20, 2×7 Right-Angle Header)

| Pin | Signal | Pin | Signal |
|-----|--------|-----|--------|
| 1   | R1     | 2   | G1     |
| 3   | B1     | 4   | GND    |
| 5   | R2     | 6   | G2     |
| 7   | B2     | 8   | E      |
| 9   | A      | 10  | B      |
| 11  | C      | 12  | D      |
| 13  | CLK    | 14  | LAT    |
| —   | OE (pin 15 on 16-pin variant, or separate) | | |

Note: Standard HUB75E is 2×8 (16-pin). We use 2×7 (14-pin) with OE routed separately
or combined with a 16-pin header. For maximum compatibility, use 2×8 headers with
pin 15 = OE, pin 16 = GND.

### 8.2 HDMI Type-A Receptacle (J21)

| Pin | Signal    | Pin | Signal    |
|-----|-----------|-----|-----------|
| 1   | TMDS D2+  | 2   | GND       |
| 3   | TMDS D2-  | 4   | TMDS D1+  |
| 5   | GND       | 6   | TMDS D1-  |
| 7   | TMDS D0+  | 8   | GND       |
| 9   | TMDS D0-  | 10  | TMDS CLK+ |
| 11  | GND       | 12  | TMDS CLK- |
| 13  | CEC       | 14  | NC        |
| 15  | DDC SCL   | 16  | DDC SDA   |
| 17  | GND       | 18  | +5V       |
| 19  | HPD       |     |           |

### 8.3 RJ45 with Integrated Magnetics (J1)

| Pin | Signal     |
|-----|------------|
| 1   | MDI0+ (TX+)|
| 2   | MDI0- (TX-)|
| 3   | MDI1+ (RX+)|
| 4   | MDI2+ (CT) |
| 5   | MDI2- (CT) |
| 6   | MDI1- (RX-)|
| 7   | MDI3+ (CT) |
| 8   | MDI3- (CT) |
| J1  | Shield GND |

---

*Document Version: 1.0 | Author: Hardware Engineer | Date: 2026-06-16*
*Target Device: Nebula Matrix — Professional LED Matrix Display Engine*
