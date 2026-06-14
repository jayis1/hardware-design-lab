# HexaScope — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Description | Qty | Unit Price ($) | Extended ($) |
|---|---|---|---|---|---|
| U1 | LFE5U-45F-8BG381C | Lattice ECP5-5G FPGA, 45K LUT, 381-BGA | 1 | 18.50 | 18.50 |
| U2 | STM32G474VET6 | ST MCU, Cortex-M4F, 512KB Flash, 100-LQFP | 1 | 5.20 | 5.20 |
| U3–U6 | ADS61B49IRSAT | TI 8-bit 250MS/s ADC, LVDS, QFN-32 | 4 | 8.80 | 35.20 |
| U7–U8 | LT1715IS8#TR | AD 200MS/s comparator, SO-8 | 2 | 3.10 | 6.20 |
| U9 | MT41K256M8TW-107:XIT | Micron 256MB DDR3L, 96-BGA | 1 | 4.50 | 4.50 |
| U10 | USB3340-UT | Microchip USB 3.0 ULPI PHY, QFN-32 | 1 | 2.80 | 2.80 |
| U11 | DA9063-NA1V | Dialog PMIC, 6-output, QFN-56 | 1 | 3.40 | 3.40 |
| U12 | FUSB302MPX | FUSB302 USB PD controller, QFN-14 | 1 | 1.20 | 1.20 |
| U13 | ESP32-C6-WROOM-1 | Espressif Wi-Fi 6 module, M.2 | 1 | 2.80 | 2.80 |
| U14 | ISO7740DWR | TI 4-ch digital isolator, 5kV, SO-16 | 1 | 1.90 | 1.90 |
| U15 | DAC60508DGS | TI 8-ch 12-bit DAC, TSSOP-16 | 1 | 1.80 | 1.80 |
| U16 | W25Q256JVEIQ | Winbond 256Mb SPI flash, SO-8 | 1 | 0.90 | 0.90 |
| U17 | TLV62569DDC | TI 5V buck, 2A, SOT-23-6 | 1 | 0.50 | 0.50 |
| U18 | TPS7A7001DSB | TI LDO 3.3V, 1A, SOT-223 | 1 | 0.80 | 0.80 |
| U19 | LMH6551MM/NOPB | TI diff ADC driver, VSSOP-8 | 4 | 2.40 | 9.60 |
| U20 | THS4531IDBR | TI diff amp, VSSOP-8 | 4 | 1.80 | 7.20 |
| U21 | OPA365AIDBVR | TI precision op-amp, SOT-23-5 | 4 | 0.90 | 3.60 |
| U22 | SN74LVC1G125DCK | TI tri-state buffer, SC-70 | 8 | 0.10 | 0.80 |
| U23 | ESD7004DQAR | TI 4-ch ESD protection, QFN-8 | 2 | 0.40 | 0.80 |
| Y1 | ASE-25.000-XTAL | 25 MHz crystal, 20 ppm | 1 | 0.40 | 0.40 |
| Y2 | ASE-48.000-XTAL | 48 MHz crystal, 30 ppm | 1 | 0.50 | 0.50 |
| Y3 | 32.768KHz-ABS07 | 32.768 kHz RTC crystal | 1 | 0.30 | 0.30 |
| — | Passive misc | Resistors, caps, inductors | ~200 | — | 8.00 |
| — | Connectors | 4× BNC, 2× MCX, 1× USB-C, 1× SMA | 8 | — | 6.00 |
| — | PCB | 6-layer, 160×100mm, ENIG | 1 | — | 8.00 |
| | | **TOTAL** | | | **~$124.00** |

---

## 2. Detailed Pinout Tables

### 2.1 Lattice ECP5 FPGA (U1) — LFE5U-45F-8BG381C

Key pin assignments (381-BGA, 0.8mm pitch):

| Pin | Bank | Function | Net | Notes |
|---|---|---|---|---|
| A3 | 0 | VCCIO0 | 1V8 | Bank 0 IO supply |
| B4 | 0 | IO_L1A | ADC1_CLK_P | LVDS clock+ from ADC1 |
| B5 | 0 | IO_L1B | ADC1_CLK_N | LVDS clock- from ADC1 |
| C6 | 0 | IO_L2A | ADC1_D0_P | LVDS data bit 0+ |
| C7 | 0 | IO_L2B | ADC1_D0_N | LVDS data bit 0- |
| D8 | 0 | IO_L3A | ADC1_D1_P | LVDS data bit 1+ |
| D9 | 0 | IO_L3B | ADC1_D1_N | LVDS data bit 1- |
| ... | 0 | IO_L4–L11 | ADC1_D2_P..ADC1_D7_N | LVDS data bits 2–7 |
| E12 | 0 | IO_L12A | ADC2_CLK_P | LVDS clock+ from ADC2 |
| ... | 0 | IO_L13–L21 | ADC2_D0..ADC2_D7 | ADC2 data |
| A14 | 1 | VCCIO1 | 1V8 | Bank 1 IO supply |
| B15 | 1 | IO_L1A | ADC3_CLK_P | LVDS clock+ from ADC3 |
| ... | 1 | IO_L2–L10 | ADC3_D0..ADC3_D7 | ADC3 data |
| C16 | 1 | IO_L11A | ADC4_CLK_P | LVDS clock+ from ADC4 |
| ... | 1 | IO_L12–L20 | ADC4_D0..ADC4_D7 | ADC4 data |
| D21 | 2 | VCCIO2 | 1V8 | Bank 2 IO supply |
| E22 | 2 | IO_L1 | CMP1_OUT | Digital comparator 1 output |
| E23 | 2 | IO_L2 | CMP2_OUT | Digital comparator 2 output |
| F24 | 2 | IO_L3 | EXT_TRIG_IN | External trigger input |
| F25 | 2 | IO_L4 | EXT_TRIG_OUT | External trigger output |
| G10 | 3 | VCCIO3 | 3V3 | Bank 3 IO supply |
| H11 | 3 | IO_L1 | ULPI_DATA0 | USB ULPI data[0] |
| H12 | 3 | IO_L2 | ULPI_DATA1 | USB ULPI data[1] |
| ... | 3 | IO_L3–L6 | ULPI_DATA2..DATA5 | USB ULPI data[2:5] |
| J13 | 3 | IO_L7 | ULPI_DATA6 | USB ULPI data[6] |
| J14 | 3 | IO_L8 | ULPI_DATA7 | USB ULPI data[7] |
| K15 | 3 | IO_L9 | ULPI_CLK | USB ULPI clock (60 MHz) |
| K16 | 3 | IO_L10 | ULPI_DIR | USB ULPI direction |
| L17 | 3 | IO_L11 | ULPI_NXT | USB ULPI next |
| L18 | 3 | IO_L12 | ULPI_STP | USB ULPI stop |
| M19 | 3 | IO_L13 | ULPI_RST | USB ULPI reset |
| N20 | 3 | IO_L14 | WIFI_TX | ESP32-C6 UART TX |
| N21 | 3 | IO_L15 | WIFI_RX | ESP32-C6 UART RX |
| N22 | 3 | IO_L16 | WIFI_RTS | ESP32-C6 UART RTS |
| P23 | 3 | IO_L17 | WIFI_CTS | ESP32-C6 UART CTS |
| R12 | 4 | VCCIO4 | 1V8 | Bank 4 IO supply |
| R13 | 4 | IO_L1 | DDR3_A0 | DDR3 address[0] |
| ... | 4 | IO_L2–L14 | DDR3_A1..DDR3_A14 | DDR3 address bus |
| T14 | 4 | IO_L15 | DDR3_BA0 | DDR3 bank address[0] |
| T15 | 4 | IO_L16 | DDR3_BA1 | DDR3 bank address[1] |
| T16 | 4 | IO_L17 | DDR3_BA2 | DDR3 bank address[2] |
| U17 | 4 | IO_L18 | DDR3_CAS_N | DDR3 CAS# |
| U18 | 4 | IO_L19 | DDR3_RAS_N | DDR3 RAS# |
| U19 | 4 | IO_L20 | DDR3_WE_N | DDR3 WE# |
| V20 | 4 | IO_L21 | DDR3_CS_N | DDR3 CS# |
| V21 | 4 | IO_L22 | DDR3_CKE | DDR3 CKE |
| V22 | 4 | IO_L23 | DDR3_CLK_P | DDR3 clock+ |
| V23 | 4 | IO_L24 | DDR3_CLK_N | DDR3 clock- |
| W24 | 4 | IO_L25 | DDR3_DQ0 | DDR3 data[0] |
| ... | 4 | IO_L26–L41 | DDR3_DQ1..DDR3_DQ15 | DDR3 data bus |
| AA25 | 4 | IO_L42 | DDR3_DQS0_P | DDR3 data strobe 0+ |
| AA26 | 4 | IO_L43 | DDR3_DQS0_N | DDR3 data strobe 0- |
| AB25 | 4 | IO_L44 | DDR3_DQS1_P | DDR3 data strobe 1+ |
| AB26 | 4 | IO_L45 | DDR3_DQS1_N | DDR3 data strobe 1- |
| AC1 | 5 | VCCIO5 | 3V3 | Bank 5 IO supply |
| AD2 | 5 | IO_L1 | SPI_FLASH_MOSI | FPGA config SPI MOSI |
| AD3 | 5 | IO_L2 | SPI_FLASH_MISO | FPGA config SPI MISO |
| AD4 | 5 | IO_L3 | SPI_FLASH_SCK | FPGA config SPI clock |
| AD5 | 5 | IO_L4 | SPI_FLASH_CS_N | FPGA config SPI CS# |
| AE6 | 5 | IO_L5 | MCU_SPI_MOSI | STM32 SPI MOSI to FPGA |
| AE7 | 5 | IO_L6 | MCU_SPI_MISO | STM32 SPI MISO from FPGA |
| AE8 | 5 | IO_L7 | MCU_SPI_SCK | STM32 SPI clock to FPGA |
| AE9 | 5 | IO_L8 | MCU_SPI_CS_N | STM32 SPI CS# to FPGA |
| AF10 | 5 | IO_L9 | MCU_I2C_SDA | (routed, unused) |
| AF11 | 5 | IO_L10 | JTAG_TDI | JTAG data in |
| AF12 | 5 | IO_L11 | JTAG_TDO | JTAG data out |
| AF13 | 5 | IO_L12 | JTAG_TCK | JTAG clock |
| AF14 | 5 | IO_L13 | JTAG_TMS | JTAG mode select |
| AF15 | 5 | IO_L14 | MCU_GPIO0 | STM32 GPIO to FPGA |
| AF16 | 5 | IO_L15 | MCU_GPIO1 | STM32 GPIO to FPGA |
| — | — | VCC_CORE | 1V1 | FPGA core supply (multiple balls) |
| — | — | GND | GND | Ground (multiple balls) |

### 2.2 STM32G474 MCU (U2) — STM32G474VET6

| Pin | Port | Function | Net | Notes |
|---|---|---|---|---|
| 1 | — | VBAT | 3V3_RTC | RTC backup supply |
| 2 | PC13 | GPIO | BTN_RUN_STOP | Run/Stop button |
| 3 | PC14 | GPIO | BTN_SINGLE | Single capture button |
| 4 | PC15 | GPIO | BTN_AUTO | Auto button |
| 5 | PH0 | OSC_IN | XTAL_25M_IN | 25 MHz crystal |
| 6 | PH1 | OSC_OUT | XTAL_25M_OUT | 25 MHz crystal |
| 7 | NRST | RESET | MCU_RST_N | System reset |
| 8 | PC0 | ADC1_IN1 | ANA_SENSE | Analog supply monitor |
| 9 | PC1 | ADC1_IN2 | TEMP_SENSE | Board temperature |
| 10 | PC2 | ADC1_IN3 | VBUS_SENSE | USB VBUS voltage |
| 11 | PC3 | GPIO | LED_RUN | Run LED (active low) |
| 12 | PA0 | GPIO | LED_TRIG | Trigger LED |
| 13 | PA1 | GPIO | LED_USB | USB active LED |
| 14 | PA2 | GPIO | LED_PWR | Power LED |
| 15 | PA3 | TIM2_CH1 | WIFI_EN | ESP32-C6 enable |
| 16 | PA4 | SPI1_NSS | SPI_FLASH_CS_N | SPI flash chip select |
| 17 | PA5 | SPI1_SCK | SPI_FLASH_SCK | SPI flash clock |
| 18 | PA6 | SPI1_MISO | SPI_FLASH_MISO | SPI flash data out |
| 19 | PA7 | SPI1_MOSI | SPI_FLASH_MOSI | SPI flash data in |
| 20 | PB0 | GPIO | MCU_GPIO0 | General purpose to FPGA |
| 21 | PB1 | GPIO | MCU_GPIO1 | General purpose to FPGA |
| 22 | PB2 | SPI2_NSS | DAC_CS_N | Threshold DAC chip select |
| 23 | PB3 | SPI2_SCK | DAC_SCK | Threshold DAC clock |
| 24 | PB4 | SPI2_MISO | DAC_MISO | Threshold DAC data out |
| 25 | PB5 | SPI2_MOSI | DAC_MOSI | Threshold DAC data in |
| 26 | PB6 | I2C1_SCL | PMIC_SCL | PMIC I²C clock |
| 27 | PB7 | I2C1_SDA | PMIC_SDA | PMIC I²C data |
| 28 | PB8 | I2C2_SCL | PD_SCL | FUSB302 I²C clock |
| 29 | PB9 | I2C2_SDA | PD_SDA | FUSB302 I²C data |
| 30 | PB10 | USART3_TX | WIFI_RX | ESP32-C6 UART RX |
| 31 | PB11 | USART3_RX | WIFI_TX | ESP32-C6 UART TX |
| 32 | PB12 | GPIO | BTN_FORCE | Force trigger button |
| 33 | PB13 | SPI4_SCK | FPGA_SPI_SCK | FPGA register SPI clock |
| 34 | PB14 | SPI4_MISO | FPGA_SPI_MISO | FPGA register SPI data out |
| 35 | PB15 | SPI4_MOSI | FPGA_SPI_MOSI | FPGA register SPI data in |
| 36 | PC6 | TIM3_CH1 | FPGA_SPI_CS_N | FPGA register SPI CS |
| 37 | PC7 | TIM3_CH2 | MCU_IRQ | FPGA interrupt to MCU |
| 38 | PC8 | TIM3_CH3 | SD_SW | microSD card detect |
| 39 | PC9 | GPIO | SD_CS_N | microSD chip select |
| 40 | PA8 | GPIO | SD_CMD | microSD command (SPI) |
| 41 | PA9 | USART1_TX | DEBUG_TX | Debug UART TX |
| 42 | PA10 | USART1_RX | DEBUG_RX | Debug UART RX |
| 43 | PA11 | USB_DM | USB_D_N | USB 2.0 D- to ISO7740 |
| 44 | PA12 | USB_DP | USB_D_P | USB 2.0 D+ to ISO7740 |
| 45 | PA13 | SWDIO | SWDIO | Debug port |
| 46 | PA14 | SWCLK | SWCLK | Debug port |
| 47 | PA15 | JTDI | JTDI | Debug port |
| 48 | PD0 | GPIO | FPGA_JTAG_TCK | FPGA JTAG clock |
| 49 | PD1 | GPIO | FPGA_JTAG_TMS | FPGA JTAG mode |
| 50 | PD2 | GPIO | FPGA_JTAG_TDI | FPGA JTAG data in |
| 51 | PD3 | GPIO | FPGA_JTAG_TDO | FPGA JTAG data out |
| 52 | PD4 | GPIO | FPGA_PROG_N | FPGA program# |
| 53 | PD5 | GPIO | FPGA_INIT_N | FPGA init# |
| 54 | PD6 | GPIO | FPGA_DONE | FPGA done |
| 55 | PD7 | GPIO | PD_INT_N | FUSB302 interrupt |
| 56 | PD8 | GPIO | PMIC_INT_N | PMIC interrupt |
| 57–63 | — | — | — | Additional GPIO / unused |
| 64 | — | VSS | GND | Ground |
| 65 | — | VDD | 3V3 | MCU supply |
| 66–100 | — | — | — | Power / ground / reserved |

### 2.3 ADS61B49 ADC (U3) — Channel 1 Example

| Pin | Function | Net | Notes |
|---|---|---|---|
| 1 | INP | ADC1_INP | Analog input + (from diff driver) |
| 2 | INN | ADC1_INN | Analog input – (from diff driver) |
| 3 | VCM | ADC1_VCM | Common-mode output (1.4 V) |
| 4 | CLKP | ADC1_CLK_P | LVDS clock+ input (250 MHz) |
| 5 | CLKN | ADC1_CLK_N | LVDS clock– input (250 MHz) |
| 6 | OVR | ADC1_OVR | Over-range flag |
| 7 | PD | ADC1_PD | Power down (active high) |
| 8 | CLKDIV | ADC1_CLKDIV | Clock divide select (0 = no div) |
| 9 | BG | ADC1_BG | Bandgap reference bypass |
| 10 | VREF | ADC1_VREF | Voltage reference (0.5 V) |
| 11–18 | D0P/D0N..D7P/D7N | ADC1_D0_P..ADC1_D7_N | LVDS data outputs |
| 19 | VDD | 1V8_ANA | Analog supply |
| 20 | AVDD | 1V8_ANA | Analog supply |
| 21 | DRVDD | 1V8 | Digital output supply |
| 22 | AGND | GND | Analog ground |
| 23–24 | DGND | GND | Digital ground |
| 25–32 | D0P/D0N..D3P/D3N | ADC1_D0_P..ADC1_D3_N | LVDS data (continued) |

### 2.4 DAC60508 Threshold DAC (U15)

| Pin | Function | Net | Notes |
|---|---|---|---|
| 1 | VDD | 3V3 | Supply |
| 2 | SCL | DAC_SCK | SPI clock from STM32 |
| 3 | SDA | DAC_MOSI | SPI data from STM32 |
| 4 | A0 | GND | Address bit 0 |
| 5 | A1 | GND | Address bit 1 |
| 6 | OUT0 | CMP1_THRESH_P | Comparator 1 threshold + |
| 7 | OUT1 | CMP1_THRESH_N | Comparator 1 threshold – |
| 8 | OUT2 | CMP2_THRESH_P | Comparator 2 threshold + |
| 9 | OUT3 | CMP2_THRESH_N | Comparator 2 threshold – |
| 10 | OUT4 | ANA_CAL_REF | Analog calibration reference |
| 11 | OUT5 | ANA_OFFSET_CH1 | Channel 1 offset adjust |
| 12 | OUT6 | ANA_OFFSET_CH2 | Channel 2 offset adjust |
| 13 | OUT7 | ANA_GAIN_ADJ | Gain adjustment reference |
| 14 | VREF | VREF_2V5 | External 2.5 V reference |
| 15 | GND | GND | Ground |
| 16 | LDAC | MCU_GPIO0 | LDAC (update outputs) |

---

## 3. Netlists

### 3.1 Power Supply Netlist

```
VBUS_RAW ─── FUSB302(VBUS_SENSE) ─── DA9063(VIN)
    │
    ├── DA9063(BUCK1) ─── VDD_CORE (1.1V @ 3A)
    │       ├── U1(VCC_CORE) × 12 balls
    │       ├── C101(100µF), C102(100µF), C103(10µF), C104(10µF)
    │       └── L101(1µH, 6A)
    │
    ├── DA9063(BUCK2) ─── VDD_DDR (1.35V @ 1A)
    │       ├── U9(VDD), U9(VREF), U9(ZQ)
    │       ├── C201(100µF), C202(10µF), C203(10µF)
    │       └── L201(2.2µH, 3A)
    │
    ├── DA9063(BUCK3) ─── VDD_IO18 (1.8V @ 1A)
    │       ├── U1(VCCIO0..VCCIO4), U3–U6(DRVDD), U7–U8(VDD)
    │       ├── C301(47µF), C302(10µF)
    │       └── L301(1µH, 3A)
    │
    ├── DA9063(LDO1) ─── VDD_IO33 (3.3V @ 2A)
    │       ├── U2(VDD), U10(VDD), U13(VDD), U12(VDD), U14(VDD_ISOLATED)
    │       ├── U15(VDD), U16(VDD), U18(IN)
    │       └── C401(47µF), C402(10µF)
    │
    ├── DA9063(LDO2) ─── VDD_ANA (5.0V @ 0.5A)
    │       ├── U19(VDD) × 4, U20(VDD) × 4, U21(VDD) × 4
    │       └── C501(47µF), C502(10µF)
    │
    └── DA9063(LDO3) ─── VDD_RTC (3.3V @ 10mA, always-on)
            ├── U2(VBAT)
            └── C601(10µF)
```

### 3.2 ADC Signal Chain Netlist (Channel 1)

```
BNC_CH1_P ─── R101(100Ω) ─── U21A(OPA365) (+input)
    │                                   │
    └── R102(100Ω) ─── GND             └── U21A output ─── R103(499Ω) ─── U19A(LMH6551) (+input)

BNC_CH1_N ─── R104(100Ω) ─── U21B(OPA365) (+input)
    │                                   │
    └── R105(100Ω) ─── GND             └── U21B output ─── R106(499Ω) ─── U19A(LMH6551) (–input)

U19A(LMH6551) VOCM ─── R107(1kΩ) ─── ADC1_VCM
U19A(LMH6551) OUT+ ─── R108(33Ω) ─── R109(49.9Ω) ─── C111(10pF) ─── GND
                                    └── U3(ADS61B49) INP
U19A(LMH6551) OUT– ─── R110(33Ω) ─── R111(49.9Ω) ─── C112(10pF) ─── GND
                                    └── U3(ADS61B49) INN

Attenuator relay:
K101 (1× path): BNC_CH1 ── R_att1 ── U21A
K102 (10× path): BNC_CH1 ── R_att2(9MΩ) ── R_att3(1MΩ) ── U21A
K103 (100× path): BNC_CH1 ── R_att4(9.9MΩ) ── R_att5(100kΩ) ── U21A
```

### 3.3 USB Netlist

```
USB-C CONN ─── ESD7004(U23A) ─── ISO7740(U14) Side 1
                │                    │
                CC1 ─── FUSB302(U12) CC1
                CC2 ─── FUSB302(U12) CC2
                │
                D_P ─── ISO7740(U14) IN_A1
                D_N ─── ISO7740(U14) IN_A2

ISO7740(U14) Side 2 (isolated side)
                │
                OUT_A1 ─── USB3340(U10) USB_DP
                OUT_A2 ─── USB3340(U10) USB_DM
                OUT_B1 ─── USB3340(U10) USB_SSTXP
                OUT_B2 ─── USB3340(U10) USB_SSTXN
                IN_B3 ─── USB3340(U10) USB_SSRXP
                IN_B4 ─── USB3340(U10) USB_SSRXN

                VCC_ISOLATED ─── USB3340(U10) VDD
                                USB3340(U10) ULPI[0:7] ─── U1(FPGA) ULPI_DATA[0:7]
                                USB3340(U10) ULPI_CLK ─── U1(FPGA) ULPI_CLK
                                USB3340(U10) ULPI_DIR ─── U1(FPGA) ULPI_DIR
                                USB3340(U10) ULPI_NXT ─── U1(FPGA) ULPI_NXT
                                USB3340(U10) ULPI_STP ─── U1(FPGA) ULPI_STP
                                USB3340(U10) ULPI_RST ─── U1(FPGA) ULPI_RST
```

### 3.4 DDR3L Netlist

```
FPGA(U1) ─── DDR3_BUS:
    A[0:14]  ─── U9(MT41K256M8) A[0:14]    (15 address lines)
    BA[0:2]  ─── U9 BA[0:2]                 (3 bank address lines)
    CAS_N    ─── U9 CAS_N
    RAS_N    ─── U9 RAS_N
    WE_N     ─── U9 WE_N
    CS_N     ─── U9 CS_N
    CKE      ─── U9 CKE
    CLK_P    ─── U9 CLK_P                   (400 MHz differential clock)
    CLK_N    ─── U9 CLK_N
    DQ[0:15] ─── U9 DQ[0:15]               (16-bit data bus)
    DQS0_P   ─── U9 LDQS_P                 (data strobe, lower byte)
    DQS0_N   ─── U9 LDQS_N
    DQS1_P   ─── U9 UDQS_P                 (data strobe, upper byte)
    DQS1_N   ─── U9 UDQS_N
    DM[0]    ─── U9 LDM                     (data mask, lower byte)
    DM[1]    ─── U9 UDM                     (data mask, upper byte)
    ODT      ─── U9 ODT                     (on-die termination)
    RESET_N  ─── U9 RESET_N

U9 ZQ ─── R401(240Ω 1%) ─── GND            (ZQ calibration resistor)
U9 VDD ─── VDD_DDR (1.35V)
U9 VREF ─── VDD_DDR_VREF (0.675V, from R421/R422 divider)
```

---

## 4. Decoupling Networks

### 4.1 FPGA Core (1.1V) Decoupling

| Component | Value | Package | Placement | Net |
|---|---|---|---|---|
| C101 | 100µF | 0805 | Near L101 output | VDD_CORE |
| C102 | 100µF | 0805 | Near L101 output | VDD_CORE |
| C103 | 10µF | 0402 | Under FPGA, center | VDD_CORE |
| C104 | 10µF | 0402 | Under FPGA, center | VDD_CORE |
| C105–C112 | 0.1µF | 0201 | Each VCC_CORE ball pair | VDD_CORE |
| C113–C120 | 0.01µF | 0201 | Each VCC_CORE ball pair | VDD_CORE |

### 4.2 ADC Decoupling (per ADC)

| Component | Value | Package | Placement | Net |
|---|---|---|---|---|
| C301 | 10µF | 0402 | Pin 19 (VDD) | 1V8_ANA |
| C302 | 0.1µF | 0201 | Pin 19 (VDD) | 1V8_ANA |
| C303 | 0.01µF | 0201 | Pin 19 (VDD) | 1V8_ANA |
| C304 | 10µF | 0402 | Pin 21 (DRVDD) | 1V8 |
| C305 | 0.1µF | 0201 | Pin 21 (DRVDD) | 1V8 |
| C306 | 0.01µF | 0201 | Pin 21 (DRVDD) | 1V8 |
| C307 | 1µF | 0402 | Pin 9 (BG) | 1V8_ANA |
| C308 | 1µF | 0402 | Pin 10 (VREF) | 1V8_ANA |

### 4.3 DDR3L Decoupling

| Component | Value | Package | Placement | Net |
|---|---|---|---|---|
| C201 | 100µF | 0805 | Near U9 VDD | VDD_DDR |
| C202 | 10µF | 0402 | Near U9 VDD | VDD_DDR |
| C203 | 10µF | 0402 | Near U9 VDD | VDD_DDR |
| C204–C211 | 0.1µF | 0201 | Each VDD ball | VDD_DDR |
| C212 | 0.1µF | 0201 | VREF pin | VDD_DDR_VREF |

---

## 5. Impedance-Matched Pairs

### 5.1 LVDS Pairs (ADC → FPGA)

| Pair | Net+ | Net– | Target Z₀ | Tolerance | Max Skew |
|---|---|---|---|---|---|
| ADC1_CLK | ADC1_CLK_P | ADC1_CLK_N | 100 Ω diff | ±5% | 5 ps |
| ADC1_D0 | ADC1_D0_P | ADC1_D0_N | 100 Ω diff | ±5% | 5 ps |
| ADC1_D1 | ADC1_D1_P | ADC1_D1_N | 100 Ω diff | ±5% | 5 ps |
| ... | ... | ... | ... | ... | ... |
| ADC1_D7 | ADC1_D7_P | ADC1_D7_N | 100 Ω diff | ±5% | 5 ps |
| ADC2_CLK | ADC2_CLK_P | ADC2_CLK_N | 100 Ω diff | ±5% | 5 ps |
| ... | ... | ... | ... | ... | ... |
| ADC4_D7 | ADC4_D7_P | ADC4_D7_N | 100 Ω diff | ±5% | 5 ps |

Total: 36 differential pairs (4 ADCs × 9 pairs each).

### 5.2 DDR3L Pairs

| Pair | Net+ | Net– | Target Z₀ | Tolerance | Max Skew |
|---|---|---|---|---|---|
| DDR_CLK | DDR3_CLK_P | DDR3_CLK_N | 100 Ω diff | ±5% | — |
| DDR_DQS0 | DDR3_DQS0_P | DDR3_DQS0_N | 100 Ω diff | ±5% | 5 ps |
| DDR_DQS1 | DDR3_DQS1_P | DDR3_DQS1_N | 100 Ω diff | ±5% | 5 ps |

### 5.3 USB SuperSpeed Pairs

| Pair | Net+ | Net– | Target Z₀ | Tolerance |
|---|---|---|---|---|
| USB_SSTX | USB_SSTXP | USB_SSTXN | 90 Ω diff | ±5% |
| USB_SSRX | USB_SSRXP | USB_SSRXN | 90 Ω diff | ±5% |

---

## 6. Pull-Up/Pull-Down Values

| Net | Resistor | Value | Purpose |
|---|---|---|---|
| USB_DP | R501 | 1.5 kΩ pull-up to 3V3 | USB 2.0 Full-speed connect |
| FPGA_PROG_N | R502 | 10 kΩ pull-up to 3V3 | Keep FPGA in user mode |
| FPGA_INIT_N | R503 | 4.7 kΩ pull-up to 3V3 | FPGA init open-drain |
| DAC_CS_N | R504 | 10 kΩ pull-up to 3V3 | SPI CS idle high |
| SPI_FLASH_CS_N | R505 | 10 kΩ pull-up to 3V3 | SPI CS idle high |
| PMIC_INT_N | R506 | 10 kΩ pull-up to 3V3 | Interrupt active low |
| PD_INT_N | R507 | 10 kΩ pull-up to 3V3 | PD interrupt active low |
| MCU_RST_N | R508 | 10 kΩ pull-up to 3V3 | MCU reset pull-up |
| WIFI_EN | R509 | 100 kΩ pull-down | ESP32 disabled by default |
| ADC_PD (×4) | R510–513 | 10 kΩ pull-down | ADCs powered down by default |
| DDR_RESET_N | R514 | 10 kΩ pull-up to VDD_DDR | DDR3 reset idle high |
| DDR_CKE | R515 | 10 kΩ pull-down | CKE low at power-up |
| ULPI_RST | R516 | 10 kΩ pull-up to 1V8 | ULPI reset idle high |

---

## 7. Clock Distribution

| Source | Frequency | Destination | Buffer |
|---|---|---|---|
| Y1 (25 MHz crystal) | 25 MHz | STM32G474 (HSE) | — |
| STM32G474 PLL (×8) | 200 MHz | STM32G474 SYSCLK | Internal |
| STM32G474 MCO1 | 48 MHz | USB3340 REFCLK | PA8 alt-fn |
| FPGA PLL (from 25 MHz via MCU SPI) | 250 MHz | ADC1–4 CLK (LVDS) | Internal PLL |
| FPGA PLL | 400 MHz | DDR3L CLK (LVDS) | Internal PLL |
| FPGA PLL | 60 MHz | ULPI CLK domain | Internal PLL |
| Y2 (48 MHz crystal) | 48 MHz | USB3340 XTALIN | Direct |
| Y3 (32.768 kHz) | 32.768 kHz | STM32G474 LSE | — |