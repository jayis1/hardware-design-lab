# SkyPilot — Phase 1: Conceptual Architecture

## 1. System Purpose

SkyPilot is a **dual-IMU drone flight controller with integrated 4G/LTE telemetry**, designed for beyond-visual-line-of-sight (BVLOS) autonomous drone operations. It combines a high-performance STM32H743 flight processor with dual redundant IMUs (ICM-42688-P + BMI270), a u-blox LARA-R6 4G/LTE modem for real-time telemetry and command, and an ESP32-H2-based companion computer for AI-assisted mission planning. The board targets professional survey, inspection, and delivery drones that require reliable cellular connectivity and redundant attitude sensing.

## 2. Performance Targets

| Parameter | Target |
|---|---|
| Flight loop rate | 8 kHz (IMU) / 400 Hz (PID) |
| Latency (sensor → motor output) | ≤ 250 µs |
| 4G/LTE telemetry throughput | ≥ 5 Mbps downlink, ≥ 2 Mbps uplink |
| 4G/LTE latency (round-trip) | ≤ 80 ms |
| Position hold accuracy | ± 0.5 m (with RTK GNSS) |
| Attitude estimation accuracy | ± 0.5° pitch/roll, ± 1.0° yaw |
| PWM outputs | 12 channels, DShot1200 capable |
| Power input range | 7–26 V (2–6S LiPo) |
| Board current consumption | ≤ 450 mA @ 5V (without modem) |
| Operating temperature | -20°C to +70°C |
| MTBF (calculated) | > 50,000 hours |
| Weight (PCB + components) | ≤ 28 g |

## 3. Constraints

- **Size**: 45 mm × 45 mm (standard FC mounting pattern, 30.5 mm hole spacing)
- **Mounting**: 4× M3 at 30.5 mm square, soft-mount compatible
- **USB**: USB-C 2.0 for configuration (not flight)
- **Power**: Single battery input, 5V/3A BEC onboard, 3.3V/1.5A LDO for digital
- **RF**: LTE antenna via u.FL connector; GNSS via ceramic patch or u.FL
- **Certification**: FCC Part 15 (Class B), CE RED, FCC Part 87 (aviation telemetry)
- **Stackup**: 6-layer, 1.0 mm thickness for weight savings
- **Safety**: Dual IMU voting, hardware watchdog, backup flight recovery via companion

## 4. High-Level Block Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                            SkyPilot FC Board                             │
│                                                                          │
│  ┌─────────────┐      SPI1 @ 30MHz      ┌────────────────────────────┐  │
│  │ ICM-42688-P │◄──────────────────────► │                            │  │
│  │ (Primary    │                         │     STM32H743VIT6          │  │
│  │  IMU/Accel+ │      SPI4 @ 30MHz      │     (Flight Processor)    │  │
│  │  Gyro)      │◄──────────────────────► │     480 MHz Cortex-M7     │  │
│  └─────────────┘                         │     1MB RAM, 2MB Flash    │  │
│                                          │                            │  │
│  ┌─────────────┐      SPI2 @ 30MHz      │  ┌──────────────────────┐  │  │
│  │ BMI270      │◄──────────────────────► │  │ FDCAN1 (CAN-FD)     │  │  │
│  │ (Secondary  │                         │  │ UART1 (GNSS)         │  │  │
│  │  IMU)       │                         │  │ UART4 (Companion)    │  │  │
│  └─────────────┘                         │  │ UART8 (LTE AT cmd)   │  │  │
│                                          │  │ SPI1/SPI2/SPI4 (IMU) │  │  │
│  ┌─────────────┐      UART1 @ 460800     │  │ TIM1-8 (PWM/DShot)   │  │  │
│  │ u-blox      │◄──────────────────────► │  │ I2C1 (Baro/RTC)      │  │  │
│  │ SAM-M10Q    │                         │  │ USB OTG FS (Config)   │  │  │
│  │ (GNSS)      │                         │  │ ADC1 (Battery)        │  │  │
│  └─────────────┘                         │  └──────────────────────┘  │  │
│                                          │                            │  │
│  ┌─────────────┐      UART8 @ 921600     │                            │  │
│  │ u-blox      │◄──────────────────────► │                            │  │
│  │ LARA-R6     │      USB (config mode)  │                            │  │
│  │ (4G/LTE)    │◄──────────────────────► │                            │  │
│  └──────┬──────┘                         │                            │  │
│         │ u.FL                            └──────────┬───────────────┘  │
│         ▼                                            │                  │
│  ┌──────────────┐                                    │                  │
│  │ LTE Antenna  │                              UART4 │                  │
│  │ (External)   │                                    ▼                  │
│  └──────────────┘                         ┌────────────────────────┐   │
│                                           │ ESP32-H2              │   │
│  ┌─────────────┐      I2C1                │ (Companion Computer)  │   │
│  │ BMP390      │◄────────────────────┐    │ 96 MHz RISC-V         │   │
│  │ (Barometer) │                     │    │ 512KB SRAM            │   │
│  └─────────────┘                     ├────►│ IEEE 802.15.4/Zigbee │   │
│                                      │     │ USB-Serial bridge     │   │
│  ┌─────────────┐      I2C1           │     └────────────────────────┘   │
│  │ RV-3032-7   │◄────────────────────┘                                  │
│  │ (RTC)       │                                                        │
│  └─────────────┘                                                        │
│                                                                          │
│  ┌─────────────────────────────────────────────────────────────────┐     │
│  │ Power Tree: VBAT (7-26V) → TPS65294 (5V/3A BEC) →                │     │
│  │             TLV75533 (3.3V/1.5A) + TLV75518 (1.8V/500mA)          │     │
│  │             + STM32 VBAT (RTC backup from CR1225)                  │     │
│  └─────────────────────────────────────────────────────────────────┘     │
│                                                                          │
│  Connectors: USB-C (config), 40-pin JST-SH (I/O), u.FL×2 (LTE+GNSS)   │
│  Motor pads: 12× DShot1200 pads on edges                                │
└──────────────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Sensor Fusion Pipeline

```
ICM-42688-P ──SPI1──► ┌─────────┐     ┌──────────┐     ┌─────────┐
                       │ Sensor  │────►│  EKF /   │────►│ Mixer / │──► Motor Outputs
BMI270 ──────SPI2──►  │ Fusion  │     │  Mahony  │     │ DShot   │    (TIM1-4)
                       │ (Voting)│     │  Filter  │     │ Output  │
BMP390 ──────I2C──►  └─────────┘     └──────────┘     └─────────┘
SAM-M10Q ──UART1►          ▲
                           │
LARA-R6 ──UART8► (telemetry relay to GCS via LTE)
ESP32-H2 ─UART4► (companion commands)
```

### 5.2 Telemetry Data Flow

```
Flight Processor ──UART8──► LARA-R6 ──4G/LTE──► Cloud/GCS
                                │
                           TCP/UDP socket
                           (MAVLink v2 over LTE)
                           
Flight Processor ──UART4──► ESP32-H2 ──802.15.4──► Swarm nodes
                                           │
                                      USB-C ──► Config PC
```

### 5.3 Boot Flow

```
Power On
  │
  ├─ STM32H743 internal bootloader checks:
  │    BOOT0=0 → Flash boot (normal)
  │    BOOT0=1 → System bootloader (DFU/UART1)
  │
  ├─ SPL (boot ROM) → Init clocks (480 MHz) → Init SRAM
  │
  ├─ Phase 1: Validate IMU self-test
  │    ├─ ICM-42688-P: WHO_AM_I = 0x47
  │    ├─ BMI270: CHIP_ID = 0x24
  │    └─ BMP390: CHIP_ID = 0x60
  │
  ├─ Phase 2: Init GNSS, LTE modem
  │    ├─ SAM-M10Q: UBX-CFG-PRT
  │    └─ LARA-R6: AT+CFUN=1
  │
  └─ Phase 3: Start flight loop @ 8kHz
       └─ Sensor read → EKF → Mixer → DShot
```

## 6. Bus Topology

| Bus | Master | Slaves | Speed | Protocol |
|---|---|---|---|---|
| SPI1 | STM32H743 | ICM-42688-P | 30 MHz | SPI Mode 0 |
| SPI2 | STM32H743 | BMI270 | 30 MHz | SPI Mode 0 |
| SPI4 | STM32H743 | Reserved (OSD/Flash) | 20 MHz | SPI Mode 0 |
| I2C1 | STM32H743 | BMP390 (0x77), RV-3032-7 (0x32) | 400 kHz | I2C Fast Mode |
| UART1 | STM32H743 | SAM-M10Q GNSS | 460800 baud | 8N1 |
| UART4 | STM32H743 | ESP32-H2 companion | 921600 baud | 8N1 |
| UART8 | STM32H743 | LARA-R6 LTE modem | 921600 baud | 8N1 + HW FC |
| FDCAN1 | STM32H743 | CAN bus / DroneCAN | 1 Mbps | CAN-FD |
| USB FS | STM32H743 | USB-C config port | 12 Mbps | USB 2.0 FS |
| ADC1 | STM32H743 | Battery voltage divider | — | 12-bit SAR |
| TIM1-4 | STM32H743 | 12× DShot/PWM motor outputs | 24 MHz | DShot1200 |

### Address Map (STM32H743)

| Region | Base Address | Size | Purpose |
|---|---|---|---|
| Flash | 0x08000000 | 2 MB | Firmware |
| ITCM | 0x00000000 | 64 KB | Flight loop code |
| DTCM | 0x20000000 | 128 KB | Sensor data, PID state |
| AXI SRAM | 0x24000000 | 512 KB | DMA buffers, telemetry |
| SRAM1 | 0x30000000 | 128 KB | FreeRTOS heap |
| SRAM2 | 0x30020000 | 128 KB | Companion IPC |
| Peripherals | 0x40000000 | — | MMIO |
| DMA1 | 0x40020000 | — | DMA controller 1 |
| DMA2 | 0x40020400 | — | DMA controller 2 |