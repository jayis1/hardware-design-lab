# Aether-Link: Phase 2 — Component Selection and Schematics

## 1. Bill of Materials (BOM)

### 1.1 Primary Integrated Circuits

| Designator | Part Number              | Manufacturer | Description                          | Qty | Unit Cost | Package          |
|------------|--------------------------|--------------|--------------------------------------|-----|-----------|------------------|
| U1         | XC7K325T-2FFG900C        | Xilinx/AMD   | Kintex-7 FPGA, 326K LUTs, 16 GTX     | 1   | $1,200    | FFG900 (31mm)    |
| U2         | MT40A512M16LY-075:E      | Micron       | DDR4-3200, 8Gb (512Mx16), 1.2V       | 2   | $18       | FBGA-96 (9x14mm) |
| U3         | MT25QU512ABB8E12-0SIT    | Micron       | 512Mb QSPI Flash, 1.8V, SOIC-16      | 1   | $6        | SOIC-16W         |
| U4         | FT232HQ-REEL             | FTDI         | USB 2.0 Hi-Speed to MPSSE/JTAG/UART  | 1   | $4        | QFN-48           |
| U5         | TMP117MAIDRVR            | TI           | I2C Temp Sensor, ±0.1°C, 1.8V        | 2   | $3        | WSON-6           |
| U6         | INA226AIDGSR             | TI           | I2C Power Monitor, 12V bus            | 1   | $2        | VSSOP-10         |
| U7         | EMC2301-1-ACZL-TR        | Microchip    | I2C Fan Controller, 1x PWM            | 1   | $2        | MSOP-8           |
| U8         | SiT9121AI-2C2-33E200.000000 | SiTime   | 200 MHz LVDS Oscillator, ±25ppm       | 1   | $8        | 3225 (3.2x2.5mm) |
| U9         | SiT9365AI-2C2-33E161.132812 | SiTime   | 161.1328125 MHz LVDS, ±200fs jitter   | 1   | $12       | 3225             |
| U10,U11    | SiT9365AI-2C2-33E156.250000 | SiTime   | 156.25 MHz LVDS, ±200fs jitter        | 2   | $12       | 3225             |
| U12        | TPS546D24ARVFR           | TI           | 12V→VCCINT, 40A DC/DC, PMBus          | 1   | $15       | LQFN-40          |
| U13        | TPS546D24ARVFR           | TI           | 12V→VCCBRAM, 40A DC/DC                | 1   | $15       | LQFN-40          |
| U14        | TPS546D24ARVFR           | TI           | 12V→VCCAUX, 40A DC/DC                 | 1   | $15       | LQFN-40          |
| U15        | TPS546D24ARVFR           | TI           | 12V→VCCO, 40A DC/DC                   | 1   | $15       | LQFN-40          |
| U16        | TPS546D24ARVFR           | TI           | 12V→MGTAVCC, 40A DC/DC                | 1   | $15       | LQFN-40          |
| U17        | TPS546D24ARVFR           | TI           | 12V→MGTAVTT, 40A DC/DC                | 1   | $15       | LQFN-40          |
| U18        | TPS7A9201DSKR            | TI           | 12V→3.3V, 2A LDO                      | 1   | $3        | WSON-10          |
| U19        | TPS7A9201DSKR            | TI           | 12V→2.5V, 2A LDO                      | 1   | $3        | WSON-10          |
| U20        | TPS7A9201DSKR            | TI           | 12V→1.8V, 2A LDO                      | 1   | $3        | WSON-10          |
| J1,J2      | 2007637-1                | TE           | QSFP28 Cage, Single, PCIe Bracket     | 2   | $12       | Through-hole     |
| J3         | 10104111-0001LF          | Amphenol     | USB Micro-B Receptacle                | 1   | $1        | SMT              |
| J4         | PCIe x8 Edge Connector   | (PCB Gold)   | 98-pin edge finger, 1.0mm pitch       | 1   | N/A       | PCB Edge         |
| FAN1,FAN2  | AFB0412VHB               | Delta        | 40x40x15mm Axial Fan, 12V, PWM        | 2   | $8        | Wire lead        |

### 1.2 Passive Components Summary

| Type                    | Value / Spec                          | Qty | Package    |
|-------------------------|---------------------------------------|-----|------------|
| MLCC 0402 0.1µF 16V    | GRM155R71C104KA88D (Murata)           | 120 | 0402       |
| MLCC 0402 0.01µF 16V   | GRM155R71C103KA01D                    | 80  | 0402       |
| MLCC 0402 1.0µF 10V    | GRM155R61A105KE15D                    | 40  | 0402       |
| MLCC 0603 10µF 16V     | GRM188R61C106MA73D                    | 30  | 0603       |
| MLCC 0805 22µF 16V     | GRM21BR61C226ME44L                    | 20  | 0805       |
| MLCC 0805 47µF 6.3V    | GRM21BR60J476ME15L                    | 12  | 0805       |
| MLCC 1206 100µF 6.3V   | GRM31CR60J107ME39L                    | 8   | 1206       |
| Tantalum 330µF 6.3V    | T520B337M006ATE025 (Kemet)            | 4   | 3528-21    |
| Tantalum 470µF 2.5V    | T520B477M0025ATE025                   | 4   | 3528-21    |
| Resistor 0402 0Ω       | RC0402JR-070RL (Yageo)                | 20  | 0402       |
| Resistor 0402 4.7kΩ    | RC0402FR-074K7L                       | 30  | 0402       |
| Resistor 0402 10kΩ     | RC0402FR-0710KL                       | 20  | 0402       |
| Resistor 0402 100Ω     | RC0402FR-07100RL                      | 16  | 0402       |
| Resistor 0402 240Ω     | RC0402FR-07240RL                      | 8   | 0402       |
| Resistor 0402 49.9Ω 1% | RC0402FR-0749R9L                      | 8   | 0402       |
| Resistor 0402 1kΩ      | RC0402FR-071KL                        | 10  | 0402       |
| Ferrite Bead 0402      | BLM15PX121SN1D (Murata) 120Ω@100MHz   | 24  | 0402       |
| Ferrite Bead 0603      | BLM18PG121SN1D (Murata) 120Ω@100MHz   | 12  | 0603       |
| Inductor 4.7µH         | DFE252012F-4R7M (Murata) 4.5A Isat    | 6   | 2520       |
| Inductor 1.0µH         | DFE252012F-1R0M 8.5A Isat             | 6   | 2520       |
| Inductor 0.47µH        | DFE252012F-R47M 11A Isat              | 6   | 2520       |

### 1.3 Connectors and Mechanical

| Designator | Part Number         | Description                          | Qty |
|------------|---------------------|--------------------------------------|-----|
| J5         | 22-05-3041           | Molex 4-pin Fan Header, 2.54mm       | 2   |
| J6         | 61300411121          | Würth 4-pin Header, I2C/UART debug   | 1   |
| SW1        | KMR211GLFS           | C&K Tactile Switch, Reset            | 1   |
| LED1-4     | LTST-C195KGJRKT      | Lite-On Bi-Color SMD LED, 0606       | 4   |

## 2. FPGA Pinout — XC7K325T-2FFG900C

### 2.1 Bank Assignment

| Bank | Voltage | Type  | Primary Function                                   |
|------|---------|-------|----------------------------------------------------|
| 12   | 1.5V    | HR    | DDR4 Channel A (DQ[0:15], DQS, DM, ADDR/CMD)       |
| 13   | 1.5V    | HR    | DDR4 Channel A (DQ[16:31], DQS, DM)                |
| 14   | 1.5V    | HR    | DDR4 Channel B (DQ[0:15], DQS, DM, ADDR/CMD)       |
| 15   | 1.5V    | HR    | DDR4 Channel B (DQ[16:31], DQS, DM)                |
| 16   | 1.5V    | HR    | SPI Flash, I2C, UART, GPIO, LEDs, Fan PWM          |
| 17   | 1.5V    | HR    | USB (FT232H), JTAG, Reset, Configuration           |
| 18   | 1.5V    | HR    | QSFP28 Control (I2C, ModPrsL, ResetL, LPMode)      |
| 115  | 1.0V    | GTX   | PCIe Gen4 x8 (MGTXRX[0:7], MGTXTX[0:7])            |
| 116  | 1.0V    | GTX   | 100GbE CMAC #0 (MGTXRX[8:11], MGTXTX[8:11])        |
| 117  | 1.0V    | GTX   | 100GbE CMAC #1 (MGTXRX[12:15], MGTXTX[12:15])      |
| 34   | 1.8V    | HR    | System clock inputs (PCIe refclk, DDR4 refclk)     |
| 35   | 1.8V    | HR    | CMAC refclk, QSFP28 refclks                        |

### 2.2 Detailed Pin Assignments — DDR4 Channel A

| FPGA Pin | Signal Name    | Direction | U2 Pin | Description                    |
|----------|----------------|-----------|--------|--------------------------------|
| L12      | DDR4_A_A0      | Out       | N2     | Address bit 0 (A0)             |
| M12      | DDR4_A_A1      | Out       | N7     | Address bit 1 (A1)             |
| N12      | DDR4_A_A2      | Out       | N3     | Address bit 2 (A2)             |
| P12      | DDR4_A_A3      | Out       | P8     | Address bit 3 (A3)             |
| R12      | DDR4_A_A4      | Out       | P3     | Address bit 4 (A4)             |
| T12      | DDR4_A_A5      | Out       | R8     | Address bit 5 (A5)             |
| U12      | DDR4_A_A6      | Out       | R2     | Address bit 6 (A6)             |
| V12      | DDR4_A_A7      | Out       | R3     | Address bit 7 (A7)             |
| W12      | DDR4_A_A8      | Out       | T8     | Address bit 8 (A8)             |
| Y12      | DDR4_A_A9      | Out       | T3     | Address bit 9 (A9)             |
| AA12     | DDR4_A_A10     | Out       | L7     | Address bit 10 (A10/AP)        |
| AB12     | DDR4_A_A11     | Out       | N1     | Address bit 11 (A11)           |
| AC12     | DDR4_A_A12     | Out       | P7     | Address bit 12 (A12/BC#)       |
| AD12     | DDR4_A_A13     | Out       | P2     | Address bit 13 (A13)            |
| AE12     | DDR4_A_BA0     | Out       | M2     | Bank Address 0                 |
| AF12     | DDR4_A_BA1     | Out       | M3     | Bank Address 1                 |
| AG12     | DDR4_A_BG0     | Out       | L6     | Bank Group 0                   |
| AH12     | DDR4_A_CKE     | Out       | K2     | Clock Enable                   |
| AJ12     | DDR4_A_CS#     | Out       | L3     | Chip Select (active low)       |
| AK12     | DDR4_A_ODT     | Out       | K8     | On-Die Termination             |
| AL12     | DDR4_A_ACT#    | Out       | L5     | Activate (active low)          |
| AM12     | DDR4_A_RAS#    | Out       | J7     | Row Address Strobe             |
| AN12     | DDR4_A_CAS#    | Out       | K3     | Column Address Strobe          |
| AP12     | DDR4_A_WE#     | Out       | K7     | Write Enable                   |
| AR12     | DDR4_A_RESET#  | Out       | G2     | DRAM Reset                     |
| AT12     | DDR4_A_PAR     | Out       | M7     | Command/Address Parity         |
| AU12     | DDR4_A_ALERT#  | In        | J3     | Alert (active low)             |
| AV12     | DDR4_A_CK0_P   | Out       | J1     | Differential Clock +           |
| AW12     | DDR4_A_CK0_N   | Out       | H1     | Differential Clock -           |
| AY12     | DDR4_A_DQ0     | InOut     | A2     | Data bit 0                     |
| BA12     | DDR4_A_DQ1     | InOut     | A3     | Data bit 1                     |
| BB12     | DDR4_A_DQ2     | InOut     | B1     | Data bit 2                     |
| BC12     | DDR4_A_DQ3     | InOut     | B2     | Data bit 3                     |
| BD12     | DDR4_A_DQ4     | InOut     | C1     | Data bit 4                     |
| BE12     | DDR4_A_DQ5     | InOut     | C2     | Data bit 5                     |
| BF12     | DDR4_A_DQ6     | InOut     | C3     | Data bit 6                     |
| BG12     | DDR4_A_DQ7     | InOut     | D2     | Data bit 7                     |
| BH12     | DDR4_A_DQ8     | InOut     | D3     | Data bit 8                     |
| BJ12     | DDR4_A_DQ9     | InOut     | E1     | Data bit 9                     |
| BK12     | DDR4_A_DQ10    | InOut     | E2     | Data bit 10                    |
| BL12     | DDR4_A_DQ11    | InOut     | E3     | Data bit 11                    |
| BM12     | DDR4_A_DQ12    | InOut     | F1     | Data bit 12                    |
| BN12     | DDR4_A_DQ13    | InOut     | F2     | Data bit 13                    |
| BP12     | DDR4_A_DQ14    | InOut     | F3     | Data bit 14                    |
| BR12     | DDR4_A_DQ15    | InOut     | G1     | Data bit 15                    |
| BS12     | DDR4_A_DQS0_P  | InOut     | A7     | Data Strobe 0 +                |
| BT12     | DDR4_A_DQS0_N  | InOut     | A8     | Data Strobe 0 -                |
| BU12     | DDR4_A_DM0     | Out       | D1     | Data Mask 0                    |
| BV12     | DDR4_A_DQ16    | InOut     | A9     | Data bit 16                    |
| BW12     | DDR4_A_DQ17    | InOut     | B9     | Data bit 17                    |
| BX12     | DDR4_A_DQ18    | InOut     | C9     | Data bit 18                    |
| BY12     | DDR4_A_DQ19    | InOut     | D9     | Data bit 19                    |
| BZ12     | DDR4_A_DQ20    | InOut     | E9     | Data bit 20                    |
| CA12     | DDR4_A_DQ21    | InOut     | F9     | Data bit 21                    |
| CB12     | DDR4_A_DQ22    | InOut     | G9     | Data bit 22                    |
| CC12     | DDR4_A_DQ23    | InOut     | H9     | Data bit 23                    |
| CD12     | DDR4_A_DQ24    | InOut     | A10    | Data bit 24                    |
| CE12     | DDR4_A_DQ25    | InOut     | B10    | Data bit 25                    |
| CF12     | DDR4_A_DQ26    | InOut     | C10    | Data bit 26                    |
| CG12     | DDR4_A_DQ27    | InOut     | D10    | Data bit 27                    |
| CH12     | DDR4_A_DQ28    | InOut     | E10    | Data bit 28                    |
| CJ12     | DDR4_A_DQ29    | InOut     | F10    | Data bit 29                    |
| CK12     | DDR4_A_DQ30    | InOut     | G10    | Data bit 30                    |
| CL12     | DDR4_A_DQ31    | InOut     | H10    | Data bit 31                    |
| CM12     | DDR4_A_DQS1_P  | InOut     | A12    | Data Strobe 1 +                |
| CN12     | DDR4_A_DQS1_N  | InOut     | B12    | Data Strobe 1 -                |
| CP12     | DDR4_A_DM1     | Out       | D11    | Data Mask 1                    |

### 2.3 Detailed Pin Assignments — PCIe Gen4 x8

| FPGA Pin | Signal Name    | Direction | PCIe Edge Pin | Description                    |
|----------|----------------|-----------|---------------|--------------------------------|
| MGT115RX0_P | PCIE_RX0_P  | In        | A21 (PERn0)   | Lane 0 Receive +              |
| MGT115RX0_N | PCIE_RX0_N  | In        | A22 (PERp0)   | Lane 0 Receive -              |
| MGT115TX0_P | PCIE_TX0_P  | Out       | B21 (PETn0)   | Lane 0 Transmit +             |
| MGT115TX0_N | PCIE_TX0_N  | Out       | B22 (PETp0)   | Lane 0 Transmit -             |
| MGT115RX1_P | PCIE_RX1_P  | In        | A25 (PERn1)   | Lane 1 Receive +              |
| MGT115RX1_N | PCIE_RX1_N  | In        | A26 (PERp1)   | Lane 1 Receive -              |
| MGT115TX1_P | PCIE_TX1_P  | Out       | B25 (PETn1)   | Lane 1 Transmit +             |
| MGT115TX1_N | PCIE_TX1_N  | Out       | B26 (PETp1)   | Lane 1 Transmit -             |
| MGT115RX2_P | PCIE_RX2_P  | In        | A29 (PERn2)   | Lane 2 Receive +              |
| MGT115RX2_N | PCIE_RX2_N  | In        | A30 (PERp2)   | Lane 2 Receive -              |
| MGT115TX2_P | PCIE_TX2_P  | Out       | B29 (PETn2)   | Lane 2 Transmit +             |
| MGT115TX2_N | PCIE_TX2_N  | Out       | B30 (PETp2)   | Lane 2 Transmit -             |
| MGT115RX3_P | PCIE_RX3_P  | In        | A33 (PERn3)   | Lane 3 Receive +              |
| MGT115RX3_N | PCIE_RX3_N  | In        | A34 (PERp3)   | Lane 3 Receive -              |
| MGT115TX3_P | PCIE_TX3_P  | Out       | B33 (PETn3)   | Lane 3 Transmit +             |
| MGT115TX3_N | PCIE_TX3_N  | Out       | B34 (PETp3)   | Lane 3 Transmit -             |
| MGT115RX4_P | PCIE_RX4_P  | In        | A37 (PERn4)   | Lane 4 Receive +              |
| MGT115RX4_N | PCIE_RX4_N  | In        | A38 (PERp4)   | Lane 4 Receive -              |
| MGT115TX4_P | PCIE_TX4_P  | Out       | B37 (PETn4)   | Lane 4 Transmit +             |
| MGT115TX4_N | PCIE_TX4_N  | Out       | B38 (PETp4)   | Lane 4 Transmit -             |
| MGT115RX5_P | PCIE_RX5_P  | In        | A41 (PERn5)   | Lane 5 Receive +              |
| MGT115RX5_N | PCIE_RX5_N  | In        | A42 (PERp5)   | Lane 5 Receive -              |
| MGT115TX5_P | PCIE_TX5_P  | Out       | B41 (PETn5)   | Lane 5 Transmit +             |
| MGT115TX5_N | PCIE_TX5_N  | Out       | B42 (PETp5)   | Lane 5 Transmit -             |
| MGT115RX6_P | PCIE_RX6_P  | In        | A45 (PERn6)   | Lane 6 Receive +              |
| MGT115RX6_N | PCIE_RX6_N  | In        | A46 (PERp6)   | Lane 6 Receive -              |
| MGT115TX6_P | PCIE_TX6_P  | Out       | B45 (PETn6)   | Lane 6 Transmit +             |
| MGT115TX6_N | PCIE_TX6_N  | Out       | B46 (PETp6)   | Lane 6 Transmit -             |
| MGT115RX7_P | PCIE_RX7_P  | In        | A49 (PERn7)   | Lane 7 Receive +              |
| MGT115RX7_N | PCIE_RX7_N  | In        | A50 (PERp7)   | Lane 7 Receive -              |
| MGT115TX7_P | PCIE_TX7_P  | Out       | B49 (PETn7)   | Lane 7 Transmit +             |
| MGT115TX7_N | PCIE_TX7_N  | Out       | B50 (PETp7)   | Lane 7 Transmit -             |
| AH17        | PCIE_PERST#  | In        | A11 (PERST#)  | PCIe Reset (3.3V aux)         |
| AG17        | PCIE_WAKE#   | Out       | B11 (WAKE#)   | PCIe Wake (3.3V aux)          |
| AF17        | PCIE_CLKREQ# | Out       | B12 (CLKREQ#) | Clock Request (3.3V aux)      |
| AE17        | PCIE_PRSNT1# | Out       | B17 (PRSNT1#) | Present detect 1 (GND)        |
| AD17        | PCIE_PRSNT2# | Out       | B31 (PRSNT2#) | Present detect 2 (GND)        |
| AC17        | PCIE_REFCLK_P| In        | A13 (REFCLK+) | 100 MHz HCSL refclk +         |
| AB17        | PCIE_REFCLK_N| In        | A14 (REFCLK-) | 100 MHz HCSL refclk -         |

### 2.4 Detailed Pin Assignments — 100GbE CMAC #0 (QSFP28 Port 0)

| FPGA Pin     | Signal Name      | Direction | QSFP28 Pin | Description                    |
|--------------|------------------|-----------|------------|--------------------------------|
| MGT116RX0_P  | QSFP0_RX0_P      | In        | 12 (RX1p)  | Lane 0 Receive + (25.78 Gbps) |
| MGT116RX0_N  | QSFP0_RX0_N      | In        | 13 (RX1n)  | Lane 0 Receive -              |
| MGT116TX0_P  | QSFP0_TX0_P      | Out       | 29 (TX1p)  | Lane 0 Transmit +             |
| MGT116TX0_N  | QSFP0_TX0_N      | Out       | 30 (TX1n)  | Lane 0 Transmit -             |
| MGT116RX1_P  | QSFP0_RX1_P      | In        | 15 (RX3p)  | Lane 1 Receive +              |
| MGT116RX1_N  | QSFP0_RX1_N      | In        | 16 (RX3n)  | Lane 1 Receive -              |
| MGT116TX1_P  | QSFP0_TX1_P      | Out       | 32 (TX3p)  | Lane 1 Transmit +             |
| MGT116TX1_N  | QSFP0_TX1_N      | Out       | 33 (TX3n)  | Lane 1 Transmit -             |
| MGT116RX2_P  | QSFP0_RX2_P      | In        | 18 (RX2p)  | Lane 2 Receive +              |
| MGT116RX2_N  | QSFP0_RX2_N      | In        | 19 (RX2n)  | Lane 2 Receive -              |
| MGT116TX2_P  | QSFP0_TX2_P      | Out       | 35 (TX2p)  | Lane 2 Transmit +             |
| MGT116TX2_N  | QSFP0_TX2_N      | Out       | 36 (TX2n)  | Lane 2 Transmit -             |
| MGT116RX3_P  | QSFP0_RX3_P      | In        | 21 (RX4p)  | Lane 3 Receive +              |
| MGT116RX3_N  | QSFP0_RX3_N      | In        | 22 (RX4n)  | Lane 3 Receive -              |
| MGT116TX3_P  | QSFP0_TX3_P      | Out       | 38 (TX4p)  | Lane 3 Transmit +             |
| MGT116TX3_N  | QSFP0_TX3_N      | Out       | 39 (TX4n)  | Lane 3 Transmit -             |
| MGT116REFCLK0_P | QSFP0_REFCLK_P | In     | —          | 156.25 MHz from SiT9365 U10   |
| MGT116REFCLK0_N | QSFP0_REFCLK_N | In     | —          | 156.25 MHz from SiT9365 U10   |
| AK18         | QSFP0_SCL        | Out       | 4 (SCL)    | I2C Clock (3.3V)              |
| AJ18         | QSFP0_SDA        | InOut     | 5 (SDA)    | I2C Data (3.3V)               |
| AH18         | QSFP0_MODPRSL    | In        | 8 (ModPrsL)| Module Present (active low)   |
| AG18         | QSFP0_RESETL     | Out       | 7 (ResetL) | Module Reset (active low)     |
| AF18         | QSFP0_LPMODE     | Out       | 31 (LPMode)| Low Power Mode                |
| AE18         | QSFP0_INTL       | In        | 9 (IntL)   | Module Interrupt (active low) |

### 2.5 Detailed Pin Assignments — 100GbE CMAC #1 (QSFP28 Port 1)

| FPGA Pin     | Signal Name      | Direction | QSFP28 Pin | Description                    |
|--------------|------------------|-----------|------------|--------------------------------|
| MGT117RX0_P  | QSFP1_RX0_P      | In        | 12 (RX1p)  | Lane 0 Receive +              |
| MGT117RX0_N  | QSFP1_RX0_N      | In        | 13 (RX1n)  | Lane 0 Receive -              |
| MGT117TX0_P  | QSFP1_TX0_P      | Out       | 29 (TX1p)  | Lane 0 Transmit +             |
| MGT117TX0_N  | QSFP1_TX0_N      | Out       | 30 (TX1n)  | Lane 0 Transmit -             |
| MGT117RX1_P  | QSFP1_RX1_P      | In        | 15 (RX3p)  | Lane 1 Receive +              |
| MGT117RX1_N  | QSFP1_RX1_N      | In        | 16 (RX3n)  | Lane 1 Receive -              |
| MGT117TX1_P  | QSFP1_TX1_P      | Out       | 32 (TX3p)  | Lane 1 Transmit +             |
| MGT117TX1_N  | QSFP1_TX1_N      | Out       | 33 (TX3n)  | Lane 1 Transmit -             |
| MGT117RX2_P  | QSFP1_RX2_P      | In        | 18 (RX2p)  | Lane 2 Receive +              |
| MGT117RX2_N  | QSFP1_RX2_N      | In        | 19 (RX2n)  | Lane 2 Receive -              |
| MGT117TX2_P  | QSFP1_TX2_P      | Out       | 35 (TX2p)  | Lane 2 Transmit +             |
| MGT117TX2_N  | QSFP1_TX2_N      | Out       | 36 (TX2n)  | Lane 2 Transmit -             |
| MGT117RX3_P  | QSFP1_RX3_P      | In        | 21 (RX4p)  | Lane 3 Receive +              |
| MGT117RX3_N  | QSFP1_RX3_N      | In        | 22 (RX4n)  | Lane 3 Receive -              |
| MGT117TX3_P  | QSFP1_TX3_P      | Out       | 38 (TX4p)  | Lane 3 Transmit +             |
| MGT117TX3_N  | QSFP1_TX3_N      | Out       | 39 (TX4n)  | Lane 3 Transmit -             |
| MGT117REFCLK0_P | QSFP1_REFCLK_P | In     | —          | 156.25 MHz from SiT9365 U11   |
| MGT117REFCLK0_N | QSFP1_REFCLK_N | In     | —          | 156.25 MHz from SiT9365 U11   |
| AL18         | QSFP1_SCL        | Out       | 4 (SCL)    | I2C Clock (3.3V)              |
| AM18         | QSFP1_SDA        | InOut     | 5 (SDA)    | I2C Data (3.3V)               |
| AN18         | QSFP1_MODPRSL    | In        | 8 (ModPrsL)| Module Present (active low)   |
| AP18         | QSFP1_RESETL     | Out       | 7 (ResetL) | Module Reset (active low)     |
| AR18         | QSFP1_LPMODE     | Out       | 31 (LPMode)| Low Power Mode                |
| AT18         | QSFP1_INTL       | In        | 9 (IntL)   | Module Interrupt (active low) |

### 2.6 Detailed Pin Assignments — Peripheral Interface

| FPGA Pin | Signal Name    | Direction | Dest Pin       | Description                    |
|----------|----------------|-----------|----------------|--------------------------------|
| A16      | SPI_FLASH_CS#  | Out       | U3-S1# (CS#)   | QSPI Chip Select               |
| B16      | SPI_FLASH_SCK  | Out       | U3-C (SCK)     | QSPI Clock                     |
| C16      | SPI_FLASH_DQ0  | InOut     | U3-DQ0 (IO0)   | QSPI Data 0 (MOSI)             |
| D16      | SPI_FLASH_DQ1  | InOut     | U3-DQ1 (IO1)   | QSPI Data 1 (MISO)             |
| E16      | SPI_FLASH_DQ2  | InOut     | U3-DQ2 (IO2)   | QSPI Data 2 (WP#)              |
| F16      | SPI_FLASH_DQ3  | InOut     | U3-DQ3 (IO3)   | QSPI Data 3 (HOLD#)            |
| G16      | SPI_FLASH_RESET#| Out      | U3-RST#        | Flash Reset                    |
| H16      | I2C_SCL        | Out       | U5,U6,U7 SCL   | I2C Bus Clock (1.8V)           |
| J16      | I2C_SDA        | InOut     | U5,U6,U7 SDA   | I2C Bus Data (1.8V)            |
| K16      | UART_TX        | Out       | U4-BDBUS0      | MicroBlaze UART TX → FT232H    |
| L16      | UART_RX        | In        | U4-BDBUS1      | MicroBlaze UART RX ← FT232H    |
| M16      | JTAG_TCK       | In        | U4-TCK         | JTAG Clock                     |
| N16      | JTAG_TMS       | In        | U4-TMS         | JTAG Mode Select               |
| P16      | JTAG_TDI       | In        | U4-TDI         | JTAG Data In                   |
| R16      | JTAG_TDO       | Out       | U4-TDO         | JTAG Data Out                  |
| T16      | FAN_PWM        | Out       | U7-PWM         | Fan PWM Control                |
| U16      | FAN_TACH0      | In        | U7-TACH0       | Fan 0 Tachometer               |
| V16      | FAN_TACH1      | In        | U7-TACH1       | Fan 1 Tachometer               |
| W16      | LED0_GREEN     | Out       | LED1-A(Green)  | Status LED Green               |
| Y16      | LED0_RED       | Out       | LED1-C(Red)    | Status LED Red                 |
| AA16     | LED1_GREEN     | Out       | LED2-A(Green)  | Link LED Green                 |
| AB16     | LED1_RED       | Out       | LED2-C(Red)    | Link LED Red                   |
| AC16     | LED2_GREEN     | Out       | LED3-A(Green)  | Activity LED Green             |
| AD16     | LED2_RED       | Out       | LED3-C(Red)    | Activity LED Red               |
| AE16     | LED3_GREEN     | Out       | LED4-A(Green)  | Error LED Green                |
| AF16     | LED3_RED       | Out       | LED4-C(Red)    | Error LED Red                  |
| AG16     | CONFIG_M0      | In        | GND/3.3V       | Mode pin 0 (SPI x4 = 01)       |
| AH16     | CONFIG_M1      | In        | GND/3.3V       | Mode pin 1 (SPI x4 = 01)       |
| AJ16     | CONFIG_M2      | In        | GND            | Mode pin 2 (SPI x4 = 0)        |
| AK16     | CONFIG_INIT_B  | Out       | —              | Configuration init (open-drain)|
| AL16     | CONFIG_DONE    | Out       | —              | Configuration done             |
| AM16     | CONFIG_PROG_B  | In        | SW1             | Program (active low, reset)     |
| AN16     | CONFIG_CCLK    | Out       | U3-C (SCK)     | Config CCLK (shared with SCK)  |
| AP16     | CONFIG_D00     | InOut     | U3-DQ0         | Config D00 (shared with DQ0)   |
| AR16     | CONFIG_D01     | InOut     | U3-DQ1         | Config D01 (shared with DQ1)   |
| AT16     | CONFIG_D02     | InOut     | U3-DQ2         | Config D02 (shared with DQ2)   |
| AU16     | CONFIG_D03     | InOut     | U3-DQ3         | Config D03 (shared with DQ3)   |

### 2.7 Clock Input Pin Assignments

| FPGA Pin | Signal Name       | Source      | Frequency        | Standard |
|----------|-------------------|-------------|------------------|----------|
| A34_P    | PCIE_REFCLK_P     | PCIe Slot   | 100.000 MHz      | HCSL     |
| A34_N    | PCIE_REFCLK_N     | PCIe Slot   | 100.000 MHz      | HCSL     |
| B34_P    | DDR4_REFCLK_P     | U8 (SiT9121)| 200.000 MHz      | LVDS     |
| B34_N    | DDR4_REFCLK_N     | U8 (SiT9121)| 200.000 MHz      | LVDS     |
| C34_P    | CMAC_REFCLK_P     | U9 (SiT9365)| 161.1328125 MHz  | LVDS     |
| C34_N    | CMAC_REFCLK_N     | U9 (SiT9365)| 161.1328125 MHz  | LVDS     |
| D34_P    | QSFP0_REFCLK_P    | U10 (SiT9365)| 156.250 MHz     | LVDS     |
| D34_N    | QSFP0_REFCLK_N    | U10 (SiT9365)| 156.250 MHz     | LVDS     |
| E34_P    | QSFP1_REFCLK_P    | U11 (SiT9365)| 156.250 MHz     | LVDS     |
| E34_N    | QSFP1_REFCLK_N    | U11 (SiT9365)| 156.250 MHz     | LVDS     |

## 3. Power Distribution Network

### 3.1 Power Tree

```
PCIe 12V (±8%) ──┬──► [Fuse 10A] ──► 12V_PCIE (main rail)
                  │
                  ├──► U12 TPS546D24A ──► VCCINT (1.00V @ 30A max)
                  │   ├── C_in: 2x 22µF 0805 + 1x 100µF 1206
                  │   ├── L_out: 0.47µH (DFE252012F-R47M)
                  │   ├── C_out: 4x 47µF 0805 + 2x 100µF 1206 + 2x 330µF Tantalum
                  │   └── Bulk: 1x 470µF Tantalum at FPGA
                  │
                  ├──► U13 TPS546D24A ──► VCCBRAM (1.00V @ 5A max)
                  │   ├── C_in: 1x 22µF 0805 + 1x 47µF 0805
                  │   ├── L_out: 1.0µH (DFE252012F-1R0M)
                  │   ├── C_out: 2x 47µF 0805 + 1x 100µF 1206
                  │   └── Bulk: 1x 330µF Tantalum at FPGA
                  │
                  ├──► U14 TPS546D24A ──► VCCAUX (1.80V @ 8A max)
                  │   ├── C_in: 1x 22µF 0805 + 1x 47µF 0805
                  │   ├── L_out: 1.0µH (DFE252012F-1R0M)
                  │   ├── C_out: 2x 47µF 0805 + 1x 100µF 1206
                  │   └── Bulk: 1x 330µF Tantalum at FPGA
                  │
                  ├──► U15 TPS546D24A ──► VCCO_1V5 (1.50V @ 10A max)
                  │   ├── C_in: 1x 22µF 0805 + 1x 47µF 0805
                  │   ├── L_out: 1.0µH (DFE252012F-1R0M)
                  │   ├── C_out: 2x 47µF 0805 + 1x 100µF 1206
                  │   └── Bulk: 1x 330µF Tantalum at FPGA
                  │
                  ├──► U16 TPS546D24A ──► MGTAVCC (1.00V @ 12A max)
                  │   ├── C_in: 2x 22µF 0805 + 1x 100µF 1206
                  │   ├── L_out: 0.47µH (DFE252012F-R47M)
                  │   ├── C_out: 4x 47µF 0805 + 2x 100µF 1206 + 2x 330µF Tantalum
                  │   └── Bulk: 1x 470µF Tantalum at FPGA
                  │
                  ├──► U17 TPS546D24A ──► MGTAVTT (1.20V @ 8A max)
                  │   ├── C_in: 1x 22µF 0805 + 1x 47µF 0805
                  │   ├── L_out: 1.0µH (DFE252012F-1R0M)
                  │   ├── C_out: 2x 47µF 0805 + 1x 100µF 1206
                  │   └── Bulk: 1x 330µF Tantalum at FPGA
                  │
                  ├──► U18 TPS7A92 ──► VCC3V3 (3.30V @ 2A max)
                  │   ├── C_in: 1x 10µF 0603 + 1x 22µF 0805
                  │   ├── C_out: 2x 10µF 0603 + 1x 22µF 0805
                  │   └── Ferrite: BLM18PG121SN1D per load group
                  │
                  ├──► U19 TPS7A92 ──► VCC2V5 (2.50V @ 1A max)
                  │   ├── C_in: 1x 10µF 0603 + 1x 22µF 0805
                  │   └── C_out: 2x 10µF 0603 + 1x 22µF 0805
                  │
                  └──► U20 TPS7A92 ──► VCC1V8 (1.80V @ 2A max)
                      ├── C_in: 1x 10µF 0603 + 1x 22µF 0805
                      └── C_out: 2x 10µF 0603 + 1x 22µF 0805
```

### 3.2 FPGA Decoupling Network (per supply)

**VCCINT (1.00V) — Core Logic Supply:**
- 1x 470µF Tantalum (T520B477M0025ATE025) at power entry
- 4x 47µF 0805 (GRM21BR60J476ME15L) distributed around FPGA perimeter
- 2x 100µF 1206 (GRM31CR60J107ME39L) near FPGA center
- 20x 1.0µF 0402 (GRM155R61A105KE15D) — one per 4 I/O banks
- 40x 0.1µF 0402 (GRM155R71C104KA88D) — one per power pin pair
- 20x 0.01µF 0402 (GRM155R71C103KA01D) — high-frequency bypass

**MGTAVCC (1.00V) — GTX Transceiver Analog Supply:**
- 1x 470µF Tantalum at power entry
- 2x 330µF Tantalum (T520B337M006ATE025) near GTX quads
- 4x 47µF 0805 distributed
- 2x 100µF 1206 near GTX quads
- 8x 1.0µF 0402 — one per GTX channel pair
- 16x 0.1µF 0402 — one per GTX power pin
- 8x 0.01µF 0402 — high-frequency bypass

**MGTAVTT (1.20V) — GTX Termination Supply:**
- 1x 330µF Tantalum at power entry
- 2x 47µF 0805 distributed
- 1x 100µF 1206 near GTX quads
- 8x 1.0µF 0402
- 16x 0.1µF 0402
- 8x 0.01µF 0402

**VCCBRAM (1.00V) — Block RAM Supply:**
- 1x 330µF Tantalum at power entry
- 2x 47µF 0805
- 1x 100µF 1206
- 8x 0.1µF 0402
- 4x 0.01µF 0402

**VCCAUX (1.80V) — Auxiliary Supply:**
- 1x 330µF Tantalum at power entry
- 2x 47µF 0805
- 1x 100µF 1206
- 8x 0.1µF 0402
- 4x 0.01µF 0402

**VCCO (1.50V) — I/O Bank Supply:**
- 1x 330µF Tantalum at power entry
- 2x 47µF 0805
- 1x 100µF 1206
- 12x 0.1µF 0402 — one per I/O bank
- 6x 0.01µF 0402

### 3.3 DDR4 VTT Termination

DDR4 requires VTT = VDD/2 = 0.6V for address/command/control termination.
- TPS51200DRCR (TI) — DDR4 VTT sink/source regulator
- VDDQ = 1.2V from VCCO rail
- VTT = 0.6V, capable of ±3A
- C_out: 2x 10µF 0603 + 2x 22µF 0805 + 1x 100µF 1206
- 49.9Ω termination resistors (RC0402FR-0749R9L) on each address/control line at DRAM end

## 4. Netlist — Critical Signal Paths

### 4.1 PCIe Differential Pairs (100Ω differential impedance)

```
PCIe Edge A21 (PERn0) ──0.1µF AC-coupling──► FPGA MGT115RX0_N
PCIe Edge A22 (PERp0) ──0.1µF AC-coupling──► FPGA MGT115RX0_P
FPGA MGT115TX0_N ──0.1µF AC-coupling──► PCIe Edge B21 (PETn0)
FPGA MGT115TX0_P ──0.1µF AC-coupling──► PCIe Edge B22 (PETp0)
... (repeat for lanes 1-7)
```

AC coupling capacitors: 0.1µF 0402 (GRM155R71C104KA88D), placed near FPGA TX pins.
PCIe spec requires 75-200nF on TX side. RX side AC coupling is on the host motherboard.

### 4.2 100GbE QSFP28 Differential Pairs (100Ω differential impedance)

```
QSFP0 Pin 12 (RX1p) ──► FPGA MGT116RX0_P
QSFP0 Pin 13 (RX1n) ──► FPGA MGT116RX0_N
FPGA MGT116TX0_P ──0.1µF AC-coupling──► QSFP0 Pin 29 (TX1p)
FPGA MGT116TX0_N ──0.1µF AC-coupling──► QSFP0 Pin 30 (TX1n)
... (repeat for lanes 1-3 on both ports)
```

### 4.3 DDR4 Fly-By Topology — Channel A

```
FPGA ──► U2 (MT40A512M16) Address/Command/Control (fly-by chain)
  CK0_P/N ──50Ω single-ended──► U2.J1/H1 (CK_t/CK_c)
  A[0:13] ──40Ω──► U2 address pins (N2,N7,N3,P8,P3,R8,R2,R3,T8,T3,L7,N1,P7,P2)
  BA[0:1] ──40Ω──► U2.M2/M3
  BG0     ──40Ω──► U2.L6
  CKE     ──40Ω──► U2.K2
  CS#     ──40Ω──► U2.L3
  ODT     ──40Ω──► U2.K8
  ACT#    ──40Ω──► U2.L5
  RAS#    ──40Ω──► U2.J7
  CAS#    ──40Ω──► U2.K3
  WE#     ──40Ω──► U2.K7
  RESET#  ──40Ω──► U2.G2
  PAR     ──40Ω──► U2.M7
  ALERT#  ──40Ω──► U2.J3 (with 4.7kΩ pull-up to 1.2V)

DQ Byte Lane 0 (point-to-point):
  DQ[0:7]   ──40Ω──► U2.A2,A3,B1,B2,C1,C2,C3,D2
  DQS0_P/N  ──40Ω──► U2.A7/A8
  DM0       ──40Ω──► U2.D1

DQ Byte Lane 1 (point-to-point):
  DQ[8:15]  ──40Ω──► U2.D3,E1,E2,E3,F1,F2,F3,G1
  DQS1_P/N  ──40Ω──► U2.A12/B12
  DM1       ──40Ω──► U2.D11

DQ Byte Lane 2 (point-to-point):
  DQ[16:23] ──40Ω──► U2.A9,B9,C9,D9,E9,F9,G9,H9
  DQS2_P/N  ──40Ω──► U2.A14/B14
  DM2       ──40Ω──► U2.D12

DQ Byte Lane 3 (point-to-point):
  DQ[24:31] ──40Ω──► U2.A10,B10,C10,D10,E10,F10,G10,H10
  DQS3_P/N  ──40Ω──► U2.A15/B15
  DM3       ──40Ω──► U2.D13
```

### 4.4 I2C Bus Topology

```
FPGA I2C_SCL (H16) ──┬──2.2kΩ pull-up to 1.8V──► U5 (TMP117 #0) SCL, addr 0x48
                      ├──2.2kΩ pull-up to 1.8V──► U5 (TMP117 #1) SCL, addr 0x49
                      ├──2.2kΩ pull-up to 1.8V──► U6 (INA226) SCL, addr 0x40
                      └──2.2kΩ pull-up to 1.8V──► U7 (EMC2301) SCL, addr 0x2E

FPGA I2C_SDA (J16) ──┬──2.2kΩ pull-up to 1.8V──► U5 #0 SDA
                      ├──2.2kΩ pull-up to 1.8V──► U5 #1 SDA
                      ├──2.2kΩ pull-up to 1.8V──► U6 SDA
                      └──2.2kΩ pull-up to 1.8V──► U7 SDA
```

Note: QSFP28 I2C buses are separate (3.3V level, FPGA Bank 18) with 4.7kΩ pull-ups.

### 4.5 QSPI Flash Connections

```
FPGA SPI_FLASH_CS# (A16) ──► U3.S1# (with 10kΩ pull-up to 1.8V)
FPGA SPI_FLASH_SCK (B16) ──► U3.C (SCK) — 50Ω series termination at FPGA
FPGA SPI_FLASH_DQ0 (C16) ──► U3.DQ0 (IO0) — 50Ω series termination
FPGA SPI_FLASH_DQ1 (D16) ──► U3.DQ1 (IO1) — 50Ω series termination
FPGA SPI_FLASH_DQ2 (E16) ──► U3.DQ2 (IO2) — 50Ω series termination
FPGA SPI_FLASH_DQ3 (F16) ──► U3.DQ3 (IO3) — 50Ω series termination
FPGA SPI_FLASH_RESET# (G16) ──► U3.RST# (with 10kΩ pull-up to 1.8V)
```

### 4.6 USB/JTAG Interface (FT232H)

```
FPGA JTAG_TCK (M16) ──► U4.TCK (pin 25) — 100Ω series termination
FPGA JTAG_TMS (N16) ──► U4.TMS (pin 26) — 100Ω series termination
FPGA JTAG_TDI (P16) ──► U4.TDI (pin 27) — 100Ω series termination
FPGA JTAG_TDO (R16) ──► U4.TDO (pin 28) — 100Ω series termination
FPGA UART_TX (K16) ──► U4.BDBUS0 (pin 13) — 100Ω series termination
FPGA UART_RX (L16) ──► U4.BDBUS1 (pin 14) — 100Ω series termination
U4.USBDM ──► J3.D- (USB Micro-B)
U4.USBDP ──► J3.D+ (USB Micro-B)
U4.VCCIO ── 3.3V from U18
U4.VCORE ── 1.8V from U20 (internal LDO bypassed)
U4.RESET# ── 10kΩ pull-up to 3.3V
```

## 5. Impedance Control Summary

| Signal Type              | Target Impedance | Tolerance | Layer Pair       |
|--------------------------|------------------|-----------|------------------|
| PCIe Gen4 differential   | 100Ω differential| ±10%      | Top/Bot to L2/L5 |
| 100GbE differential      | 100Ω differential| ±10%      | Top to L2        |
| DDR4 DQ single-ended     | 40Ω              | ±10%      | L1/L3 to L2/L4   |
| DDR4 DQS differential    | 80Ω differential | ±10%      | L1/L3 to L2/L4   |
| DDR4 CLK differential    | 100Ω differential| ±10%      | L1 to L2         |
| DDR4 ADDR/CMD single-ended| 40Ω            | ±10%      | L1 to L2         |
| USB 2.0 differential     | 90Ω differential | ±10%      | Top to L2        |
| LVDS clocks              | 100Ω differential| ±10%      | Top to L2        |
| Single-ended GPIO/I2C    | 50Ω              | ±15%      | Any              |

## 6. Pull-Up / Pull-Down Resistor Table

| Signal              | Resistor  | Voltage | Purpose                                    |
|---------------------|-----------|---------|--------------------------------------------|
| PCIE_PERST#         | 10kΩ PU   | 3.3Vaux | Ensure high when slot not driving          |
| PCIE_WAKE#          | 10kΩ PU   | 3.3Vaux | Open-drain, pull to inactive               |
| PCIE_CLKREQ#        | 10kΩ PD   | GND     | Default to clock request inactive           |
| CONFIG_M0           | 4.7kΩ PU  | 3.3V    | SPI x4 master mode (M[2:0] = 001)          |
| CONFIG_M1           | 4.7kΩ PD  | GND     | SPI x4 master mode                         |
| CONFIG_M2           | 4.7kΩ PD  | GND     | SPI x4 master mode                         |
| CONFIG_PROG_B       | 4.7kΩ PU  | 3.3V    | Default not programming                    |
| CONFIG_INIT_B       | 4.7kΩ PU  | 3.3V    | Open-drain, pull to done state             |
| CONFIG_DONE         | 4.7kΩ PU  | 3.3V    | Pull high when config complete             |
| SPI_FLASH_CS#       | 10kΩ PU   | 1.8V    | Deselect flash when FPGA not driving       |
| SPI_FLASH_RESET#    | 10kΩ PU   | 1.8V    | Keep flash out of reset                    |
| I2C_SCL             | 2.2kΩ PU  | 1.8V    | I2C bus pull-up (1.8V domain)              |
| I2C_SDA             | 2.2kΩ PU  | 1.8V    | I2C bus pull-up (1.8V domain)              |
| QSFP0_SCL           | 4.7kΩ PU  | 3.3V    | QSFP I2C bus pull-up                       |
| QSFP0_SDA           | 4.7kΩ PU  | 3.3V    | QSFP I2C bus pull-up                       |
| QSFP1_SCL           | 4.7kΩ PU  | 3.3V    | QSFP I2C bus pull-up                       |
| QSFP1_SDA           | 4.7kΩ PU  | 3.3V    | QSFP I2C bus pull-up                       |
| QSFP0_MODPRSL       | 10kΩ PU   | 3.3V    | Default high (module absent)               |
| QSFP1_MODPRSL       | 10kΩ PU   | 3.3V    | Default high (module absent)               |
| QSFP0_INTL          | 10kΩ PU   | 3.3V    | Open-drain interrupt                       |
| QSFP1_INTL          | 10kΩ PU   | 3.3V    | Open-drain interrupt                       |
| DDR4_ALERT#         | 4.7kΩ PU  | 1.2V    | DRAM alert (active low)                    |
| FT232H_RESET#       | 10kΩ PU   | 3.3V    | Keep FT232H out of reset                   |
| JTAG_TMS            | 10kΩ PU   | 3.3V    | JTAG state machine idle                    |
| JTAG_TDI            | 10kΩ PU   | 3.3V    | JTAG data idle                             |
| FAN_TACH0           | 10kΩ PU   | 3.3V    | Tachometer open-collector pull-up          |
| FAN_TACH1           | 10kΩ PU   | 3.3V    | Tachometer open-collector pull-up          |

## 7. Thermal Sensor Placement

| Sensor | Location                          | Monitors                        |
|--------|-----------------------------------|---------------------------------|
| U5 #0  | Adjacent to FPGA die center       | FPGA junction temperature proxy |
| U5 #1  | Near QSFP28 cage #0               | QSFP28 module ambient           |
| U6     | Near 12V power entry              | Board power consumption         |
| FPGA   | Internal XADC (Die Temp + VCCINT) | FPGA junction + supply monitor  |

## 8. Configuration and Bootstrap Sequence

1. PCIe 12V and 3.3Vaux applied from slot.
2. 3.3Vaux powers FPGA configuration bank (Bank 0), CONFIG pins sampled.
3. M[2:0] = 001 → SPI x4 master mode.
4. FPGA drives CCLK, reads bitstream from U3 (MT25QU512).
5. CONFIG_DONE asserts high.
6. FPGA releases internal reset, MMCMs lock.
7. DDR4 MIG calibration runs (~100ms).
8. PCIe Hard IP negotiates link (Gen4 x8).
9. CMAC SerDes PLLs lock to reference clocks.
10. MicroBlaze boots from BRAM, initializes peripherals.
11. NVMe controller BARs enumerated by host BIOS/OS.
12. Host NVMe driver loads, admin queues established.
13. Management driver loads, exposes sysfs interface.
14. Device ready for NVMe-oF connections.
