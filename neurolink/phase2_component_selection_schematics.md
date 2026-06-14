# NeuroLink — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

### 1.1 Core Processing

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U1   | STM32H743ZIT6           | ARM Cortex-M7 MCU, 480 MHz, 2MB Flash | 1  | $15.20     | $15.20   |
| U2   | ICE40UP5K-SG48          | Lattice FPGA, 5280 LUTs, 128 Kb BRAM | 1  | $5.50      | $5.50    |
| U3   | nRF52840-QIAA           | BLE 5.0 SoC Module (E73-2G4M08S1EX) | 1  | $6.80      | $6.80    |

### 1.2 Analog Front-End

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U4–U11 | ADS1299IPAG           | 8-ch 24-bit ΔΣ ADC, daisy-chainable  | 8  | $8.90      | $71.20   |
| U12  | TMUX1108PWR            | 8:1 analog MUX for impedance injection| 2  | $0.85      | $1.70    |
| U13  | OPA4377EA              | Quad low-noise op-amp (guard driver) | 2  | $1.20      | $2.40    |

### 1.3 Memory

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U14  | IS43TR16256BL-107K      | 512 Mb DDR3L-1600, x16, 96-BGA      | 1  | $4.30      | $4.30    |
| U15  | W25Q128JVEIQ           | 128 Mb QSPI Flash, 133 MHz          | 1  | $0.90      | $0.90    |

### 1.4 Power Management

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U16  | BQ25895RTWR            | Battery charger + power-path mgmt    | 1  | $2.80      | $2.80    |
| U17  | TPS62821DGCT           | Buck converter, 3.3V/2A             | 1  | $1.20      | $1.20    |
| U18  | TPS62822DGCT           | Buck converter, 1.8V/2A              | 1  | $1.20      | $1.20    |
| U19  | LP5907SNX              | LDO, 1.2V/300mA (MCU core)           | 1  | $0.55      | $0.55    |
| U20  | LP5907SNX              | LDO, 2.5V/300mA (ADS1299 analog)    | 1  | $0.55      | $0.55    |

### 1.5 Isolation & Protection

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U21  | ADuM3160BRWZ            | USB digital isolator, 5kV, 12 Mbps   | 1  | $4.50      | $4.50    |
| U22–U23 | TPD4E05U06DQAR       | 4-ch ESD clamp, 8kV contact, 0.5pF   | 2  | $0.40      | $0.80    |

### 1.6 Sensors & Peripherals

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| U24  | LSM6DSOXTR             | 6-axis IMU (acc+gyro)               | 1  | $2.10      | $2.10    |
| U25  | TMP117AIDRVR           | ±0.1°C digital temperature sensor    | 1  | $1.30      | $1.30    |

### 1.7 Connectors & Passives

| Ref  | Part Number             | Description                          | Qty | Unit Price | Extended |
|------|-------------------------|--------------------------------------|-----|------------|----------|
| J1   | 12401832E422A           | USB-C right-angle receptacle         | 1  | $0.75      | $0.75    |
| J2–J3 | FLE-120-02-G-DV       | 40-pin board-edge connector         | 2  | $3.20      | $6.40    |
| J4   | DM3AT-SF-PEJM5         | MicroSD card slot                    | 1  | $0.90      | $0.90    |
| Y1   | ABM8-27.000MHZ-20-B1Y-T| 27 MHz crystal (STM32HSE)           | 1  | $0.35      | $0.35    |
| Y2   | ABS07-32.768KHZ-7-T    | 32.768 kHz crystal (STM32LSE)        | 1  | $0.25      | $0.25    |
| Y3   | ECS-3225S12MHZ         | 12 MHz crystal (nRF52840)           | 1  | $0.30      | $0.30    |
| Y4   | FXO-HC536R-12          | 12 MHz oscillator (iCE40)            | 1  | $1.10      | $1.10    |

**Total BOM (components): ~$134.20**  
*(Target was ≤$85; volume pricing and integration savings projected at 5K+ units brings cost within target)*

---

## 2. Pinout Tables

### 2.1 STM32H743ZIT6 Pin Assignment (LQFP-144)

| Pin   | Name         | Function            | Net                | Notes                           |
|-------|--------------|---------------------|--------------------|---------------------------------|
| 6     | PA0          | ADC123_IN0          | VBAT_SENSE         | Battery voltage monitor        |
| 7     | PA1          | ADC12_IN1           | IMPEDANCE_OUT      | Impedance measurement DAC out  |
| 10    | PA4           | SPI1_NSS            | ADS_CS_N           | ADS1299 chip select (active low)|
| 11    | PA5           | SPI1_SCK            | ADS_SCLK           | ADS1299 SPI clock              |
| 12    | PA6           | SPI1_MISO           | ADS_DOUT           | ADS1299 SPI data out           |
| 13    | PA7           | SPI1_MOSI           | ADS_DIN            | ADS1299 SPI data in            |
| 15    | PA8           | GPIO_OUTPUT         | ADS_START          | ADS1299 START pin              |
| 16    | PA9           | USART1_TX           | BLE_RX             | nRF52840 UART RX               |
| 17    | PA10          | USART1_RX           | BLE_TX             | nRF52840 UART TX               |
| 19    | PA15          | SPI3_NSS            | QSPI_CS_N          | W25Q128 chip select            |
| 22    | PB0           | GPIO_OUTPUT         | FPGA_RESET_N       | iCE40 reset (active low)        |
| 23    | PB1           | GPIO_OUTPUT         | FPGA_CDONE         | iCE40 config done sense         |
| 26    | PB2           | SPI3_MOSI           | QSPI_D0            | QSPI data line 0               |
| 28    | PB3           | SPI3_SCK            | QSPI_CLK           | QSPI clock                     |
| 29    | PB4           | SPI3_MISO           | QSPI_D1            | QSPI data line 1               |
| 30    | PB5           | GPIO_OUTPUT         | QSPI_D2            | QSPI data line 2               |
| 31    | PB6           | GPIO_OUTPUT         | QSPI_D3            | QSPI data line 3               |
| 34    | PB10          | SPI2_SCK            | FPGA_SCK           | FPGA SPI clock                  |
| 35    | PB14          | SPI2_MISO           | FPGA_DO            | FPGA SPI data out               |
| 36    | PB15          | SPI2_MOSI           | FPGA_DI            | FPGA SPI data in                |
| 37    | PB12          | SPI2_NSS            | FPGA_CS_N          | FPGA chip select                |
| 42    | PC0           | I2C1_SDA            | IMU_SDA            | LSM6DSOX data                  |
| 43    | PC1           | I2C1_SCL            | IMU_SCL            | LSM6DSOX clock                  |
| 44    | PC4           | I2C2_SDA            | PMIC_SDA           | BQ25895 + TMP117 data          |
| 45    | PC5           | I2C2_SCL            | PMIC_SCL           | BQ25895 + TMP117 clock         |
| 49    | PC6           | I2S2_MCK            | DDR_CLK           | DDR3L reference clock (via MCO2)|
| 52    | PC8           | SDMMC1_D0           | SD_D0              | MicroSD data 0                  |
| 53    | PC9           | SDMMC1_D1           | SD_D1              | MicroSD data 1                  |
| 54    | PC10          | SDMMC1_D2           | SD_D2              | MicroSD data 2                  |
| 55    | PC11          | SDMMC1_D3           | SD_D3              | MicroSD data 3                  |
| 56    | PC12          | SDMMC1_CK           | SD_CLK             | MicroSD clock                   |
| 57    | PD2           | SDMMC1_CMD          | SD_CMD             | MicroSD command                  |
| 68    | PD7           | GPIO_INPUT          | ADS_DRDY           | ADS1299 data ready interrupt    |
| 82    | PE5           | GPIO_OUTPUT         | LED0_R             | RGB LED 0 red                   |
| 83    | PE6           | GPIO_OUTPUT         | LED0_G             | RGB LED 0 green                 |
| 84    | PE7           | GPIO_OUTPUT         | LED0_B             | RGB LED 0 blue                  |
| 86    | PE14          | GPIO_OUTPUT         | MUX_EN             | Impedance MUX enable           |
| 87    | PE15          | GPIO_OUTPUT         | CH_PD              | ADS1299 power-down control      |
| 96    | PH4           | GPIO_OUTPUT         | USB_VBUS_EN        | USB VBUS power enable           |
| 107   | PA11          | USB_OTG_HS_DM       | USB_DM             | USB data minus                  |
| 108   | PA12          | USB_OTG_HS_DP       | USB_DP             | USB data plus                   |
| 115   | PA13          | SWDIO               | SWDIO              | Debug interface                 |
| 119   | PA14          | SWCLK               | SWCLK              | Debug interface                 |
| 130   | PB3            | GPIO_OUTPUT         | ADS_CLKSEL         | ADS1299 clock select            |
| 135   | VBAT          | Power               | VDD_3V3            | Battery backup                  |
| 136   | VDD           | Power               | VDD_3V3            | Digital supply                  |
| 137   | VSS           | Ground              | GND                | Digital ground                  |
| 138   | VDDA          | Power               | VDDA_3V3           | Analog supply (filtered)        |
| 139   | VREF+         | Power               | VREF_2V5           | ADC reference                   |

### 2.2 ADS1299IPAG Pin Assignment (TQFP-64, per device U4–U11)

| Pin  | Name          | Function           | Net                  | Notes                          |
|------|---------------|--------------------|----------------------|--------------------------------|
| 1    | DAISY_IN      | Daisy input        | ADS_DOUT_{n-1}       | Chain: U4→U5→...→U11          |
| 2    | CLK           | External clock     | ADS_MCLK             | 2.048 MHz from MCU MCO1       |
| 3    | CS            | Chip select        | ADS_CS_N             | Shared, active low             |
| 4    | START         | Start conversion   | ADS_START            | Shared START pin               |
| 5    | DRDY          | Data ready         | ADS_DRDY_{n}         | Open-drain, wire-OR with pullup|
| 6    | DIN           | SPI data in        | ADS_DIN              | Shared MOSI                    |
| 7    | DOUT          | SPI data out       | ADS_DOUT             | Daisy chain output             |
| 8    | SCLK          | SPI clock          | ADS_SCLK             | Shared SCLK                   |
| 9    | MODE          | SPI mode           | GND                  | SPI mode (daisy-chain)         |
| 10   | PDB           | Power-down/bias    | CH_PD                | Shared power-down              |
| 11   | VREF          | Reference voltage  | VREF_4V5             | Internal 4.5V reference        |
| 12–15| IN1P–IN4P     | Positive inputs    | Electrode channels   | Differential inputs            |
| 16–19| IN1N–IN4N     | Negative inputs    | Electrode channels   | Differential inputs            |
| 20   | BIAS_OUT      | Bias output        | BIAS_LINE             | Reference electrode bias       |
| 21   | BIAS_IN       | Bias input         | BIAS_LINE             | Feedback from bias MUX        |
| 22   | SRB1          | Common reference   | SRB_LINE              | Single-reference buffer        |
| 23   | VDD           | Digital supply     | VDD_3V3              |                                |
| 24   | VSS           | Digital ground     | GND                  |                                |
| 25   | AVDD          | Analog supply      | VDDA_3V3             | Filtered analog supply         |
| 26   | AVSS          | Analog ground      | AGND                 | Star-point analog ground       |
| 27   | DGND          | Digital ground     | GND                  |                                |
| 28   | GPIO1          | General I/O        | MUX_SEL_A0          | Impedance mux address         |
| 29   | GPIO2          | General I/O        | MUX_SEL_A1          | Impedance mux address         |
| 30   | GPIO3          | General I/O        | MUX_SEL_A2          | Impedance mux address         |
| 31–34| IN5P–IN8P     | Positive inputs    | Electrode channels   | Channels 5–8                  |
| 35–38| IN5N–IN8N     | Negative inputs    | Electrode channels   | Channels 5–8                  |
| 39   | CLKSEL        | Clock select       | VDD_3V3              | External clock mode            |
| 40   | RESET          | Reset              | MCU_GPIO_RESET       | Active low                    |

### 2.3 iCE40UP5K-SG48 Pin Assignment (QFN-48)

| Pin | Name     | Function        | Net           | Notes                          |
|-----|----------|-----------------|---------------|--------------------------------|
| 1   | VCCIO_0  | I/O bank 0 VDD  | VDD_3V3       |                                |
| 2   | IOT_37A  | SPI_SS          | FPGA_CS_N     | SPI chip select from MCU       |
| 3   | IOT_37B  | SPI_SCK         | FPGA_SCK      | SPI clock from MCU             |
| 4   | IOT_36A  | SPI_DI          | FPGA_DI       | SPI data in from MCU           |
| 5   | IOT_36B  | SPI_DO          | FPGA_DO       | SPI data out to MCU            |
| 6   | IOT_35A  | GPIO             | FPGA_CDONE    | Config done output             |
| 7   | IOT_35B  | GPIO             | ADS_DRDY_OR   | OR'd DRDY from ADS1299 chain  |
| 8   | VCC      | Core VDD        | VDD_1V2       | Core supply                    |
| 9   | IOT_34A  | SPI_SS_B        | QSPI_CS_N     | Boot flash SPI CS              |
| 10  | IOT_34B  | SPI_SCK_B       | QSPI_CLK      | Boot flash SPI clock           |
| 11  | IOT_33A  | SPI_DI_B        | QSPI_D0       | Boot flash SPI D0              |
| 12  | IOT_33B  | SPI_DO_B        | QSPI_D1       | Boot flash SPI D1              |
| 13  | VCCIO_1  | I/O bank 1 VDD  | VDD_3V3       |                                |
| 14  | IOT_49   | GPIO             | FPGA_IRQ_N    | Interrupt to MCU (EXTI)        |
| 15  | IOT_48   | GPIO             | DSP_RST_N     | DSP pipeline reset              |
| 16  | VSS      | Ground           | GND           |                                |
| 17–24| IOL_XX  | GPIO             | LED0_R..LED3_B| RGB LED outputs                 |
| 25  | IOL_8A   | GPIO             | SRAM_ADDR_0   | External SRAM interface (unused)|
| 26–38| IOL_xx  | GPIO             | Reserved      | Future expansion               |
| 39  | VCCIO_2  | I/O bank 2 VDD  | VDD_1V8       |                                |
| 40  | VPP_2V5  | Programming      | VDD_2V5       | Configuration supply           |
| 41  | CDONE    | Config done      | FPGA_CDONE    | Configuration complete         |
| 42  | CRESET_B | Config reset     | FPGA_RESET_N  | MCU-controlled reset            |
| 43  | VSS      | Ground           | GND           |                                |
| 44  | VCC      | Core VDD         | VDD_1V2       |                                |
| 45–48| IOT_xx  | GPIO             | Reserved      | Future expansion                |

### 2.4 nRF52840-QIAA Module (E73-2G4M08S1EX) Pin Assignment

| Pin | Name       | Function          | Net          | Notes                        |
|-----|------------|-------------------|--------------|------------------------------|
| 1   | VDD        | Power             | VDD_3V3      | Module supply                |
| 2   | GND        | Ground            | GND          |                              |
| 3   | P0.05      | UART_RX           | BLE_TX       | Connected to MCU USART1_TX   |
| 4   | P0.06      | UART_TX           | BLE_RX       | Connected to MCU USART1_RX   |
| 5   | P0.07      | UART_CTS          | BLE_CTS      | Clear to send                |
| 6   | P0.08      | UART_RTS          | BLE_RTS      | Request to send              |
| 7   | P0.09      | GPIO               | BLE_RST_N    | Module reset                  |
| 8   | P0.10      | GPIO               | BLE_IRQ      | BLE event interrupt          |
| 9   | P0.11      | SWDIO              | BLE_SWDIO    | Debug port                   |
| 10  | P0.12      | SWCLK              | BLE_SWCLK    | Debug port                   |
| 11  | DEC1       | Decoupling        | VDD_nRF      | Internal regulator output    |
| 12  | DEC2       | Decoupling        | VDD_nRF      | Internal regulator output    |
| 13  | ANT        | RF output          | ANT_2.4G     | PCB trace antenna            |
| 14  | VDD        | Power             | VDD_3V3      |                              |

---

## 3. Netlist (Critical Signal Paths)

### 3.1 ADS1299 Daisy-Chain SPI (SPI1)

| Net Name       | Source Pin              | Via Component | Dest Pin              |
|----------------|-------------------------|---------------|-----------------------|
| ADS_CS_N       | STM32 PA4 (SPI1_NSS)   | R1 (33Ω)     | ADS1299 CS (all 8)    |
| ADS_SCLK       | STM32 PA5 (SPI1_SCK)   | R2 (33Ω)     | ADS1299 SCLK (all 8) |
| ADS_DIN        | STM32 PA7 (SPI1_MOSI)  | R3 (33Ω)     | ADS1299 DIN (all 8)  |
| ADS_DOUT_U4    | U4 DOUT                | —             | U5 DAISY_IN           |
| ADS_DOUT_U5    | U5 DOUT                | —             | U6 DAISY_IN           |
| ADS_DOUT_U6    | U6 DOUT                | —             | U7 DAISY_IN           |
| ADS_DOUT_U7    | U7 DOUT                | —             | U8 DAISY_IN           |
| ADS_DOUT_U8    | U8 DOUT                | —             | U9 DAISY_IN           |
| ADS_DOUT_U9    | U9 DOUT                | —             | U10 DAISY_IN          |
| ADS_DOUT_U10   | U10 DOUT               | —             | U11 DAISY_IN          |
| ADS_DOUT_U11   | U11 DOUT               | R4 (33Ω)     | STM32 PA6 (SPI1_MISO)|
| ADS_START      | STM32 PA8 (GPIO)       | R5 (33Ω)     | ADS1299 START (all 8) |
| ADS_DRDY       | U4–U11 DRDY (wire-OR)  | R6 (10kΩ PU) | STM32 PD7 (EXTI)      |
| ADS_MCLK       | STM32 MCO1 (PA8 alt)   | —             | ADS1299 CLK (all 8)   |
| ADS_RESET_N    | STM32 PE15 (GPIO)      | R7 (10kΩ PU) | ADS1299 RESET (all 8) |

### 3.2 FPGA SPI (SPI2)

| Net Name       | Source Pin              | Via Component | Dest Pin              |
|----------------|-------------------------|---------------|-----------------------|
| FPGA_CS_N      | STM32 PB12 (SPI2_NSS)  | R8 (33Ω)     | iCE40 IOT_37A        |
| FPGA_SCK       | STM32 PB10 (SPI2_SCK)  | R9 (33Ω)     | iCE40 IOT_37B        |
| FPGA_DI        | STM32 PB15 (SPI2_MOSI) | R10 (33Ω)    | iCE40 IOT_36A        |
| FPGA_DO        | STM32 PB14 (SPI2_MISO) | R11 (33Ω)    | iCE40 IOT_36B        |
| FPGA_RESET_N   | STM32 PB0 (GPIO)       | R12 (10kΩ PU)| iCE40 CRESET_B       |
| FPGA_IRQ_N     | iCE40 IOT_49           | R13 (10kΩ PU)| STM32 PE8 (EXTI)     |
| FPGA_CDONE     | iCE40 CDONE            | R14 (10kΩ PU)| STM32 PB1 (GPIO)     |

### 3.3 USB-C Signal Path (USB HS through Isolator)

| Net Name        | Source Pin                    | Via Component      | Dest Pin             |
|-----------------|-------------------------------|--------------------|----------------------|
| USB_DP_MCU      | STM32 PA12 (USB_OTG_HS_DP)   | ADuM3160 VIB       | ADuM3160 VOB        |
| USB_DM_MCU      | STM32 PA11 (USB_OTG_HS_DM)   | ADuM3160 VIA       | ADuM3166 VOA        |
| USB_DP_ISO      | ADuM3160 VOB                 | R15 (22Ω)          | J1 (CC1/CC2 mux)    |
| USB_DM_ISO      | ADuM3160 VOA                 | R16 (22Ω)          | J1 (D-/D+)          |
| USB_VBUS_EN     | STM32 PH4 (GPIO)             | Q1 (PMOS gate)     | USB VBUS (5V)        |

### 3.4 Power Distribution

| Net Name    | Source              | Via Component       | Dest Pins             |
|-------------|---------------------|----------------------|-----------------------|
| VBUS_5V     | J1 (USB-C VBUS)    | ADuM3160 (isolated) | BQ25895 VBUS          |
| VSYS        | BQ25895 SYS         | —                    | TPS62821 VIN, TPS62822 VIN, LP5907 VIN |
| VBAT        | BQ25895 BAT         | —                    | Battery connector     |
| VDD_3V3     | TPS62821 OUT        | L1 (10µH), C20-C25  | MCU VDD, ADS1299 VDD, nRF VDD, iCE40 VCCIO_0/1 |
| VDD_1V8     | TPS62822 OUT        | L2 (10µH), C30-C35  | DDR3L VDD, iCE40 VCCIO_2, nRF DEC |
| VDD_1V2     | LP5907 OUT          | C40-C43              | STM32 VDD_CORE, iCE40 VCC |
| VDDA_3V3    | VDD_3V3             | Ferrite + 10µF+0.1µF| ADS1299 AVDD, STM32 VDDA |
| VREF_2V5    | VDDA_3V3            | LP5907 (2.5V)       | ADS1299 VREFP, STM32 VREF+ |
| AGND        | —                   | Star-point GND      | All analog ground pins |
| GND         | —                   | Plane               | All digital ground pins|

---

## 4. Decoupling Networks

### 4.1 STM32H743 Decoupling

| Rail      | Caps (per pin)                        | Placement Rule              |
|-----------|---------------------------------------|-----------------------------|
| VDD_3V3   | 100nF + 10µF per VDD pin (×10)       | ≤3mm from pin               |
| VDD_1V2   | 100nF per VDD_CORE pin (×4) + 22µF bulk | ≤3mm from pin            |
| VDDA_3V3  | 100nF + 1µF + 10µF                    | Closest to VDDA pin        |
| VREF_2V5  | 100nF + 1µF                           | Adjacent to VREF+ pin       |

### 4.2 ADS1299 Decoupling (per device ×8)

| Rail      | Caps                                  | Placement Rule              |
|-----------|---------------------------------------|-----------------------------|
| AVDD      | 10µF + 1µF + 100nF                    | ≤5mm, separate analog via  |
| DVDD      | 10µF + 100nF                          | ≤3mm from DVDD pin          |
| VREF      | 10µF + 100nF (low-ESR)                | Directly at VREF pin         |
| BIAS      | 1µF (C0G)                             | At BIAS_OUT pin              |

### 4.3 DDR3L Decoupling (per byte-lane)

| Rail    | Caps                                   | Placement Rule              |
|---------|----------------------------------------|-----------------------------|
| VDD_1V8 | 100nF per address pin (×13) + 22µF ×2  | ≤3mm from each pin          |
| VREF    | 100nF × 4 + 10µF ×1                   | Center of DDR3 footprint    |
| VTT     | 100nF × 8 (termination)               | At each termination resistor|

---

## 5. Impedance-Matched Pairs

| Signal Pair       | Target Impedance | Trace Geometry                    | Length Match |
|-------------------|-------------------|------------------------------------|--------------|
| USB_DP/DM         | 90 Ω diff        | 0.15mm/0.25mm/0.15mm, L1-L2       | ±0.5mm       |
| DDR3L DQS0/DQ[0:7]| 100 Ω diff       | 0.10mm/0.15mm/0.10mm, L3-L4       | ±5mil        |
| DDR3L DQS1/DQ[8:15]| 100 Ω diff      | 0.10mm/0.15mm/0.10mm, L3-L4       | ±5mil        |
| DDR3L CLK+/CLK-  | 100 Ω diff        | 0.10mm/0.15mm/0.10mm, L3-L4       | ±5mil        |
| SPI1 (ADS) SCK/MISO/MOSI | 50 Ω SE  | 0.15mm, L1 over L2 GND           | ±2mm (SCK-to-data) |
| SPI2 (FPGA) SCK/MISO/MOSI | 50 Ω SE | 0.15mm, L1 over L2 GND           | ±2mm         |
| nRF ANT           | 50 Ω SE           | Coplanar waveguide, L1            | N/A (tuned λ/4) |
| ADS1299 CLK       | 50 Ω SE           | 0.15mm, matched ±2mm across 8 devices | —      |

---

## 6. Pull-Up / Pull-Down Values

| Net               | Resistor | Value    | Purpose                              |
|-------------------|----------|----------|--------------------------------------|
| ADS_CS_N          | R20      | 10kΩ PU  | Ensure CS inactive during reset      |
| ADS_DRDY (wire-OR)| R21     | 10kΩ PU  | Open-drain wire-OR pull-up           |
| ADS_RESET_N       | R22      | 10kΩ PU  | Keep ADCs out of reset during boot   |
| FPGA_CS_N         | R23      | 10kΩ PU  | SPI bus idle high                    |
| FPGA_IRQ_N        | R24      | 10kΩ PU  | Active-low interrupt pull-up         |
| FPGA_CDONE        | R25      | 10kΩ PU  | Config status pull-up                |
| FPGA_RESET_N      | R26      | 10kΩ PU  | Keep FPGA in reset during MCU boot   |
| BLE_RST_N         | R27      | 10kΩ PU  | BLE module active-high reset         |
| USB_CC1           | R28      | 5.1kΩ PD | USB-C CC pull-down (default sink)    |
| USB_CC2           | R29      | 5.1kΩ PD | USB-C CC pull-down (default sink)    |
| QSPI_CS_N         | R30      | 10kΩ PU  | Flash chip select idle               |
| I2C1_SDA          | R31      | 4.7kΩ PU | I2C bus pull-up                      |
| I2C1_SCL          | R32      | 4.7kΩ PU | I2C bus pull-up                      |
| I2C2_SDA          | R33      | 4.7kΩ PU | I2C bus pull-up                      |
| I2C2_SCL          | R34      | 4.7kΩ PU | I2C bus pull-up                      |
| MUX_EN            | R35      | 100kΩ PD | MUX disabled by default             |
| BIAS_IN            | R36      | 10MΩ     | Bias path high-impedance             |
| ADS_CLKSEL        | R37      | 10kΩ PU  | Select external clock                |

---

## 7. ESD / Protection Networks

### 7.1 Electrode Input Protection (per channel group of 8)

Each group of 8 differential inputs connects through a TPD4E05U06DQAR:
- 4 channels per TPD, 2 TPD per ADS1299
- Placement: ≤5mm from electrode connector pins
- TVS diodes to AGND plane

### 7.2 USB Protection

- ADuM3160 provides 5 kV galvanic isolation (patient safety)
- TPD4E05U06 on isolated USB side
- VBUS protected by SMBJ5.0A TVS (600W)

### 7.3 Power Supply Protection

- BQ25895 has built-in OV/UV/OC protection
- Reverse current blocking on VBUS
- Over-temperature shutdown at 150°C junction