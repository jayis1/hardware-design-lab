# RideCore — 48V/200A PMSM Motor Controller

RideCore is an open-hardware three-phase PMSM motor controller for light electric vehicles (e-bikes, e-scooters, mopeds, small EVs). It features sensorless Field-Oriented Control (FOC) with SVPWM, CAN FD telemetry, BLE 5.0 wireless tuning, and a companion React Native app.

## Specifications

| Parameter | Value |
|---|---|
| DC Bus Voltage | 24–60 V (abs max 72 V) |
| Continuous Phase Current | 120 A RMS |
| Peak Phase Current | 200 A RMS (10 s) |
| PWM Frequency | 10–30 kHz (default 20 kHz) |
| Control Loop Rate | 20 kHz |
| MCU | STM32G474CEU6 (Cortex-M4 @ 170 MHz) |
| MOSFETs | 6× Infineon IPT015N10N5ATMA1 (100V, 1.5 mΩ) |
| Gate Drivers | 3× IRS2186STRPBF (half-bridge, bootstrap) |
| Isolation | 3× ADuM3223CRZ digital isolators |
| Current Sensing | 3× INA241A (50 V/V, 0.5 mΩ shunts) |
| Communication | CAN FD (5 Mbps) + BLE 5.0 |
| Power Management | TPS6521801 PMIC (5V, 3.3V, 1.8V) |
| Storage | 16 Mb W25Q128 SPI flash |
| Board Size | 80 × 60 × 1.6 mm, 6-layer FR-4 |
| BOM Cost | ~$45 USD (1K volume) |
| Firmware License | Apache 2.0 |
| Hardware License | CERN-OHL-S v2 |
| App License | MIT |

## Directory Tree

```
ridecore/
├── README.md
├── phase1_conceptual_architecture.md
├── phase2_component_selection_schematics.md
├── phase3_pcb_blueprints_layout.md
├── phase4_software_stack.md
├── kicad/
│   ├── device.kicad_pro
│   ├── device.kicad_sch
│   └── device.kicad_pcb
├── firmware/
│   ├── Makefile
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── clock_config.c
│   ├── gpio_init.c
│   ├── foc.h
│   ├── foc.c
│   └── drivers/
│       ├── pmic.h
│       ├── pmic.c
│       ├── canfd.h
│       ├── canfd.c
│       ├── spi_flash.h
│       ├── spi_flash.c
│       ├── ble_uart.h
│       └── ble_uart.c
└── app/
    ├── package.json
    ├── App.js
    ├── screens/
    │   ├── DashboardScreen.js
    │   ├── TuningScreen.js
    │   ├── DiagnosticsScreen.js
    │   └── SettingsScreen.js
    ├── components/
    │   ├── CircularGauge.js
    │   └── SliderCard.js
    └── utils/
        └── protocol.js
```

## Quick Start

### Firmware Build

```bash
cd ridecore/firmware
# Requires arm-none-eabi-gcc toolchain
make clean && make all
# Output: build/ridecore.elf, build/ridecore.bin, build/ridecore.hex
```

### Flash (ST-Link)

```bash
make flash
# or: openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
#     -c "program build/ridecore.elf verify reset exit"
```

### Flash (USB DFU)

```bash
make dfu
# Hold BOOT0, press RESET, then release BOOT0
```

### Companion App

```bash
cd ridecore/app
npm install
npx react-native run-android   # or run-ios
```

### Serial Console

Connect USB-C (115200 8N1) or use any UART adapter:

```
> ridecore info
> ridecore set current_limit 120000
> ridecore faults
> ridecore save
```

## Key Design Features

- **Galvanic Isolation:** Digital isolators (ADuM3223) separate the MCU from the 48V power stage, protecting logic circuitry from HV transients.
- **SVPWM FOC:** Space-vector PWM with center-aligned mode, 500 ns dead time, PI current controllers on d/q axes.
- **Triple Current Sensing:** Low-side shunt resistors (0.5 mΩ) with INA241A 50 V/V amplifiers feed 12-bit ADCs for closed-loop current control.
- **CAN FD:** 5 Mbps data-phase CAN FD via MCP2518FD, compatible with automotive vehicle networks.
- **BLE 5.0 Tuning:** Nordic nRF52832 provides wireless real-time parameter adjustment and telemetry via the companion app.
- **Full Protection Suite:** Overvoltage, undervoltage, overcurrent, DESAT, over-temperature, and stall detection with automatic gate shutdown.

## License

- **Hardware:** CERN-OHL-S v2 (Open Hardware License)
- **Firmware:** Apache License 2.0
- **Companion App:** MIT License