# RideCore — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Manufacturer | Description | Qty | Unit Cost (est.) |
|---|---|---|---|---|---|
| U1 | STM32G474CEU6 | STMicroelectronics | Cortex-M4 @ 170 MHz, 512K Flash, 128K RAM, UFQFPN-48 | 1 | $5.20 |
| U2 | TPS6521801RSL | Texas Instruments | PMIC: 3× DCDC + 4× LDO, WQFN-32 | 1 | $2.80 |
| U3 | MCP2518FD-I/ST | Microchip | CAN FD controller, SPI, TSSOP-14 | 1 | $1.90 |
| U4 | TJA1463AT/1J | NXP | CAN FD transceiver, 5 Mbps, SOIC-8 | 1 | $0.95 |
| U5 | nRF52832-CIAA | Nordic | BLE 5.0 SoC, QFN-48 | 1 | $3.10 |
| U6 | W25Q128JVSIM | Winbond | 16 Mb SPI flash, SOIC-8 | 1 | $0.55 |
| U7–U9 | IRS2186STRPBF | Infineon | Half-bridge gate driver, 600V, SOIC-8 | 3 | $1.20 ea |
| U10–U12 | ADuM3223CRZ | Analog Devices | 4A dual-output digital isolator, SOIC-16 | 3 | $2.40 ea |
| Q1–Q6 | IPT015N10N5ATMA1 | Infineon | 100V N-ch MOSFET, 1.5 mΩ, PG-TDSON-8 | 6 | $1.80 ea |
| U13–U15 | INA241AIDR | TI | 50V/V bidirectional current-sense amp, SOIC-8 | 3 | $1.50 ea |
| U16 | AT30TS74-MA8M-T | Microchip | I2C temp sensor ±0.5°C, SOT-23-5 | 1 | $0.45 |
| D1 | SMBJ58A-13-F | Diodes Inc. | 58V TVS, SMB | 1 | $0.30 |
| D2 | SS34-E3/57T | Vishay | 40V Schottky, 3A, SMA | 1 | $0.15 |
| L1 | XAL6060-472MEB | Coilcraft | 4.7 µH 6A DCDC inductor, 6×6 mm | 1 | $0.80 |
| L2 | XAL4020-222MEB | Coilcraft | 2.2 µH 3A DCDC inductor | 1 | $0.50 |
| Y1 | ABM8-8.000MHZ-20-B1Y-T | Abracon | 8 MHz crystal, 20 ppm | 1 | $0.30 |
| Y2 | ABM8-32.000MHZ-20-B1Y-T | Abracon | 32 MHz crystal (for nRF52832) | 1 | $0.30 |
| C1–C4 | EKY-500ETD471MJC5D | United Chemi-Con | 470 µF 63V aluminium polymer, 10×10 mm | 4 | $1.20 ea |
| C5–C10 | GRM32ER71H226KE15L | Murata | 22 µF 50V X7R 1210 | 6 | $0.35 ea |
| C11–C16 | GRM21BC71E106ME11L | Murata | 10 µF 25V X7R 0805 | 6 | $0.15 ea |
| C17–C24 | GRM188R71H103KA01D | Murata | 10 nF 50V X7R 0603 | 8 | $0.02 ea |
| C25–C40 | GRM188R71C104KA01D | Murata | 100 nF 50V X7R 0603 (decoupling) | 16 | $0.01 ea |
| R1–R6 | CRCW120610R0FKEA | Vishay | 10 Ω 1/4W gate resistors, 1206 | 6 | $0.02 ea |
| R7–R9 | CSR1206-0R5FKE | Vishay | 0.5 mΩ 1W current-sense, 1206 | 3 | $0.60 ea |
| R10 | CRCW0603100KFKEA | Vishay | 100 kΩ VBAT divider upper | 1 | $0.01 |
| R11 | CRCW0603330RFKEA | Vishay | 3.3 kΩ VBAT divider lower | 1 | $0.01 |
| R12 | CRCW0603100RFKEA | Vishay | 100 Ω CAN termination | 2 | $0.01 |
| J1 | 6435228-3 | TE | 3-phase + HV connector, 10A/pin, 4-pin | 1 | $1.20 |
| J2 | 1734528-1 | TE | CAN + PWR connector, 5-pin | 1 | $0.80 |
| J3 | 12401832E412A | Amphenol | USB-C 2.0 receptacle, 16-pin | 1 | $0.60 |
| J4 | 505570-0601 | Molex | Hall sensor input, 6-pin | 1 | $0.40 |
| SW1 | SKQGAFE010 | ALPS | Reset tactile switch, 6×3.5 mm | 1 | $0.15 |
| LED1–LED2 | 150060VS75000 | Würth | Green/Red 0603 LED | 2 | $0.05 ea |
| FB1–FB2 | BLM18PG121SN1D | Murata | 120 Ω @ 100 MHz ferrite bead, 0603 | 2 | $0.05 ea |
| T1 | 744273101 | Würth | Common-mode choke, 3-phase, 10A | 1 | $2.50 |

**Total estimated BOM cost (1K volume): ~$44.80**

## 2. Pinout Tables

### 2.1 STM32G474CEU6 (U1) — UFQFPN-48

| Pin | Name | Function | Net |
|---|---|---|---|
| 1 | VBAT | Battery backup | VBAT_SENSE (via divider) |
| 2 | PC13 | GPIO | STATUS_LED (active low) |
| 3 | PC14 | GPIO | FAULT_LED (active low) |
| 4 | PC15 | GPIO | HALL_A_IN |
| 5 | PF0 / OSC_IN | HSE input | Y1_OUT (8 MHz) |
| 6 | PF1 / OSC_OUT | HSE output | Y1_IN (8 MHz) |
| 7 | NRST | Reset | SW1 / RST |
| 8 | PC0 | ADC1_IN1 | CUR_A (INA241A U13 out) |
| 9 | PC1 | ADC1_IN2 | CUR_B (INA241A U14 out) |
| 10 | PC2 | ADC1_IN3 | CUR_C (INA241A U15 out) |
| 11 | PC3 | ADC2_IN4 | VBAT_ADC (divider) |
| 12 | PA0 | ADC2_IN5 | TEMP_MCU (internal) |
| 13 | PA1 | ADC2_IN6 | TEMP_EXT (AT30TS74) |
| 14 | PA2 | USART2_TX | BLE_TX (to nRF52832 RX) |
| 15 | PA3 | USART2_RX | BLE_RX (from nRF52832 TX) |
| 16 | PA4 | SPI1_NSS | CAN_CS (MCP2518FD) |
| 17 | PA5 | SPI1_SCK | SPI1_SCK |
| 18 | PA6 | SPI1_MISO | SPI1_MISO |
| 19 | PA7 | SPI1_MOSI | SPI1_MOSI |
| 20 | PB0 | GPIO | GATE_EN (global gate enable) |
| 21 | PB1 | GPIO | DESAT_FAULT (active low) |
| 22 | PB2 | SPI1_NSS_ALT | FLASH_CS (W25Q128) |
| 23 | PB10 | I2C1_SCL | I2C1_SCL |
| 24 | PB11 | I2C1_SDA | I2C1_SDA |
| 25 | PB12 | GPIO | CAN_INT (MCP2518FD INT) |
| 26 | PB13 | GPIO | CAN_RST (MCP2518FD RST) |
| 27 | PB14 | TIM1_CH2N | PWM_B_LOW |
| 28 | PB15 | TIM1_CH3N | PWM_C_LOW |
| 29 | PA8 | TIM1_CH1 | PWM_A_HIGH |
| 30 | PA9 | TIM1_CH2 | PWM_B_HIGH |
| 31 | PA10 | TIM1_CH3 | PWM_C_HIGH |
| 32 | PA11 | TIM1_CH1N | PWM_A_LOW |
| 33 | PA12 | USB_DP | USB_DP |
| 34 | PA13 | SWDIO | SWDIO |
| 35 | PA14 | SWCLK | SWCLK |
| 36 | PA15 | GPIO | FLASH_WP (W25Q128 WP) |
| 37 | PB3 | GPIO | FLASH_HOLD (W25Q128 HOLD) |
| 38 | PB4 | GPIO | BLE_RST (nRF52832 RST) |
| 39 | PB5 | GPIO | HALL_B_IN |
| 40 | PB6 | GPIO | HALL_C_IN |
| 41 | PB7 | GPIO | PMIC_IRQ (TPS65218 IRQ) |
| 42 | PB8 | BOOT0 | BOOT0 (pulled low) |
| 43 | VDDA | Analog supply | VDDA (3.3V filtered) |
| 44 | VSSA | Analog ground | GND |
| 45–48 | VDD/VSS | Power | 3.3V / GND |

### 2.2 MCP2518FD (U3) — TSSOP-14

| Pin | Name | Net |
|---|---|---|
| 1 | TXCAN | CAN_TX (to TJA1463 TXD) |
| 2 | RXCAN | CAN_RX (from TJA1463 RXD) |
| 3 | CLKSEL | GND (internal oscillator) |
| 4 | INT | CAN_INT (to STM32 PB12) |
| 5 | SCK | SPI1_SCK |
| 6 | SI | SPI1_MOSI |
| 7 | SO | SPI1_MISO |
| 8 | CS | CAN_CS (from STM32 PA4) |
| 9 | RESET | CAN_RST (from STM32 PB13) |
| 10 | VDD | 3.3V |
| 11 | VSS | GND |
| 12 | OSC1 | (NC, internal osc) |
| 13 | OSC2 | (NC, internal osc) |
| 14 | STBY | GND (normal mode) |

### 2.3 nRF52832-CIAA (U5) — QFN-48 (relevant pins only)

| Pin | Name | Net |
|---|---|---|
| 1 | DEC1 | 1.8V decoupling |
| 7 | P0.06 | BLE_RX (from STM32 PA2 TX) |
| 8 | P0.08 | BLE_TX (to STM32 PA3 RX) |
| 9 | P0.09 | BLE_CTS |
| 10 | P0.10 | BLE_RTS |
| 13 | P0.11 | BLE_RST (from STM32 PB4) |
| 22 | XL1 | Y2_OUT (32 MHz) |
| 23 | XL2 | Y2_IN (32 MHz) |
| 30 | VDD | 3.3V |
| 33 | VDD_nRF | 3.3V |
| 45 | ANT | ANT (2.4 GHz, to PCB trace antenna) |
| 48 | GND | GND |

## 3. Netlist (Source Pin → Component → Dest Pin)

### 3.1 Power Netlist

| Net | Source | Destination(s) |
|---|---|---|
| VBATT | J1 pin 1 (HV+) | D2 anode → D1 cathode; C1–C4 (+); Q1/Q3/Q5 drains |
| HV_BUS | D2 cathode | C1–C4 (+); R10 upper; U7 VCC; U8 VCC; U9 VCC |
| 5V0 | U2 DCDC1 out | C5 (+); U3 VDD; U4 VCC; FB1 input |
| 3V3 | U2 LDO1 out | C11 (+); U1 VDD; U6 VDD; U10–U12 VDDA; FB2 input |
| 1V8 | U2 LDO2 out | U5 DEC4, C16 (+) |
| VDDA | 3V3 → L1 ferrite → C17 | U1 pin 43 |
| GND | Star point | All IC GND pins, C(−) terminals |

### 3.2 Gate Drive Netlist

| Net | Source MCU Pin | Isolator | Gate Driver | MOSFET |
|---|---|---|---|---|
| PWM_A_HIGH | PA8 → U10 INA | ADuM3223 U10 OUTA | IRS2186 U7 INA | Q1 gate (via R1) |
| PWM_A_LOW | PA11 → U10 INB | ADuM3223 U10 OUTB | IRS2186 U7 INB | Q2 gate (via R2) |
| PWM_B_HIGH | PA9 → U11 INA | ADuM3223 U11 OUTA | IRS2186 U8 INA | Q3 gate (via R3) |
| PWM_B_LOW | PB14 → U11 INB | ADuM3223 U11 OUTB | IRS2186 U8 INB | Q4 gate (via R4) |
| PWM_C_HIGH | PA10 → U12 INA | ADuM3223 U12 OUTA | IRS2186 U9 INA | Q5 gate (via R5) |
| PWM_C_LOW | PB15 → U12 INB | ADuM3223 U12 OUTB | IRS2186 U9 INB | Q6 gate (via R6) |

### 3.3 Current Sense Netlist

| Net | Shunt | Sense Amp | MCU ADC |
|---|---|---|---|
| CUR_A | R7 (PH_A to GND) → U13 IN+ / IN− | INA241A U13 OUT | PC0 (ADC1_IN1) |
| CUR_B | R8 (PH_B to GND) → U14 IN+ / IN− | INA241A U14 OUT | PC1 (ADC1_IN2) |
| CUR_C | R9 (PH_C to GND) → U15 IN+ / IN− | INA241A U15 OUT | PC2 (ADC1_IN3) |

### 3.4 Communication Netlist

| Net | Source | Dest | Protocol |
|---|---|---|---|
| SPI1_SCK | U1 PA5 | U3 pin 5, U6 pin 6 | SPI 20 MHz |
| SPI1_MISO | U6 pin 2, U3 pin 7 | U1 PA6 | SPI |
| SPI1_MOSI | U1 PA7 | U3 pin 6, U6 pin 5 | SPI |
| CAN_CS | U1 PA4 | U3 pin 8 | GPIO |
| FLASH_CS | U1 PB2 | U6 pin 1 | GPIO |
| I2C1_SCL | U1 PB10 | U2 pin D3, U16 pin 2 | I2C 400 kHz |
| I2C1_SDA | U1 PB11 | U2 pin D4, U16 pin 3 | I2C 400 kHz |
| BLE_TX | U1 PA2 | U5 pin 8 | UART 1 Mbps |
| BLE_RX | U1 PA3 | U5 pin 7 | UART 1 Mbps |
| CAN_H | U4 pin 7 | J2 pin 1, R12 | CAN FD diff pair |
| CAN_L | U4 pin 6 | J2 pin 2, R12 | CAN FD diff pair |
| USB_DP | U1 PA12 | J3 pin A2 | USB 2.0 FS |

## 4. Decoupling Networks

### 4.1 STM32G474 (U1)
- VDD pins (45–48): 4× 100 nF X7R 0603 (C25–C28) + 1× 10 µF X7R 0805 (C11), each within 3 mm
- VDDA (pin 43): 1× 10 nF X7R 0603 (C17) + 1× 100 nF X7R 0603 (C29) + 1× 1 µF X7R 0603 (C30), with ferrite bead BLM18PG121SN1D (FB2) in series from 3V3

### 4.2 MCP2518FD (U3)
- VDD: 1× 100 nF (C31) + 1× 10 µF (C12), within 5 mm

### 4.3 Gate Drivers (U7–U9)
- VCC each: 1× 1 µF X7R 0805 (C18–C20) + 1× 100 nF X7R 0603 (C32–C34)
- Bootstrap each: 1× 100 nF X7R 0603 (C35–C37) from BOOT pin to phase output

### 4.4 MOSFET Gate Decoupling
- Each gate: 10 Ω series resistor (R1–R6, 1206) + 1× 10 nF X7R 0603 (C38–C43) gate-to-source for dV/dt immunity

### 4.5 nRF52832 (U5)
- VDD pins: 2× 100 nF (C44–C45) + 1× 4.7 µF (C46), within 2 mm of pins
- DEC1/2: 1× 100 nF (C47–C48) on each decoupling pin

## 5. Impedance-Matched Pairs

| Pair | Impedance | Notes |
|---|---|---|
| CAN_H / CAN_L | 120 Ω differential | Terminated with 120 Ω at each end of bus |
| USB_DP / USB_DM | 90 Ω differential ±10% | Routed on top layer over GND plane |
| ANT (2.4 GHz) | 50 Ω single-ended | Microstrip from U5 pin 45 to PCB antenna |

## 6. Pull-Up / Pull-Down Values

| Net | Resistor | Value | Purpose |
|---|---|---|---|
| BOOT0 | R20 | 10 kΩ to GND | Boot from Flash |
| NRST | R21 | 10 kΩ to 3V3 | Reset pull-up |
| CAN_CS | R22 | 10 kΩ to 3V3 | SPI CS idle high (active low) |
| FLASH_CS | R23 | 10 kΩ to 3V3 | SPI CS idle high (active low) |
| CAN_INT | R24 | 10 kΩ to 3V3 | Interrupt active low |
| DESAT_FAULT | R25 | 10 kΩ to 3V3 | Fault active low |
| HALL_A/B/C | R26–R28 | 4.7 kΩ to 3V3 | Hall sensor pull-ups |
| GATE_EN | R29 | 10 kΩ to GND | Default: gates disabled (safe) |
| BLE_RST | R30 | 10 kΩ to 3V3 | nRF held in run mode |
| PMIC_IRQ | R31 | 10 kΩ to 3V3 | Interrupt active low |

## 7. TVS & Protection

| Location | Part | Clamping | Purpose |
|---|---|---|---|
| VBATT input | SMBJ58A (D1) | 58 V | Over-voltage from regen/charger |
| CAN bus | PESD2CANUX (D3) | 24 V | ESD on CAN lines |
| USB | PRTR5V0U2X (D4) | 6 V | ESD + short-circuit on USB |
| Each phase output | P6SMB16A (D5–D7) | 16 V | Inductive kickback clamp |