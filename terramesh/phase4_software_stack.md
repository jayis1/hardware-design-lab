# Terramesh — Phase 4: Software Stack

**Author: jayis1**  
**Copyright © 2026 jayis1**

---

## 1. MMIO / Register Definitions

See `firmware/registers.h` for the complete register map.

Key memory-mapped regions:

| Base Address | Region | Size | Description |
|-------------|--------|------|-------------|
| 0x4000 0000 | APB1 | 64 KB | Timers, I2C, SPI, USART, LPUART |
| 0x4001 0000 | APB2 | 64 KB | GPIO, SPI, TIM, ADC |
| 0x4002 0000 | AHB1 | 64 KB | DMA, RCC, CRC, FLASH |
| 0x4003 0000 | AHB2 | 64 KB | PWR, ADC, DAC |
| 0x5000 0000 | AHB3 | 64 KB | FMC, QSPI, OCTOSPI |
| 0x5800 0000 | APB3 | 64 KB | TZSC, TZIC, GTZC |

## 2. Clock Initialization

The STM32U5 uses the following clock tree:

```
HSI16 (16 MHz) ──▶ PLL ──▶ SYSCLK = 160 MHz
                    │         ├── AHB = 160 MHz
                    │         ├── APB1 = 80 MHz
                    │         └── APB2 = 80 MHz
                    │
LSE (32.768 kHz) ──▶ RTC, IWDG, LPUART
                    │
MSI (4 MHz) ───────▶ ADC (synchronous)
```

PLL configuration:
- PLLM = 2 (divide HSI16 by 2 → 8 MHz)
- PLLN = 80 (multiply by 80 → 640 MHz)
- PLLP = 4 (divide by 4 → 160 MHz SYSCLK)
- PLLQ = 8 (divide by 8 → 80 MHz for peripherals)

## 3. GPIO Initialization

See `firmware/board.h` for pin definitions and `firmware/drivers/gpio_init.c` for initialization code.

Key GPIO configuration:
- All unused pins: Analog mode (lowest power)
- SPI pins: Alternate function, push-pull, 50 MHz speed
- I2C pins: Alternate function, open-drain, pull-up enabled
- Interrupt pins: Input, pull-up, EXTI enabled
- LED pins: Output, push-pull, 2 MHz speed

## 4. Critical Device Drivers

### 4.1 SX1262 LoRa Driver

The SX1262 driver (`firmware/drivers/sx1262.c`) implements:
- SPI command interface (register read/write, buffer access)
- TX/RX mode switching with DIO1 interrupt
- CAD (Channel Activity Detection) for listen-before-talk
- Configurable spreading factor (SF7–SF12), bandwidth, coding rate
- Continuous RX mode with timeout
- TCXO control for ±0.5 ppm frequency stability

### 4.2 SCL3300 Inclinometer Driver

The SCL3300 driver (`firmware/drivers/scl3300.c`) implements:
- SPI read/write with auto-increment
- Self-test on startup
- Filter configuration (10 Hz low-pass)
- Offset calibration (factory + field zeroing)
- Data-ready interrupt for synchronized reads

### 4.3 ADS1120 ADC Driver

The ADS1120 driver (`firmware/drivers/ads1120.c`) implements:
- I²C communication (100 kHz)
- PGA gain configuration (1× to 128×)
- Continuous conversion mode
- Data-ready interrupt handling
- Internal temperature sensor read for cold-junction compensation

### 4.4 MAX20361 PMIC Driver

The PMIC driver (`firmware/drivers/max20361.c`) implements:
- I²C register access
- Boost converter voltage set (3.3 V output)
- Battery voltage monitoring (12-bit ADC)
- Battery current monitoring (sense resistor)
- Low-battery threshold interrupt
- Power rail sequencing

## 5. Device Tree (Conceptual)

For Zephyr RTOS port (future):

```dts
/ {
    model = "Terramesh Geotechnical Node";
    compatible = "jayis1,terramesh";

    chosen {
        zephyr,console = &usart2;
        zephyr,flash = &mx25r64;
    };

    lora0: &sx1262 {
        compatible = "semtech,sx1262";
        reg = <0>;
        spi-max-frequency = <10000000>;
        dio1-gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
        busy-gpios = <&gpioa 6 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpioa 7 GPIO_ACTIVE_LOW>;
        tcxo-power = <0x08>;
    };

    inclinometer0: &scl3300 {
        compatible = "murata,scl3300";
        reg = <0>;
        spi-max-frequency = <8000000>;
        drdy-gpios = <&gpioc 7 GPIO_ACTIVE_HIGH>;
    };

    adc0: &ads1120@48 {
        compatible = "ti,ads1120";
        reg = <0x48>;
        i2c-max-frequency = <100000>;
        drdy-gpios = <&gpioc 8 GPIO_ACTIVE_LOW>;
    };
};
```

## 6. Bootloader

The bootloader resides in the first 32 KB of flash (sector 0). It provides:
- DFU over USB CDC-ACM (YMODEM protocol)
- Application integrity check (CRC32 over flash sectors 1–63)
- Rollback to previous firmware version
- Secure boot with TrustZone (SHA-256 signature verification)

Boot sequence:
1. Reset vector → bootloader entry
2. Check DFU request pin (PA3 held low during reset)
3. If DFU: enter YMODEM receive mode
4. If no DFU: verify CRC32 of application
5. If CRC OK: jump to application vector table (0x0800 8000)
6. If CRC fail: enter DFU mode automatically

## 7. Power Management Strategy

| Mode | State | Current | Entry Condition |
|------|-------|---------|-----------------|
| SLEEP | STOP2, 1.8 V retention | 2.5 µA | After measurement cycle complete |
| SENSE | Active, 160 MHz | 15 mA | RTC alarm or tilt interrupt |
| TX_LORA | Active + PA | 120 mA | Mesh transmission |
| TX_LTE | Active + LTE | 250 mA | Gateway uplink (6-hourly) |
| DFU | Active, USB | 50 mA | Firmware update |

Transition rules:
- SLEEP → SENSE: RTC alarm (periodic) or ADXL372 INT1 (tilt event)
- SENSE → TX_LORA: After classification, if data to send
- SENSE → SLEEP: If no anomaly and no pending TX
- TX_LORA → SLEEP: After TX complete + RX window
- Any → DFU: Reset with DFU pin held

## 8. Mesh Routing Algorithm

The mesh uses a simplified AODV (Ad-hoc On-Demand Distance Vector) routing protocol:

1. **Route discovery**: When a node has data for the gateway but no route, it broadcasts a RREQ (Route Request) with its address and sequence number.
2. **Route reply**: The gateway (or any node with a route to the gateway) responds with RREP (Route Reply), establishing a reverse path.
3. **Route maintenance**: Nodes periodically send HELLO beacons (every 30 min). If 3 consecutive HELLOs are missed, the route is invalidated and a new RREQ is sent.
4. **Data forwarding**: Each node maintains a routing table with next-hop addresses. Data packets are forwarded hop-by-hop with TTL decrement.
5. **Duplicate suppression**: Sequence numbers prevent duplicate forwarding.

## 9. Companion App Protocol

The mobile app communicates with the gateway via REST API (cloud) or BLE (local, future). The wire protocol uses JSON over HTTPS:

```json
{
    "node_id": "TM-0042",
    "timestamp": 1752806400,
    "sensors": {
        "pore_pressure_shallow": 142.3,
        "pore_pressure_deep": 87.6,
        "moisture": 42.1,
        "tilt_x": 0.023,
        "tilt_y": -0.015,
        "accel_x": 12,
        "accel_y": -8,
        "accel_z": 1002,
        "temperature": 18.5,
        "pressure": 101325
    },
    "classification": 0,
    "battery_mv": 7200,
    "rssi": -95,
    "snr": 8.2
}
```
