# Vortex-SDR Phase 2: Component Selection & Schematics

## 1. Component Selection Rationale

### 1.1 MCU: STM32H743ZIT6

The STM32H743 was selected over alternatives for these reasons:

| Criterion | STM32H743 | i.MX RT1062 | ESP32-S3 | SAMD51 |
|---|---|---|---|---|
| Core | Cortex-M7 @ 480 MHz | Cortex-M7 @ 600 MHz | Xtensa @ 240 MHz | Cortex-M4 @ 120 MHz |
| RAM | 1 MB | 1 MB | 512 KB | 256 KB |
| Flash | 1 MB | 1 MB | 8 MB (external) | 512 KB |
| FPU | Double-precision | Double-precision | Single-precision | Single-precision |
| DSP | SIMD, MAC | SIMD, MAC | None | None |
| USB | HS PHY | HS PHY | FS only | FS only |
| LVDS | No (via FPGA) | No (via FPGA) | No | No |
| Power | 350 mW typical | 400 mW typical | 200 mW typical | 100 mW typical |
| Availability | 15-year commitment | 10-year commitment | 5-year | 10-year |
| Cost | $18.50 | $22.00 | $4.50 | $7.20 |

Key selection factors:
- **480 MHz + DP-FPU**: Sufficient for FFT post-processing, display rendering, and protocol handling
- **1 MB SRAM**: Multiple FFT buffers without external RAM
- **USB HS PHY**: 480 Mbps for bulk data transfer
- **Multiple SPI ports**: Separate buses for FPGA, display, and flash
- **15-year availability**: Industrial lifecycle commitment from ST

### 1.2 FPGA: iCE40UP5K-SG48

Selected for on-chip FFT acceleration:

| Criterion | iCE40UP5K | ECP5-25K | MAX10-08 |
|---|---|---|---|
| LUTs | 5280 | 24880 | 8000 |
| DSP | 1 MAC16 | 24 MAC16 | 12 MAC18 |
| RAM | 128 KB | 636 KB | 375 KB |
| I/O | 33 | 196 | 60 |
| LVDS | Yes | Yes | No |
| Power | ≤ 100 mW | ≤ 300 mW | ≤ 500 mW |
| Cost | $5.90 | $12.00 | $8.50 |
| Open toolchain | IceStorm | IceStorm/Yosys | Quartus only |

Key selection factors:
- **5280 LUTs + SPRAM**: Sufficient for 1024-point FFT + windowing + peak detect
- **LVDS input**: Direct interface to AD9645 ADC
- **Low power**: Critical for battery operation
- **Open toolchain**: IceStorm + Yosys for fully open-source FPGA flow
- **SPI configuration**: Simple passive configuration from MCU

### 1.3 ADC: AD9645BCZ-80

| Criterion | AD9645 | AD9236 | LTC2208 |
|---|---|---|---|
| Resolution | 14 bits | 12 bits | 16 bits |
| Sample rate | 80 MSPS (61.44 used) | 20 MSPS | 65 MSPS |
| SFDR | 85 dB | 80 dB | 82 dB |
| SNR | 73 dBFS | 70 dBFS | 77 dBFS |
| Interface | LVDS/CMOS | CMOS | CMOS |
| Power | 370 mW | 120 mW | 500 mW |
| Cost | $12.80 | $8.50 | $25.00 |

Key selection factors:
- **14-bit resolution**: Adequate dynamic range for spectrum analysis
- **61.44 MSPS operation**: Provides 30.72 MHz Nyquist bandwidth (enough for 10 MHz IF)
- **LVDS output**: Direct interface to FPGA, minimal MCU loading
- **370 mW at 80 MSPS**: Acceptable for battery operation
- **Internal reference**: Simplifies power supply

### 1.4 Mixer: ADL5801ACPZ

| Criterion | ADL5801 | AD8343 | LTC5599 |
|---|---|---|---|
| Frequency range | 100 kHz – 6 GHz | 0 – 500 MHz | 300 – 6000 MHz |
| Conversion gain | 24 dB | 20 dB | 3.4 dB |
| Noise figure | 14 dB | 12 dB | 15 dB |
| OIP3 | 36 dBm | 30 dBm | 27 dBm |
| LO drive | -10 to +10 dBm | -10 dBm | 0 dBm |
| Power | 350 mW | 250 mW | 200 mW |
| Cost | $8.40 | $5.20 | $9.50 |

Key selection factors:
- **6 GHz bandwidth**: Covers entire target range in a single mixer
- **24 dB conversion gain**: Reduces need for additional IF amplification
- **Wide LO drive range**: Compatible with ADF4351 direct output

### 1.5 PLL: ADF4351BCPZ

| Criterion | ADF4351 | ADF4153 | LMX2572 |
|---|---|---|---|
| Frequency range | 35 MHz – 4.4 GHz | 0.5 – 6 GHz | 10 – 6400 MHz |
| Resolution | Fractional-N (sub-Hz) | Integer-N | Fractional-N |
| Phase noise (1 GHz) | -90 dBc/Hz @ 10 kHz | -85 dBc/Hz | -92 dBc/Hz |
| VCO | Integrated | External | Integrated |
| Power | 500 mW | 200 mW | 600 mW |
| Cost | $9.20 | $6.50 | $18.00 |

Key selection factors:
- **Integrated VCO**: No external VCO needed, simplifies layout
- **4.4 GHz + ADF5002 prescaler**: Extends to 6 GHz with /2
- **Sub-Hz resolution**: Enables 1 Hz tuning step size
- **SPI programming**: Directly compatible with MCU SPI

## 2. Complete BOM

### 2.1 Active Components

| Ref | Component | Part Number | Package | Qty | Unit Price | Extended | Digikey/Mouser |
|---|---|---|---|---|---|---|---|
| U1 | MCU | STM32H743ZIT6 | LQFP-144 | 1 | $18.50 | $18.50 | 497-17440-ND |
| U2 | FPGA | iCE40UP5K-SG48 | QFN-48 | 1 | $5.90 | $5.90 | 220-1617-ND |
| U3 | ADC | AD9645BCZ-80 | LQFP-64 | 1 | $12.80 | $12.80 | AD9645-80EBZ |
| U4 | Mixer | ADL5801ACPZ | LFCSP-24 | 1 | $8.40 | $8.40 | ADL5801ACPZ-R7 |
| U5 | PLL+VCO | ADF4351BCPZ | LFCSP-32 | 1 | $9.20 | $9.20 | ADF4351BCPZ |
| U6 | LNA | SPF5189Z | SOT-89 | 1 | $0.80 | $0.80 | SPF5189Z |
| U7 | Buck regulator | TPS62A02VLSR | VSON-8 | 1 | $1.20 | $1.20 | 296-50390-ND |
| U8 | LDO 1.8V | TLV75518PDBV | SOT-23-5 | 1 | $0.35 | $0.35 | 296-47522-ND |
| U9 | LDO 3.3V analog | TLV75533PDBV | SOT-23-5 | 1 | $0.35 | $0.35 | 296-47523-ND |
| U10 | BLE module | nRF52832-MK | QFN-48 module | 1 | $4.50 | $4.50 | NRF52832-MK |
| U11 | TFT display | ILI9341 TFT 2.8" | Module | 1 | $6.00 | $6.00 | N/A (AliExpress) |
| U12 | Flash | W25Q256JVEQ | SOP-8 | 1 | $1.80 | $1.80 | W25Q256JVEQ |
| U13 | LiPo charger | MCP73871T-2CCI | MSOP-10 | 1 | $1.50 | $1.50 | MCP73871T-2CCI |
| U14 | SAW filter | TA1099EC8110 | SMD 3×3 | 1 | $2.10 | $2.10 | TA1099EC8110 |
| U15 | Prescaler | ADF5002BCPZ | LFCSP-8 | 1 | $3.80 | $3.80 | ADF5002BCPZ |
| U16 | Op-amp | OPA364AIDBVT | SOT-23-5 | 1 | $0.55 | $0.55 | OPA364AIDBVT |
| U17 | ESD protection | TPD4E05U06DQAR | USON-10 | 1 | $0.45 | $0.45 | TPD4E05U06DQAR |
| U18 | Level shifter | TXB0108PWR | TSSOP-20 | 2 | $0.90 | $1.80 | 296-18661-ND |
| U19 | Touch controller | XPT2046TS | TSSOP-16 | 1 | $0.60 | $0.60 | XPT2046TS |

### 2.2 Passive Components

| Ref | Component | Value | Package | Qty | Unit Price | Extended |
|---|---|---|---|---|---|---|
| Y1 | Crystal | 8.000 MHz | HC49/SMD | 1 | $0.30 | $0.30 |
| Y2 | Crystal | 32.768 kHz | ABS07 | 1 | $0.25 | $0.25 |
| Y3 | TCXO | 61.44 MHz | SMD 5×3.2 | 1 | $2.80 | $2.80 |
| Y4 | TCXO | 25.000 MHz | SMD 5×3.2 | 1 | $1.90 | $1.90 |
| L1 | Inductor | 10 µH | SRN4018 | 1 | $0.20 | $0.20 |
| L2 | Inductor | 2.2 µH | SRN3015 | 1 | $0.15 | $0.15 |
| L3 | Inductor | 10 µH (LC filter) | SRN4018 | 2 | $0.20 | $0.40 |
| L4-L7 | Bead | 600 Ω @ 100 MHz | 0603 | 4 | $0.02 | $0.08 |

#### Decoupling Capacitors

| Ref | Value | Voltage | Package | Qty | Purpose |
|---|---|---|---|---|---|
| C1-C4 | 100 µF | 6.3V | 0805 | 4 | TPS62A02 output bulk |
| C5-C6 | 10 µF | 10V | 0603 | 2 | MCU VCAP, MCU VDD |
| C7-C14 | 100 nF | 10V | 0402 | 8 | MCU decoupling (VDD pairs) |
| C15-C16 | 100 nF | 10V | 0402 | 2 | FPGA VCC decoupling |
| C17-C18 | 100 nF | 10V | 0402 | 2 | FPGA VCCIO decoupling |
| C19 | 10 µF | 10V | 0603 | 1 | ADC AVDD decoupling |
| C20 | 100 nF | 10V | 0402 | 1 | ADC DVDD decoupling |
| C21-C22 | 100 nF | 10V | 0402 | 2 | PLL decoupling |
| C23-C24 | 10 µF | 10V | 0603 | 2 | LDO output (1.8V, 3.3VA) |
| C25 | 100 pF | 50V | 0402 | 1 | Mixer LO bypass |
| C26 | 100 pF | 50V | 0402 | 1 | Mixer RF bypass |
| C27 | 10 nF | 25V | 0402 | 1 | PLL loop filter C1 |
| C28 | 1 nF | 25V | 0402 | 1 | PLL loop filter C2 |
| C29-C30 | 22 pF | 25V | 0402 | 2 | Crystal load caps (8 MHz) |
| C31-C32 | 12 pF | 25V | 0402 | 2 | Crystal load caps (32.768 kHz) |
| C33 | 1 µF | 10V | 0603 | 1 | ADC reference bypass |
| C34 | 10 nF | 50V | 0402 | 1 | LNA output matching |
| C35 | 100 nF | 50V | 0402 | 1 | SAW filter output coupling |

#### Resistors

| Ref | Value | Tolerance | Package | Qty | Purpose |
|---|---|---|---|---|---|
| R1 | 4.7 kΩ | 1% | 0402 | 1 | PLL loop filter R1 |
| R2 | 10 kΩ | 1% | 0402 | 1 | ADF4351 MUXOUT pull-up |
| R3 | 47 Ω | 1% | 0402 | 1 | USB D+ series |
| R4 | 47 Ω | 1% | 0402 | 1 | USB D- series |
| R5-R6 | 1.5 kΩ | 1% | 0402 | 2 | USB D+ pull-up (1.5k to VDD) |
| R7 | 1 kΩ | 1% | 0402 | 1 | LED series (green) |
| R8 | 1 kΩ | 1% | 0402 | 1 | LED series (red) |
| R9 | 100 kΩ | 1% | 0402 | 1 | MCP73871 PROG resistor |
| R10 | 10 kΩ | 1% | 0402 | 1 | Battery voltage divider upper |
| R11 | 10 kΩ | 1% | 0402 | 1 | Battery voltage divider lower |
| R12-R17 | 10 kΩ | 1% | 0402 | 6 | Button pull-ups |
| R18 | 22 Ω | 1% | 0402 | 1 | TCXO enable bias |
| R19 | 330 Ω | 1% | 0402 | 1 | ADC driver gain |
| R20 | 330 Ω | 1% | 0402 | 1 | ADC driver feedback |
| R21 | 50 Ω | 1% | 0402 | 1 | SMA input 50Ω termination |

### 2.3 Connectors & Mechanical

| Ref | Component | Part Number | Qty | Purpose |
|---|---|---|---|---|
| J1 | SMA connector | 142-0701-801 | 1 | RF input |
| J2 | USB-C 16-pin | 12401832E4#A | 1 | USB + charging |
| J3 | JST-PH 2-pin | B2B-PH-K-S | 1 | Battery connector |
| J4 | SWD 5-pin header | 2.54mm pitch | 1 | Debug port |
| SW1-SW6 | Tactile switch | EVQPUJ02K | 6 | User buttons |
| LED1 | Green LED | LTST-C190GKT | 1 | Status |
| LED2 | Red LED | LTST-C190CKT | 1 | Error |
| FB1-FB4 | Ferrite bead | BLM18AG601SN1 | 4 | EMI suppression |

### 2.4 BOM Cost Summary

| Category | Extended Cost |
|---|---|
| Active ICs | $87.10 |
| Passives (caps, resistors, inductors) | $3.50 |
| Crystals & TCXOs | $5.25 |
| Connectors & mechanical | $4.80 |
| **Total (1 qty)** | **$100.65** |

> Note: At volume (100+), BOM cost drops below $70 due to IC pricing breaks.

## 3. Pin Assignments (Detailed)

### 3.1 STM32H743ZIT6 Pin Assignment

#### Power Pins

| Pin | Net | Notes |
|---|---|---|
| 15, 38, 56, 72, 95, 118, 139 | VDD | 3.3V digital power |
| 16, 39, 57, 73, 96, 119, 140 | VSS | Digital ground |
| 30 | VCAP1 | 2.2 µF to VSS (internal regulator) |
| 61 | VCAP2 | 2.2 µF to VSS (internal regulator) |
| 21 | VBAT | Battery backup (3.3V via diode) |
| 22 | VDDA | 3.3V analog (filtered) |
| 23 | VSSA | Analog ground |
| 33 | VREF+ | 3.3V reference (filtered) |
| 34 | VREF- | Analog ground |

#### GPIO Port A

| Pin | Net | AF | Mode | Drive | Notes |
|---|---|---|---|---|---|
| PA0 | NET_ADC_IRQ | GPIO | Input | — | FPGA interrupt |
| PA1 | NET_LNA_EN | — | Output push-pull | Low | LNA enable |
| PA2 | NET_USART2_TX | AF7 | AF push-pull | High | BLE TX |
| PA3 | NET_USART2_RX | AF7 | AF push-pull | High | BLE RX |
| PA4 | NET_DAC1_OUT | — | Analog | — | AGC voltage |
| PA5 | NET_SPI1_SCK | AF5 | AF push-pull | High | FPGA SPI clock |
| PA6 | NET_SPI1_MISO | AF5 | Input | — | FPGA SPI data out |
| PA7 | NET_SPI1_MOSI | AF5 | AF push-pull | High | FPGA SPI data in |
| PA8 | NET_MCO1 | AF0 | AF push-pull | High | 8 MHz reference |
| PA9 | NET_USB_VBUS | AF10 | Input | — | USB VBUS detect |
| PA10 | NET_USB_ID | AF10 | Input | — | USB OTG ID |
| PA11 | NET_USB_DM | AF10 | AF push-pull | High | USB D- |
| PA12 | NET_USB_DP | AF10 | AF push-pull | High | USB D+ |
| PA13 | NET_JTMS | AF0 | AF push-pull | — | SWDIO |
| PA14 | NET_JTCK | AF0 | AF push-pull | — | SWCLK |
| PA15 | NET_FPGA_CDONE | — | Input | — | FPGA config done |

#### GPIO Port B

| Pin | Net | AF | Mode | Drive | Notes |
|---|---|---|---|---|---|
| PB0 | NET_MIXER_EN | — | Output push-pull | Low | Mixer enable |
| PB1 | NET_ATTEN_SEL | — | Output push-pull | Low | Attenuator select |
| PB2 | NET_SPI2_SCK | AF5 | AF push-pull | High | Display SPI clock |
| PB3 | NET_SPI2_MOSI | AF5 | AF push-pull | High | Display SPI data |
| PB4 | NET_LCD_DC | — | Output push-pull | Low | Display D/C |
| PB5 | NET_LCD_CS | — | Output push-pull | Low | Display CS# |
| PB6 | NET_LCD_RST | — | Output push-pull | Low | Display reset |
| PB7 | NET_TOUCH_IRQ | — | Input pull-up | — | Touch interrupt |
| PB8 | NET_I2C1_SCL | AF4 | AF open-drain | High | Touch I2C clock |
| PB9 | NET_I2C1_SDA | AF4 | AF open-drain | High | Touch I2C data |
| PB10 | NET_PLL_LE | — | Output push-pull | Low | ADF4351 latch enable |
| PB11 | NET_PLL_CE | — | Output push-pull | Low | ADF4351 chip enable |
| PB12 | NET_SPI3_SCK | AF6 | AF push-pull | High | Flash SPI clock |
| PB13 | NET_SPI3_MISO | AF6 | Input | — | Flash SPI data out |
| PB14 | NET_SPI3_MOSI | AF6 | AF push-pull | High | Flash SPI data in |
| PB15 | NET_FLASH_CS | — | Output push-pull | Low | Flash CS# |

#### GPIO Port C

| Pin | Net | AF | Mode | Drive | Notes |
|---|---|---|---|---|---|
| PC0-PC5 | NET_ADC_Dx | AF8 | AF | — | LVDS ADC data |
| PC6 | NET_FPGA_RST | — | Output push-pull | Low | FPGA reset |
| PC7 | NET_FPGA_CS | — | Output push-pull | Low | FPGA SPI CS# |
| PC8 | NET_TOUCH_CS | — | Output push-pull | Low | Touch CS# |
| PC9 | NET_LED_STATUS | — | Output push-pull | Low | Green LED |
| PC10 | NET_LED_ERROR | — | Output push-pull | Low | Red LED |
| PC11 | NET_CHG_STAT1 | — | Input | — | Charger status 1 |
| PC12 | NET_CHG_STAT2 | — | Input | — | Charger status 2 |

#### GPIO Port D

| Pin | Net | AF | Mode | Drive | Notes |
|---|---|---|---|---|---|
| PD0 | NET_FPGA_SCK | — | Output push-pull | High | FPGA config clock |
| PD1 | NET_FPGA_SI | — | Output push-pull | High | FPGA config data |
| PD2 | NET_FPGA_SS | — | Output push-pull | Low | FPGA config select |
| PD3 | NET_BAT_SENSE | — | Analog | — | Battery ADC |
| PD4 | NET_TEMP_SENSE | — | Analog | — | Temp ADC |
| PD5 | NET_MUX_SEL0 | — | Output push-pull | Low | RF mux select 0 |
| PD6 | NET_MUX_SEL1 | — | Output push-pull | Low | RF mux select 1 |
| PD7 | NET_MUX_EN | — | Output push-pull | Low | RF mux enable |
| PD8 | NET_USART1_TX | AF7 | AF push-pull | High | Debug UART TX |
| PD9 | NET_USART1_RX | AF7 | Input | — | Debug UART RX |

#### GPIO Port E

| Pin | Net | AF | Mode | Drive | Notes |
|---|---|---|---|---|---|
| PE0 | NET_BTN_CENTER | — | Input pull-up | — | Center button |
| PE1 | NET_BTN_UP | — | Input pull-up | — | Up button |
| PE2 | NET_BTN_DOWN | — | Input pull-up | — | Down button |
| PE3 | NET_BTN_LEFT | — | Input pull-up | — | Left button |
| PE4 | NET_BTN_RIGHT | — | Input pull-up | — | Right button |
| PE5 | NET_BTN_MENU | — | Input pull-up | — | Menu button |

### 3.2 iCE40UP5K-SG48 Pin Assignment

| Pin | Net | Bank | Function |
|---|---|---|---|
| 1 | VCCIO_2 | Bank 2 | 1.8V I/O power |
| 2 | NET_ADC_D0_P | Bank 2 | LVDS input bit 0+ |
| 3 | NET_ADC_D0_N | Bank 2 | LVDS input bit 0- |
| 4 | NET_ADC_D1_P | Bank 2 | LVDS input bit 1+ |
| 5 | NET_ADC_D1_N | Bank 2 | LVDS input bit 1- |
| 6 | NET_ADC_D2_P | Bank 2 | LVDS input bit 2+ |
| 7 | NET_ADC_D2_N | Bank 2 | LVDS input bit 2- |
| 8 | GND | — | Ground |
| 9 | NET_ADC_CLK_P | Bank 2 | LVDS clock+ |
| 10 | NET_ADC_CLK_N | Bank 2 | LVDS clock- |
| 11 | VCC | Core | 1.2V core power |
| 12-19 | NET_ADC_D3-10 | Bank 2 | LVDS data bits 3-10 |
| 20 | GND | — | Ground |
| 21-25 | NET_FPGA_IO0-4 | Bank 0 | 3.3V GPIO |
| 26 | NET_SPI_SS | Bank 0 | SPI chip select |
| 27 | NET_SPI_SCK | Bank 0 | SPI clock |
| 28 | NET_SPI_MOSI | Bank 0 | SPI data in |
| 29 | NET_SPI_MISO | Bank 0 | SPI data out |
| 30 | VCCIO_0 | Bank 0 | 3.3V I/O power |
| 31 | NET_CDONE | Bank 0 | Configuration done |
| 32 | NET_FPGA_RST | Bank 0 | Reset (active-low) |
| 33-38 | NET_FPGA_IO5-10 | Bank 0 | 3.3V GPIO |
| 39-44 | NET_FPGA_IO11-16 | Bank 1 | 3.3V GPIO |
| 45 | NET_FPGA_SCK | Bank 1 | Config clock |
| 46 | NET_FPGA_SI | Bank 1 | Config data in |
| 47 | NET_FPGA_SS | Bank 1 | Config select |
| 48 | VCCIO_1 | Bank 1 | 3.3V I/O power |

### 3.3 ADF4351 Pin Assignment (SPI bit-bang)

| Pin | Net | MCU Pin | Notes |
|---|---|---|---|
| CLK | NET_PLL_SCK | PB10 (GPIO) | SPI clock (bit-bang) |
| DATA | NET_PLL_MOSI | PB11 (GPIO) | SPI data (bit-bang) |
| LE | NET_PLL_LE | PB10 (GPIO) | Latch enable |
| CE | NET_PLL_CE | PB11 (GPIO) | Chip enable |
| MUXOUT | NET_PLL_MUX | PC10 (GPIO) | Lock detect / MUX output |
| RFOUTA+ | NET_LO_P | Direct to mixer | LO positive output |
| RFOUTA- | NET_LO_N | Direct to mixer | LO negative output |

### 3.4 AD9645 Pin Assignment (LVDS to FPGA, SPI from MCU)

| Pin | Net | MCU/FPGA Pin | Notes |
|---|---|---|---|
| D0+/D0- | NET_ADC_D0_P/N | FPGA pin 2/3 | LVDS bit 0 |
| D1+/D1- | NET_ADC_D1_P/N | FPGA pin 4/5 | LVDS bit 1 |
| D2+/D2- | NET_ADC_D2_P/N | FPGA pin 6/7 | LVDS bit 2 |
| ... | ... | ... | LVDS bits 3-13 |
| CLK+/CLK- | NET_ADC_CLK_P/N | FPGA pin 9/10 | LVDS clock |
| SCLK | NET_ADC_SPI_SCK | MCU SPI1 | Config clock |
| SDIO | NET_ADC_SPI_DIO | MCU SPI1 | Config data (bidirectional) |
| CSB | NET_ADC_CS | MCU GPIO | Chip select |
| OTR | NET_ADC_OTR | MCU GPIO | Out-of-range flag |
| PDW | NET_ADC_PD | MCU GPIO | Power-down |

## 4. Netlists

### 4.1 Power Netlist

```
NET_VSRC          J3.2  U7.VIN    U13.V_IN    C1.1  C2.1
NET_VDD_3V3       U7.VOUT  U1.VDD[7]  U2.VCCIO_0  U2.VCCIO_1  U9.VIN  U10.VCC  U11.VCC  U12.VDD  U19.VCC  C3.1  C4.1  C5.1  C6.1  C7-C14.1  C15-C16.1  C21-C22.1
NET_VDD_1V8       U8.VOUT  U2.VCCIO_2  U3.AVDD  U3.DVDD  C17-C18.1  C19.1  C23.1
NET_VDD_3V3A      U9.VOUT  U1.VDDA  U1.VREF+  U16.VCC  C24.1  C25.1  C33.1
NET_VDD_RF        L3.2  U4.VPOS  U5.VCC  U6.VCC  C34.1  C35.1
NET_GND           U7.GND  U8.GND  U9.GND  U1.VSS[7]  U2.GND  U3.GND  U4.GND  U5.GND  C1-C35.2
NET_VBAT          J3.1  U13.V_BAT  R10.1
```

### 4.2 SPI Bus Netlist

```
NET_SPI1_SCK       U1.PA5   U2.27
NET_SPI1_MISO      U1.PA6   U2.29
NET_SPI1_MOSI      U1.PA7   U2.28
NET_SPI1_CS        U1.PC7   U2.26

NET_SPI2_SCK       U1.PB2   U11.SCK   U19.SCK
NET_SPI2_MOSI      U1.PB3   U11.SDI    U19.DIN
NET_LCD_DC         U1.PB4   U11.DC
NET_LCD_CS         U1.PB5   U11.CS
NET_TOUCH_CS       U1.PC8   U19.CS

NET_SPI3_SCK       U1.PB12  U12.CLK
NET_SPI3_MISO      U1.PB13  U12.DO
NET_SPI3_MOSI      U1.PB14  U12.DI
NET_FLASH_CS       U1.PB15  U12.CS
```

### 4.3 LVDS Netlist (ADC → FPGA)

```
NET_ADC_D0_P       U3.D0+   U2.2
NET_ADC_D0_N       U3.D0-   U2.3
NET_ADC_D1_P       U3.D1+   U2.4
NET_ADC_D1_N       U3.D1-   U2.5
NET_ADC_D2_P       U3.D2+   U2.6
NET_ADC_D2_N       U3.D2-   U2.7
NET_ADC_CLK_P      U3.CLK+  U2.9
NET_ADC_CLK_N      U3.CLK-  U2.10
NET_ADC_IRQ        U3.OTR   U1.PA0
NET_ADC_CS         U3.CSB   U1.PE6
NET_ADC_SPI_SCK    U3.SCLK  U1.PA5   (shared SPI1)
NET_ADC_SPI_DIO    U3.SDIO  U1.PA7   (shared SPI1)
```

### 4.4 USB Netlist

```
NET_USB_DP         U1.PA12  U16.D1+  R3.1
NET_USB_DM         U1.PA11  U16.D1-  R4.1
NET_USB_VBUS       U1.PA9   U16.VBUS
NET_USB_ID         U1.PA10  U16.ID
NET_USB_CC1        U16.CC1  R5.1
NET_USB_CC2        U16.CC2  R6.1
```

## 5. Decoupling Networks

### 5.1 STM32H743 Decoupling

Each VDD/VSS pair requires a 100 nF capacitor placed within 2 mm of the pins, plus a 10 µF bulk capacitor per power group.

| Group | Pins | 100 nF | 10 µF | Notes |
|---|---|---|---|---|
| VDD group 1 | 15, 16 | C7 | C5 | Port A power |
| VDD group 2 | 38, 39 | C8 | C5 | Port B power |
| VDD group 3 | 56, 57 | C9 | C6 | Port C power |
| VDD group 4 | 72, 73 | C10 | C6 | Port D power |
| VDD group 5 | 95, 96 | C11 | C6 | Port E power |
| VDD group 6 | 118, 119 | C12 | C5 | USB power |
| VDD group 7 | 139, 140 | C13 | C5 | Core power |
| VDDA | 22, 23 | C14 | — | Via LC filter from VDD_3V3 |
| VCAP1 | 30 | C15 (2.2 µF) | — | Internal regulator |
| VCAP2 | 61 | C16 (2.2 µF) | — | Internal regulator |

### 5.2 FPGA Decoupling

| Rail | Pins | 100 nF | 10 µF | Notes |
|---|---|---|---|---|
| VCC (core 1.2V) | 11 | C17 | C19 | Internal core power |
| VCCIO_0 (3.3V) | 30 | C18 | C5 (shared) | Bank 0 I/O power |
| VCCIO_1 (3.3V) | 48 | C20 | C5 (shared) | Bank 1 I/O power |
| VCCIO_2 (1.8V) | 1 | C21 | C23 | Bank 2 LVDS power |

### 5.3 ADC Decoupling

| Rail | Pins | 100 nF | 10 µF | Notes |
|---|---|---|---|---|
| AVDD (1.8V) | 7, 30 | C22 | C19 | Analog power |
| DVDD (1.8V) | 45, 55 | C23 | — | Digital power |
| VREF (1.8V) | 25 | C24 (1 µF) | — | Internal reference bypass |

### 5.4 PLL Decoupling

| Rail | Pins | 100 nF | Notes |
|---|---|---|---|
| VCC (3.3V) | 3, 6, 10, 13 | C25 | Main power |
| VP (3.3V) | 7, 11 | C26 | VCO power |
| VREG (3.3V) | 15 | C27 (10 nF) | Regulator bypass |

### 5.5 Power Supply Decoupling

| Component | Output | 100 µF | 10 µF | 100 nF | Notes |
|---|---|---|---|---|---|
| TPS62A02 | VDD_3V3 | C1-C4 | C5-C6 | C7-C14 | 4×100µF ceramic + distributed 100nF |
| TLV75518 | VDD_1V8 | — | C23 | C17-C18 | 10µF + 100nF per bank |
| TLV75533 | VDD_3V3A | — | C24 | C14 | LC filtered 3.3V analog |

## 6. ADF4351 PLL Configuration

### 6.1 Register Map (6 registers, 32-bit each, MSB first)

```
Register 0: [R/W][1][0][0][AUTOCAL][CP GAIN][R_DIV2][R_DIV1][...][N_DIV][...][MUXOUT][...][PD1][PD2][CP THREE][CP MODE][LDP][ABP][BDIV][R DIV2][MUXOUT]
Register 1: [R/W][1][0][0][...][N DIV][...][A COUNTER][B COUNTER][CP GAIN][...]
Register 2: [R/W][1][0][0][...][R COUNTER][...][LD PIN MODE][...][CP THREE][...]
Register 3: [R/W][1][0][0][...][CSR][DIV SEL][...][CLK DIV MODE][...][AUX SELECT][AUX ENABLE][B SELECT][...]
Register 4: [R/W][1][0][0][...][RF DIV][...][FB SELECT][AUX SELECT][AUX ENABLE][B SELECT][VCO POWER][...][MUX LOGIC][CP THREE][CP MODE][...]
Register 5: [R/W][1][0][0][...][LD PIN MODE][...]
```

### 6.2 Example Configuration (1 GHz LO)

```
R0 = 0x00400005  // N=5, AUTOCAL=1, MUXOUT=DLD
R1 = 0x08008011  // FRAC=0, INT=8, MOD=2
R2 = 0x00008042  // R=2, CP=2.1mA, DB=1
R3 = 0x00000003  // CSR=0, DIV=1, CLK=OFF
R4 = 0x0040003C  // RF DIV=1, FB=1, VCO=ON, AUX=OFF
R5 = 0x00400005  // LD=Digital Lock Detect
```

Write order: R5 → R4 → R3 → R2 → R1 → R0 (MSB first, 32 clocks per register)

## 7. Schematic Symbol Cross-References

| Symbol | Library | Footprint | Datasheet |
|---|---|---|---|
| STM32H743ZIT6 | MCU_ST_STM32H7 | LQFP-144 | st.com STM32H743 |
| iCE40UP5K-SG48 | FPGA_Lattice | QFN-48 | lattice.com iCE40UP |
| AD9645BCZ-80 | ADC_AD | LQFP-64 | analog.com AD9645 |
| ADL5801ACPZ | Mixer_AD | LFCSP-24 | analog.com ADL5801 |
| ADF4351BCPZ | PLL_AD | LFCSP-32 | analog.com ADF4351 |
| SPF5189Z | RF_Qorvo | SOT-89 | qorvo.com SPF5189Z |
| TPS62A02VLSR | Regulator_TI | VSON-8 | ti.com TPS62A02 |
| TLV75518PDBV | Regulator_TI | SOT-23-5 | ti.com TLV755 |
| nRF52832-MK | BLE_Nordic | Module-48 | nordicsemi.com nRF52832 |
| ILI9341 TFT | Display | Module-2.8inch | ilitek.com ILI9341 |
| W25Q256JVEQ | Flash_Winbond | SOP-8 | winbond.com W25Q256 |
| MCP73871T-2CCI | Charger_Microchip | MSOP-10 | microchip.com MCP73871 |
| TA1099EC8110 | Filter_Murata | SMD-3x3 | murata.com SAW |
| ADF5002BCPZ | Prescaler_AD | LFCSP-8 | analog.com ADF5002 |
| OPA364AIDBVT | OpAmp_TI | SOT-23-5 | ti.com OPA364 |
| TPD4E05U06DQAR | ESD_TI | USON-10 | ti.com TPD4E05U06 |
| TXB0108PWR | LevelShift_TI | TSSOP-20 | ti.com TXB0108 |
| XPT2046TS | Touch | TSSOP-16 | ti.com XPT2046 |