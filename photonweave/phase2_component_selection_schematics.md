# PhotonWeave — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

### Primary Integrated Circuits

| Designator | Part Number | Manufacturer | Description | Package | Qty | Unit Cost |
|------------|-------------|--------------|-------------|---------|-----|-----------|
| U1 | XC7K325T-2FFG900C | Xilinx (AMD) | Kintex-7 FPGA, 326K logic cells, 840 DSP, 16 GTX | FFG900 (BGA, 31×31mm, 1.0mm pitch) | 1 | $1,200 |
| U2 | STM32H743VIT6 | STMicro | ARM Cortex-M7, 480 MHz, 2MB Flash, 1MB RAM | LQFP-100 (14×14mm) | 1 | $15 |
| U3 | Si5345A-B-GM | Silicon Labs | Any-Frequency Clock Generator, 10 outputs | QFN-44 (7×7mm) | 1 | $12 |
| U4 | TPS65218D0RSLR | Texas Instruments | PMIC, 6 buck converters, 1 LDO, power sequencing | VQFN-48 (6×6mm) | 1 | $6 |
| U5 | FT601Q-B-T | FTDI | USB 3.0 to 32-bit FIFO bridge | QFN-76 (9×9mm) | 1 | $8 |
| U6-U9 | MT40A512M16JY-075E | Micron | DDR4 SDRAM, 8Gb (512M×16), 2400 Mbps | FBGA-96 (9×14mm) | 4 | $12 |
| U10 | MT25QU512ABB8ESF-0SIT | Micron | 512Mb QSPI NOR Flash, 133 MHz | SOIC-16W (10.3×7.5mm) | 1 | $4 |
| U11 | M24C02-RMN6TP | STMicro | 2Kb I2C EEPROM | SOIC-8 | 1 | $0.30 |
| U12-U15 | TMP117AIDRVR | Texas Instruments | ±0.1°C I2C Temperature Sensor | WSON-6 (2×2mm) | 4 | $3 |
| U16 | SiT5356AI-FQ-25E0-25.000000 | SiTime | TCXO, 25 MHz, ±0.1 ppm, LVCMOS | SMD 5032 (5×3.2mm) | 1 | $15 |
| U17 | SN74LVC1G125DCKR | Texas Instruments | Single Bus Buffer Gate with 3-State Output | SC-70-5 | 1 | $0.15 |
| U18 | TPD4E05U06DQAR | Texas Instruments | 4-Channel ESD Protection for HDMI | USON-10 | 1 | $0.50 |
| U19 | TPD4E02B04DQAR | Texas Instruments | 4-Channel ESD for USB 3.0 | USON-10 | 1 | $0.45 |

### Connectors

| Designator | Part Number | Description | Qty |
|------------|-------------|-------------|-----|
| J1 | PCIe x8 Edge Connector | Gold-plated, 98-position card edge | 1 |
| J2 | 0473910001 (Molex) | HDMI Type-A Receptacle, 19-position | 1 |
| J3 | 2007557-1 (TE) | QSFP+ Cage with Integrated Connector | 1 |
| J4 | 10118194-0001LF (Amphenol) | USB 3.0 Micro-B Receptacle | 1 |
| J5 | 3220-10-0300-00 (CNC Tech) | 10-pin 0.1" Header (JTAG/SWD) | 1 |
| J6 | TSW-104-07-G-S (Samtec) | 4-pin Header (I2C breakout) | 1 |
| J7 | 22-23-2021 (Molex) | 2-pin KK Header (Fan) | 1 |

### Passives Summary

| Type | Value | Package | Qty |
|------|-------|---------|-----|
| MLCC Capacitor | 0.1 µF, 25V, X7R | 0402 | 120 |
| MLCC Capacitor | 1.0 µF, 25V, X7R | 0402 | 45 |
| MLCC Capacitor | 10 µF, 25V, X7R | 0805 | 30 |
| MLCC Capacitor | 22 µF, 16V, X7R | 0805 | 16 |
| MLCC Capacitor | 100 µF, 6.3V, X5R | 1206 | 12 |
| Tantalum Polymer | 330 µF, 6.3V | 7343 (D-case) | 8 |
| Resistor | 0 Ω (jumper) | 0402 | 10 |
| Resistor | 10 Ω ±1% | 0402 | 4 |
| Resistor | 33 Ω ±1% | 0402 | 8 |
| Resistor | 49.9 Ω ±1% | 0402 | 16 |
| Resistor | 100 Ω ±1% | 0402 | 12 |
| Resistor | 240 Ω ±1% | 0402 | 8 |
| Resistor | 1 kΩ ±1% | 0402 | 20 |
| Resistor | 2.2 kΩ ±1% | 0402 | 8 |
| Resistor | 4.7 kΩ ±1% | 0402 | 16 |
| Resistor | 10 kΩ ±1% | 0402 | 12 |
| Resistor | 100 kΩ ±1% | 0402 | 4 |
| Ferrite Bead | 120 Ω @ 100 MHz, 3A | 0805 | 12 |
| Ferrite Bead | 600 Ω @ 100 MHz, 1.5A | 0603 | 8 |
| Inductor | 1.0 µH, 15A, shielded | 10×10mm SMD | 6 |
| Inductor | 2.2 µH, 8A, shielded | 8×8mm SMD | 4 |
| Inductor | 4.7 µH, 3A, shielded | 6×6mm SMD | 2 |

## 2. FPGA Pinout — XC7K325T-2FFG900C

### Bank Assignment

| Bank | VCCO | Type | Primary Function |
|------|------|------|------------------|
| 12 | 1.2V | HR | DDR4 Interface 0 (DQ[15:0], DQS, DM, ADDR/CMD) |
| 13 | 1.2V | HR | DDR4 Interface 1 (DQ[15:0], DQS, DM, ADDR/CMD) |
| 14 | 1.2V | HR | DDR4 Interface 2 (DQ[15:0], DQS, DM, ADDR/CMD) |
| 15 | 1.2V | HR | DDR4 Interface 3 (DQ[15:0], DQS, DM, ADDR/CMD) |
| 16 | 3.3V | HR | FT601 FIFO (D[31:0], CLK, TXE, RXF, WR, RD, OE) |
| 17 | 3.3V | HR | QSPI Flash, I2C, GPIO, LED |
| 18 | 3.3V | HR | STM32 SPI, UART, Control Signals |
| 32 | 1.8V | HR | HDMI Aux (DDC, HPD, CEC) |
| 33 | 1.8V | HR | System Control, Reset, JTAG |
| 34 | 1.8V | HR | Si5345 I2C, Clock Inputs |
| 115 | 1.0V | GTX | PCIe Gen3 x8 (MGTXTX/MGTXRX pairs 0-7) |
| 116 | 1.0V | GTX | HDMI TMDS Output (MGTXTX pairs 8-11) |
| 117 | 1.0V | GTX | QSFP+ (MGTXTX/MGTXRX pairs 12-15) |

### Critical Pin Assignments

#### DDR4 Interface 0 (Bank 12, U6)

| FPGA Pin | Signal | DDR4 Pin | Notes |
|----------|--------|----------|-------|
| L20 | DQ0 | G2 | Byte lane 0 |
| M20 | DQ1 | H7 | |
| M19 | DQ2 | G3 | |
| N20 | DQ3 | F7 | |
| L19 | DQ4 | H2 | |
| K19 | DQ5 | H8 | |
| N19 | DQ6 | G1 | |
| M18 | DQ7 | F8 | |
| K18 | DQS0_P | E7 | Differential strobe |
| L18 | DQS0_N | D7 | |
| J19 | DM0 | D3 | Data mask |
| P20 | DQ8 | C2 | Byte lane 1 |
| R20 | DQ9 | B7 | |
| P19 | DQ10 | C3 | |
| T20 | DQ11 | A7 | |
| R19 | DQ12 | C8 | |
| N18 | DQ13 | A2 | |
| T19 | DQ14 | B8 | |
| P18 | DQ15 | A3 | |
| R18 | DQS1_P | B3 | |
| T18 | DQS1_N | A3 | |
| U19 | DM1 | D1 | |

#### DDR4 Address/Command (Bank 12, shared across interfaces)

| FPGA Pin | Signal | DDR4 Pin | Notes |
|----------|--------|----------|-------|
| U20 | A0 | N2 | Row/Col address |
| V20 | A1 | N7 | |
| V19 | A2 | N3 | |
| W20 | A3 | M2 | |
| W19 | A4 | M7 | |
| Y20 | A5 | M3 | |
| Y19 | A6 | N1 | |
| AA20 | A7 | M8 | |
| AA19 | A8 | P2 | |
| AB20 | A9 | P7 | |
| AB19 | A10/AP | P3 | Auto-precharge |
| AC20 | A11 | P8 | |
| AC19 | A12/BC | R2 | Burst chop |
| AD20 | A13 | R7 | |
| AD19 | A14 | R3 | |
| AE20 | A15 | R8 | |
| AF20 | A16 | T8 | |
| AF19 | A17 | N8 | |
| V18 | BA0 | L2 | Bank address |
| W18 | BA1 | L7 | |
| Y18 | BG0 | K2 | Bank group |
| AA18 | BG1 | K7 | |
| U18 | CK0_P | J7 | Differential clock |
| V17 | CK0_N | K7 | |
| AB18 | CKE0 | K9 | Clock enable |
| AC18 | CS0_N | L3 | Chip select |
| AD18 | ODT0 | K1 | On-die termination |
| AE18 | RAS_N | J3 | Row address strobe |
| AF18 | CAS_N | L8 | Column address strobe |
| AG18 | WE_N | J1 | Write enable |
| AG19 | ACT_N | L1 | Activate |
| AH18 | RESET_N | G9 | DDR4 reset |
| AG20 | ALERT_N | F1 | Parity alert |
| AH20 | PAR | G8 | Command parity |

#### PCIe Interface (Bank 115, GTX Quad 0-1)

| FPGA Pin | Signal | PCIe Edge Pin | Notes |
|----------|--------|---------------|-------|
| MGTXTX0_P | PETp0 | A16 (PERST# side) | Lane 0 TX |
| MGTXTX0_N | PETn0 | A17 | |
| MGTXRX0_P | PERp0 | B14 | Lane 0 RX |
| MGTXRX0_N | PERn0 | B15 | |
| MGTXTX1_P | PETp1 | A19 | Lane 1 TX |
| MGTXTX1_N | PETn1 | A20 | |
| MGTXRX1_P | PERp1 | B17 | Lane 1 RX |
| MGTXRX1_N | PERn1 | B18 | |
| MGTXTX2_P | PETp2 | A22 | Lane 2 TX |
| MGTXTX2_N | PETn2 | A23 | |
| MGTXRX2_P | PERp2 | B20 | Lane 2 RX |
| MGTXRX2_N | PERn2 | B21 | |
| MGTXTX3_P | PETp3 | A25 | Lane 3 TX |
| MGTXTX3_N | PETn3 | A26 | |
| MGTXRX3_P | PERp3 | B23 | Lane 3 RX |
| MGTXRX3_N | PERn3 | B24 | |
| MGTXTX4_P | PETp4 | A28 | Lane 4 TX |
| MGTXTX4_N | PETn4 | A29 | |
| MGTXRX4_P | PERp4 | B26 | Lane 4 RX |
| MGTXRX4_N | PERn4 | B27 | |
| MGTXTX5_P | PETp5 | A31 | Lane 5 TX |
| MGTXTX5_N | PETn5 | A32 | |
| MGTXRX5_P | PERp5 | B29 | Lane 5 RX |
| MGTXRX5_N | PERn5 | B30 | |
| MGTXTX6_P | PETp6 | A34 | Lane 6 TX |
| MGTXTX6_N | PETn6 | A35 | |
| MGTXRX6_P | PERp6 | B32 | Lane 6 RX |
| MGTXRX6_N | PERn6 | B33 | |
| MGTXTX7_P | PETp7 | A37 | Lane 7 TX |
| MGTXTX7_N | PETn7 | A38 | |
| MGTXRX7_P | PERp7 | B35 | Lane 7 RX |
| MGTXRX7_N | PERn7 | B36 | |
| IO_L13P_T2_MRCC_34 | PERST# | A11 | 100 MHz refclk capable |
| IO_L13N_T2_MRCC_34 | REFCLK_P | A12 | PCIe refclk from slot |
| IO_L14P_T2_SRCC_34 | REFCLK_N | A13 | |
| IO_L14N_T2_SRCC_34 | WAKE# | B11 | Open-drain, 10k pull-up |
| IO_25_34 | PRSNT1# | A1 | GND on card |
| IO_25_33 | PRSNT2# | B17 | Open (x8 detect) |
| IO_0_34 | CLKREQ# | B12 | Open-drain, 10k pull-up |

#### HDMI TMDS Output (Bank 116, GTX Quad 2-3)

| FPGA Pin | Signal | HDMI Pin | Notes |
|----------|--------|----------|-------|
| MGTXTX8_P | TMDS_D2+ | J2-1 | Red channel |
| MGTXTX8_N | TMDS_D2- | J2-2 | |
| MGTXTX9_P | TMDS_D1+ | J2-4 | Green channel |
| MGTXTX9_N | TMDS_D1- | J2-5 | |
| MGTXTX10_P | TMDS_D0+ | J2-7 | Blue channel |
| MGTXTX10_N | TMDS_D0- | J2-8 | |
| MGTXTX11_P | TMDS_CLK+ | J2-10 | Pixel clock |
| MGTXTX11_N | TMDS_CLK- | J2-11 | |
| IO_L19P_T3_34 | HDMI_DDC_SCL | J2-15 | I2C for EDID |
| IO_L19N_T3_34 | HDMI_DDC_SDA | J2-16 | |
| IO_L20P_T3_34 | HDMI_HPD | J2-19 | Hot plug detect (5V tolerant via divider) |
| IO_L20N_T3_34 | HDMI_CEC | J2-13 | Consumer Electronics Control |

#### FT601 USB 3.0 FIFO (Bank 16)

| FPGA Pin | Signal | FT601 Pin | Notes |
|----------|--------|-----------|-------|
| IO_L1P_T0_16 | DATA0 | 52 | 32-bit data bus |
| IO_L1N_T0_16 | DATA1 | 53 | |
| IO_L2P_T0_16 | DATA2 | 54 | |
| IO_L2N_T0_16 | DATA3 | 55 | |
| IO_L3P_T0_16 | DATA4 | 56 | |
| IO_L3N_T0_16 | DATA5 | 57 | |
| IO_L4P_T0_16 | DATA6 | 58 | |
| IO_L4N_T0_16 | DATA7 | 59 | |
| IO_L5P_T0_16 | DATA8 | 60 | |
| IO_L5N_T0_16 | DATA9 | 61 | |
| IO_L6P_T0_16 | DATA10 | 62 | |
| IO_L6N_T0_16 | DATA11 | 63 | |
| IO_L7P_T0_16 | DATA12 | 64 | |
| IO_L7N_T0_16 | DATA13 | 65 | |
| IO_L8P_T0_16 | DATA14 | 66 | |
| IO_L8N_T0_16 | DATA15 | 67 | |
| IO_L9P_T0_16 | DATA16 | 68 | |
| IO_L9N_T0_16 | DATA17 | 69 | |
| IO_L10P_T0_16 | DATA18 | 70 | |
| IO_L10N_T0_16 | DATA19 | 71 | |
| IO_L11P_T0_16 | DATA20 | 72 | |
| IO_L11N_T0_16 | DATA21 | 73 | |
| IO_L12P_T0_16 | DATA22 | 74 | |
| IO_L12N_T0_16 | DATA23 | 75 | |
| IO_L13P_T0_16 | DATA24 | 76 | |
| IO_L13N_T0_16 | DATA25 | 77 | |
| IO_L14P_T0_16 | DATA26 | 78 | |
| IO_L14N_T0_16 | DATA27 | 79 | |
| IO_L15P_T0_16 | DATA28 | 80 | |
| IO_L15N_T0_16 | DATA29 | 81 | |
| IO_L16P_T0_16 | DATA30 | 82 | |
| IO_L16N_T0_16 | DATA31 | 83 | |
| IO_L17P_T0_16 | CLK | 84 | 60 MHz from FT601 |
| IO_L18P_T0_16 | TXE_N | 85 | Transmit enable (active low) |
| IO_L18N_T0_16 | RXF_N | 86 | Receive flag (active low) |
| IO_L19P_T0_16 | WR_N | 87 | Write strobe (active low) |
| IO_L19N_T0_16 | RD_N | 88 | Read strobe (active low) |
| IO_L20P_T0_16 | OE_N | 89 | Output enable (active low) |
| IO_L20N_T0_16 | SIWU_N | 90 | Send immediate / wake up |
| IO_L21P_T0_16 | RESET_N | 91 | FT601 reset |
| IO_L21N_T0_16 | BE0 | 92 | Byte enable 0 |
| IO_L22P_T0_16 | BE1 | 93 | Byte enable 1 |
| IO_L22N_T0_16 | BE2 | 94 | Byte enable 2 |
| IO_L23P_T0_16 | BE3 | 95 | Byte enable 3 |

#### STM32H743 Interface (Bank 18)

| FPGA Pin | Signal | STM32 Pin | Notes |
|----------|--------|-----------|-------|
| IO_L1P_T0_18 | STM32_SPI1_SCK | PA5 (pin 14) | 50 MHz SPI |
| IO_L1N_T0_18 | STM32_SPI1_MISO | PA6 (pin 15) | |
| IO_L2P_T0_18 | STM32_SPI1_MOSI | PA7 (pin 16) | |
| IO_L2N_T0_18 | STM32_SPI1_NSS | PA4 (pin 13) | |
| IO_L3P_T0_18 | STM32_UART4_TX | PD1 (pin 58) | Debug UART |
| IO_L3N_T0_18 | STM32_UART4_RX | PD0 (pin 57) | |
| IO_L4P_T0_18 | STM32_I2C1_SCL | PB6 (pin 36) | 100 kHz I2C |
| IO_L4N_T0_18 | STM32_I2C1_SDA | PB7 (pin 37) | |
| IO_L5P_T0_18 | FPGA_INT_N | PE0 (pin 60) | FPGA→STM32 interrupt |
| IO_L5N_T0_18 | STM32_BOOT0 | BOOT0 (pin 94) | Boot mode control |
| IO_L6P_T0_18 | FPGA_PROG_B | PE1 (pin 61) | FPGA config initiate |
| IO_L6N_T0_18 | FPGA_INIT_B | PE2 (pin 62) | FPGA config status |
| IO_L7P_T0_18 | FPGA_DONE | PE3 (pin 63) | FPGA config complete |
| IO_L7N_T0_18 | STM32_RST_N | NRST (pin 14) | Bidirectional reset |

#### Si5345 Clock Generator (Bank 34, I2C)

| FPGA Pin | Signal | Si5345 Pin | Notes |
|----------|--------|------------|-------|
| IO_L21P_T3_34 | SI5345_SCL | 18 (I2C_SCL) | 3.3V I2C |
| IO_L21N_T3_34 | SI5345_SDA | 19 (I2C_SDA) | |
| IO_L22P_T3_34 | SI5345_INTR | 20 (INTR) | Interrupt from Si5345 |
| IO_L22N_T3_34 | SI5345_LOL | 21 (LOL) | Loss of lock |
| IO_L23P_T3_34 | SI5345_RST | 22 (RST) | Reset |
| IO_L23N_T3_34 | SI5345_OE | 23 (OE) | Output enable |

#### QSPI Flash (Bank 17)

| FPGA Pin | Signal | MT25QU512 Pin | Notes |
|----------|--------|---------------|-------|
| IO_L1P_T0_17 | QSPI_D0 | 5 (DQ0/SIO0) | Quad I/O data 0 |
| IO_L1N_T0_17 | QSPI_D1 | 2 (DQ1/SIO1) | Quad I/O data 1 |
| IO_L2P_T0_17 | QSPI_D2 | 3 (DQ2/SIO2) | Quad I/O data 2 |
| IO_L2N_T0_17 | QSPI_D3 | 7 (DQ3/SIO3) | Quad I/O data 3 |
| IO_L3P_T0_17 | QSPI_CLK | 6 (CLK) | 50 MHz max |
| IO_L3N_T0_17 | QSPI_CS_N | 1 (CS#) | Chip select |
| IO_L4P_T0_17 | QSPI_RESET_N | 4 (RESET#) | Hardware reset |
| IO_L4N_T0_17 | QSPI_WP_N | 8 (W#/DQ2 in quad) | Write protect |

## 3. Power Distribution Network

### TPS65218D0 PMIC Rail Assignment

| Rail | Voltage | Max Current | FPGA Rail | Notes |
|------|---------|-------------|-----------|-------|
| DCDC1 | 1.0V | 25A | VCCINT, VCCBRAM | FPGA core logic |
| DCDC2 | 1.8V | 3A | VCCAUX, VCCAUX_IO | FPGA auxiliary |
| DCDC3 | 1.2V | 12A | VDD_DDR (×4) | DDR4 supply |
| DCDC4 | 3.3V | 5A | VCCO_16, VCCO_17, VCCO_18 | I/O banks |
| DCDC5 | 1.0V | 8A | MGTAVCC | GTX transceiver analog |
| DCDC6 | 1.2V | 4A | MGTAVTT | GTX termination |
| LDO1 | 3.3V | 0.5A | STM32, Si5345, TMP117, EEPROM | System peripherals |
| VTT_DDR | 0.6V | 6A | DDR4 VTT termination | External LDO (TPS51200) |

### Decoupling Network per FPGA Rail

#### VCCINT (1.0V) — Core Logic
- **Bulk**: 4× 330 µF tantalum polymer (D-case), placed at PMIC output
- **Mid-frequency**: 8× 100 µF X5R 1206, placed around FPGA perimeter
- **High-frequency**: 40× 0.1 µF X7R 0402, one per VCCINT ball pair
- **Ultra-high**: 16× 1.0 µF X7R 0402 (reverse-geometry, low ESL), placed on bottom side directly under FPGA

#### MGTAVCC (1.0V) — GTX Analog
- **Bulk**: 2× 330 µF tantalum polymer
- **Mid-frequency**: 4× 100 µF X5R 1206
- **High-frequency**: 16× 0.1 µF X7R 0402 (one per GTX quad)
- **Ferrite bead**: 120 Ω @ 100 MHz between MGTAVCC plane and VCCINT plane

#### MGTAVTT (1.2V) — GTX Termination
- **Bulk**: 1× 330 µF tantalum polymer
- **Mid-frequency**: 2× 100 µF X5R 1206
- **High-frequency**: 8× 0.1 µF X7R 0402
- **Precision**: ±30 mV ripple tolerance — requires dedicated LDO post-regulation

#### VCCAUX (1.8V) — Auxiliary
- **Bulk**: 1× 100 µF X5R 1206
- **High-frequency**: 12× 0.1 µF X7R 0402

#### VCCO_3V3 (3.3V) — I/O Banks
- **Bulk**: 2× 100 µF X5R 1206
- **High-frequency**: 20× 0.1 µF X7R 0402

#### VDD_DDR (1.2V) — DDR4 Supply
- **Bulk**: 4× 330 µF tantalum polymer (one per DDR4 chip)
- **Mid-frequency**: 8× 100 µF X5R 1206
- **High-frequency**: 16× 0.1 µF X7R 0402 (4 per DDR4 chip)
- **VTT**: 0.6V via TPS51200 sink/source LDO, 4× 10 µF + 8× 0.1 µF

### Power Sequencing (TPS65218 Enforced)

```
T=0:    3.3V_STM32 (LDO1) — always on when 12V present
T=1ms:  VCCAUX (1.8V) — DCDC2
T=2ms:  VCCINT (1.0V) — DCDC1
T=3ms:  VCCBRAM (1.0V) — same rail as VCCINT
T=4ms:  MGTAVCC (1.0V) — DCDC5
T=5ms:  MGTAVTT (1.2V) — DCDC6
T=6ms:  VCCO_3V3 (3.3V) — DCDC4
T=7ms:  VDD_DDR (1.2V) — DCDC3
T=8ms:  VTT_DDR (0.6V) — TPS51200 enable
T=10ms: PGOOD asserted → FPGA PROG_B released
```

## 4. Impedance-Controlled Nets

| Net Class | Target Z0 | Tolerance | Trace Type | Notes |
|-----------|-----------|-----------|------------|-------|
| PCIe TX/RX | 85 Ω differential | ±10% | Edge-coupled microstrip | 100 Ω common-mode |
| HDMI TMDS | 100 Ω differential | ±5% | Edge-coupled microstrip | Tight tolerance for eye diagram |
| DDR4 DQ/DQS | 40 Ω single-ended | ±10% | Microstrip | ODT to 40 Ω at DRAM |
| DDR4 CLK | 100 Ω differential | ±10% | Edge-coupled microstrip | |
| DDR4 ADDR/CMD | 40 Ω single-ended | ±10% | Microstrip | Fly-by topology |
| USB 3.0 SS | 90 Ω differential | ±7% | Edge-coupled microstrip | |
| QSFP+ TX/RX | 100 Ω differential | ±10% | Edge-coupled microstrip | |
| 50 Ω General | 50 Ω single-ended | ±10% | Microstrip | SPI, I2C, UART, GPIO |

## 5. Pull-Up / Pull-Down Resistor Table

| Signal | Resistor | Value | Notes |
|--------|----------|-------|-------|
| FPGA_PROG_B | Pull-up to 3.3V | 4.7 kΩ | FPGA config initiate |
| FPGA_INIT_B | Pull-up to 3.3V | 4.7 kΩ | Config status |
| FPGA_DONE | Pull-up to 3.3V | 4.7 kΩ | Config complete |
| FPGA_M2, M1, M0 | Pull-down to GND | 1 kΩ | SPI master config mode (M[2:0]=001) |
| PCIE_PERST# | Pull-up to 3.3V | 10 kΩ | De-asserted when power stable |
| PCIE_WAKE# | Pull-up to 3.3V | 10 kΩ | Open-drain from host |
| PCIE_CLKREQ# | Pull-up to 3.3V | 10 kΩ | Open-drain |
| HDMI_HPD | Pull-down to GND | 100 kΩ | Default low until sink detected |
| HDMI_CEC | Pull-up to 3.3V | 27 kΩ | CEC bus requirement |
| HDMI_DDC_SCL/SDA | Pull-up to 5V | 2.2 kΩ | DDC I2C (5V tolerant) |
| STM32_NRST | Pull-up to 3.3V | 10 kΩ | System reset |
| STM32_BOOT0 | Pull-down to GND | 10 kΩ | Boot from Flash |
| I2C_SCL/SDA (all buses) | Pull-up to 3.3V | 2.2 kΩ | Standard I2C |
| FT601_RESET_N | Pull-up to 3.3V | 10 kΩ | Active low reset |
| QSPI_CS_N | Pull-up to 3.3V | 10 kΩ | Deselect flash when idle |
| JTAG_TMS | Pull-up to 3.3V | 4.7 kΩ | JTAG state machine |
| JTAG_TDI | Pull-up to 3.3V | 4.7 kΩ | |
| JTAG_TCK | Pull-down to GND | 4.7 kΩ | |
| Si5345_RST | Pull-up to 3.3V | 10 kΩ | |
| Si5345_OE | Pull-up to 3.3V | 10 kΩ | Outputs enabled by default |

## 6. Netlist — Key Signal Paths

### PCIe Data Path
```
Host PCIe Slot (J1)
  PETp[0:7]/PETn[0:7] → AC coupling caps (220 nF, 0402) → FPGA MGTXTX[0:7]_P/N (Bank 115)
  PERp[0:7]/PERn[0:7] ← AC coupling caps (220 nF, 0402) ← FPGA MGTXRX[0:7]_P/N (Bank 115)
  REFCLK_P/N → AC coupling caps (100 nF, 0402) → FPGA IO_L13/L14 (Bank 34, MRCC)
  PERST# → FPGA IO_25_34
  CLKREQ# → FPGA IO_0_34
  WAKE# → FPGA IO_25_33
  PRSNT1# → GND (x1 present)
  PRSNT2# → NC (x8 present)
```

### DDR4 Data Path (per interface, ×4)
```
FPGA Bank 12/13/14/15 → 22 Ω series terminator → DDR4 SDRAM (U6/U7/U8/U9)
  DQ[0:15] → 22 Ω → DDR4 DQ[0:15]
  DQS_P/N → 22 Ω → DDR4 DQS_P/N
  DM → 22 Ω → DDR4 DM
  ADDR/CMD[0:17] → 22 Ω → DDR4 A[0:17] (fly-by, daisy-chained across 4 chips)
  CK_P/N → 22 Ω → DDR4 CK_P/N (fly-by)
  CKE, CS, ODT, RESET → 22 Ω → DDR4 control (point-to-point per rank)
```

### HDMI Output Path
```
FPGA Bank 116 (GTX) → AC coupling caps (100 nF, 0201) → HDMI Connector (J2)
  MGTXTX8_P/N → 100 nF → TMDS_D2+/-
  MGTXTX9_P/N → 100 nF → TMDS_D1+/-
  MGTXTX10_P/N → 100 nF → TMDS_D0+/-
  MGTXTX11_P/N → 100 nF → TMDS_CLK+/-
  FPGA IO_L19 → Level shifter (TXS0102) → HDMI_DDC_SCL/SDA (5V)
  FPGA IO_L20 → 10k/10k divider → HDMI_HPD (5V tolerant)
  FPGA IO_L20N → 27k pull-up → HDMI_CEC
  HDMI +5V → 100 mA current limit → J2-18 (HDMI_5V)
  TPD4E05U06 ESD protection on all TMDS pairs at connector
```

### USB 3.0 Data Path
```
USB 3.0 Micro-B (J4) → FT601Q (U5) → FPGA Bank 16
  USB_SSRX_P/N → FT601 SSRX_P/N (pins 5/6)
  USB_SSTX_P/N → FT601 SSTX_P/N (pins 8/9)
  USB_DP/DM → FT601 DP/DM (pins 11/12)
  FT601 DATA[31:0] → FPGA Bank 16 (32-bit parallel)
  FT601 CLK (60 MHz) → FPGA IO_L17P_16
  FT601 TXE/RXF/WR/RD/OE → FPGA Bank 16 control
  TPD4E02B04 ESD protection on SS pairs at connector
```

### Clock Distribution
```
SiT5356 25 MHz TCXO (U16) → Si5345 XA/XB (pins 1/2) — reference input
Si5345 OUT0 (200 MHz) → FPGA IO_L12P_MRCC_34 (GCLK0) — FFT core
Si5345 OUT1 (125 MHz) → FPGA IO_L13P_MRCC_34 (GCLK1) — PCIe user clk
Si5345 OUT2 (300 MHz) → FPGA IO_L14P_MRCC_34 (GCLK2) — DDR4 ref
Si5345 OUT3 (148.5 MHz) → FPGA IO_L15P_MRCC_34 (GCLK3) — HDMI pixel
Si5345 OUT4 (100 MHz) → FPGA IO_L16P_MRCC_34 (GCLK4) — AXI interconnect
Si5345 OUT5 (25 MHz) → STM32H743 OSC_IN (PH0, pin 8) — HSE bypass
Si5345 OUT6 (50 MHz) → FPGA IO_L17P_MRCC_34 (EMCCLK) — Config clock
Si5345 OUT7 (60 MHz) → FPGA IO_L18P_MRCC_34 → FPGA PLL → FT601 CLK
Si5345 OUT8 (156.25 MHz) → FPGA IO_L19P_MRCC_34 (GCLK5) — QSFP+ ref
Si5345 OUT9 (26 MHz) → FPGA IO_L20P_MRCC_34 — HDMI TMDS ref
```

### I2C Bus Topology
```
STM32H743 I2C1 (PB6/PB7) — Master
  ├── 0x24: TPS65218 PMIC
  ├── 0x30: FPGA (I2C slave emulation in fabric)
  ├── 0x48: TMP117 #1 (FPGA die edge, top-left)
  ├── 0x49: TMP117 #2 (FPGA die edge, bottom-right)
  ├── 0x4A: TMP117 #3 (DDR4 region)
  ├── 0x4B: TMP117 #4 (PMIC region)
  ├── 0x50: M24C02 EEPROM (board info, serial, MAC)
  └── 0x68: Si5345 Clock Generator
All with 2.2 kΩ pull-ups to 3.3V_STM32
```

## 7. QSFP+ Direct Camera Ingest

```
QSFP+ Cage (J3) → FPGA Bank 117 (GTX Quad 4)
  MGTXRX12_P/N ← QSFP+ RX1 (Lane 0)
  MGTXRX13_P/N ← QSFP+ RX2 (Lane 1)
  MGTXRX14_P/N ← QSFP+ RX3 (Lane 2)
  MGTXRX15_P/N ← QSFP+ RX4 (Lane 3)
  MGTXTX12_P/N → QSFP+ TX1 (Lane 0) — control/telemetry backchannel
  MGTXTX13_P/N → QSFP+ TX2 (Lane 1)
  MGTXTX14_P/N → QSFP+ TX3 (Lane 2)
  MGTXTX15_P/N → QSFP+ TX4 (Lane 3)
  FPGA IO_L1P_17 → QSFP+ SCL (I2C management)
  FPGA IO_L1N_17 → QSFP+ SDA
  FPGA IO_L2P_17 → QSFP+ MODSEL (module select)
  FPGA IO_L2N_17 → QSFP+ RESETL (module reset)
  FPGA IO_L3P_17 → QSFP+ LPMODE (low power mode)
  FPGA IO_L4P_17 → QSFP+ MODPRS (module present)
  FPGA IO_L4N_17 → QSFP+ INTL (interrupt)
```

## 8. Configuration & Boot Flash

```
FPGA Bank 17 → MT25QU512 (U10)
  QSPI_D[0:3] → DQ[0:3] (Quad I/O mode)
  QSPI_CLK → CLK (50 MHz max from FPGA EMCCLK)
  QSPI_CS_N → CS#
  FPGA M[2:0] = 001 (SPI x4 master mode)
  FPGA CFGBVS = GND (3.3V config bank voltage)
  FPGA PROGRAM_B → STM32 PE1 (controlled programming)
  FPGA INIT_B → STM32 PE2 (status monitoring)
  FPGA DONE → STM32 PE3 (config complete)
```

## 9. JTAG / Debug Interface

```
J5 (10-pin 0.1" header, ARM Cortex Debug Connector)
  Pin 1: VTREF → 3.3V_STM32
  Pin 2: SWDIO → STM32 PA13 (pin 72)
  Pin 3: GND
  Pin 4: SWCLK → STM32 PA14 (pin 76)
  Pin 5: GND
  Pin 6: SWO → STM32 PB3 (pin 35)
  Pin 7: KEY (notch)
  Pin 8: NC
  Pin 9: GNDDetect → GND
  Pin 10: nRESET → STM32 NRST (pin 14)

FPGA JTAG (separate header, J8, 6-pin)
  Pin 1: TDI → FPGA TDI_34
  Pin 2: TDO ← FPGA TDO_34
  Pin 3: TMS → FPGA TMS_34
  Pin 4: TCK → FPGA TCK_34
  Pin 5: GND
  Pin 6: 3.3V
```

## 10. LED Indicators

| LED | Color | FPGA Pin | Function |
|-----|-------|----------|----------|
| D1 | Green | IO_L5P_17 | FPGA DONE / System Ready |
| D2 | Blue | IO_L5N_17 | PCIe Link Up (L0 state) |
| D3 | Yellow | IO_L6P_17 | Activity / Frame Processing |
| D4 | Red | IO_L6N_17 | Error / Thermal Warning |
| D5 | Green | STM32 PB0 (pin 26) | STM32 Heartbeat |
| D6 | Blue | STM32 PB1 (pin 27) | USB Enumeration Done |

All LEDs: 220 Ω series resistor to GND, driven by 3.3V LVCMOS.

## 11. Board Identification EEPROM

M24C02 (U11) at I2C address 0x50:
```
Offset 0x00-0x03: Magic number 0x50574E56 ("PWNV")
Offset 0x04-0x05: Board revision (BCD, e.g., 0x0100 = Rev 1.0)
Offset 0x06-0x0B: MAC address (6 bytes, for USB Ethernet gadget)
Offset 0x0C-0x0F: Serial number (32-bit)
Offset 0x10-0x13: Manufacturing date (Unix timestamp)
Offset 0x14-0x1F: Reserved
Offset 0x20-0x3F: Product name "PhotonWeave CGH Engine\0" (padded)
Offset 0x40-0x5F: Manufacturer "jayis1\0" (padded)
Offset 0x60-0xFF: Reserved / user data
```

## 12. Thermal Sensor Placement

| Sensor | I2C Addr | Location | Monitors |
|--------|----------|----------|----------|
| TMP117 #1 | 0x48 | Top-left of FPGA, 5mm from die edge | FPGA core temp zone 1 |
| TMP117 #2 | 0x49 | Bottom-right of FPGA, 5mm from die edge | FPGA core temp zone 2 |
| TMP117 #3 | 0x4A | Between DDR4 U7 and U8 | DDR4 ambient |
| TMP117 #4 | 0x4B | Adjacent to TPS65218 | PMIC temperature |

All TMP117s: 0.1 µF decoupling at VCC, 2.2 kΩ pull-ups shared on I2C bus.
Alert pins wired-OR to STM32 PE4 (pin 64) for thermal interrupt.
