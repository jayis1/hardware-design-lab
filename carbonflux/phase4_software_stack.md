# Phase 4: Software Stack — CarbonFlux

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**

---

## 1. Firmware Architecture

The CarbonFlux firmware is a **super-loop with a 1 kHz SysTick scheduler**. No RTOS is used — the application is simple enough (single measurement cycle per RTC waking) that the overhead of FreeRTOS is unnecessary and adds flash/RAM pressure to the ~60 kB firmware image.

### 1.1 Memory Map (STM32U5A9ZG)

| Region | Start | Size | Contents |
|--------|-------|------|----------|
| FLASH_APP | 0x08000000 | 256 kB | Main firmware (max) |
| FLASH_CONFIG | 0x08040000 | 8 kB | Calibration coefficients + LoRaWAN keys (encrypted) |
| FLASH_SAFE | 0x08042000 | 8 kB | Secure backup config (TrustZone-protected) |
| RAM_TEXT | 0x20000000 | 128 kB | .data, .bss, heap |
| RAM_DATA | 0x20020000 | 256 kB | CO₂ accumulation buffer (max 150 samples × 4 bytes = 600 B) |
| RAM_FLASH | 0x20060000 | 64 kB | QSPI memory-mapped flash cache |
| BACKUP_SRAM | 0x40016000 | 4 kB | RTC backup registers (retained in Stop 2) |
| SRAM_SAFE | 0x40006C00 | 4 kB | Secure SRAM (TrustZone) for keys |

### 1.2 Startup Sequence

```
Reset vector
    │
    ├── SystemInit() — STM32 HAL startup
    │   ├── Configure HSI16 → PLL → 160 MHz system clock
    │   ├── Configure HSE 16 MHz → PLL → 48 MHz for USB
    │   └── Configure LSE 32.768 kHz → RTC
    │
    ├── main()
    │   ├── board_init()
    │   │   ├── GPIO init (all outputs to safe state)
    │   │   ├── I2C1 init (400 kHz, 3.3V)
    │   │   ├── SPI1 init (8 MHz, master, Mode 0)
    │   │   ├── SPI2/QSPI init (40 MHz, memory-mapped mode)
    │   │   ├── USART2 init (115200 baud, USB CDC)
    │   │   ├── ADC1 init (12-bit, 1 µs sample time, continuous scan)
    │   │   ├── TIM2 init (50 Hz PWM for servo)
    │   │   ├── DS18B20 1-Wire init
    │   │   └── SysTick init (1 kHz)
    │   │
    │   ├── sensor_init()
    │   │   ├── rv8803_init() — set RTC, check battery
    │   │   ├── dps310_init() — set precision mode
    │   │   ├── scd41_init() — start periodic measurement
    │   │   ├── tmp117_init() — set continuous conversion
    │   │   └── sx1262_init() — configure LoRaWAN 1.0.4
    │   │
    │   ├── flash_init()
    │   │   ├── w25q128_init() — read device ID
    │   │   └── flash_log_init() — locate ring buffer head/tail
    │   │
    │   ├── config_load()
    │   │   ├── Read calibration coefficients from FLASH_CONFIG
    │   │   ├── Read LoRaWAN keys from FLASH_CONFIG (decrypt with device key)
    │   │   ├── Read measurement schedule from FLASH_CONFIG
    │   │   └── Validate CRC32 of entire config block
    │   │
    │   └── start_scheduler()
    │       └── Main state machine loop (see below)
    │
    └── fault handlers
        ├── HardFault_Handler() — log to flash, blink red LED, wait reset
        ├── MemManage_Handler() — TrustZone violation
        └── SecureFault_Handler() — secure boundary violation
```

### 1.3 Main State Machine (implemented in main.c)

```c
typedef enum {
    STATE_BOOT = 0,
    STATE_EQUILIBRATE,
    STATE_CLOSE_LID,
    STATE_ACCUMULATE,
    STATE_COMPUTE_FLUX,
    STATE_LOG_DATA,
    STATE_TX_LORA,
    STATE_OPEN_LID,
    STATE_SLEEP,
    STATE_CALIBRATE_ZERO,
    STATE_CALIBRATE_SPAN,
    STATE_ERROR
} flux_state_t;
```

Transitions driven by:
- **RTC alarm** — wakes from Stop 2, transitions to EQUILIBRATE
- **Timer completion** — ACCUMULATE timer (120–300 s) hits → COMPUTE_FLUX
- **Sensor error** — any sensor NACK → ERROR state
- **LoRaWAN join failure** — retry 3 times, then skip TX and log locally
- **Manual command** — USB CDC or BLE command forces state transition

## 2. MMIO Register Definitions (registers.h)

All peripheral registers are defined as volatile structs with bitfield unions for safe access:

```c
typedef struct {
    volatile uint32_t CR;      // Control register
    volatile uint32_t CFGR;    // Configuration register
    volatile uint32_t ISR;     // Interrupt status register
    volatile uint32_t IER;     // Interrupt enable register
    // ... more registers
} I2C_TypeDef;

#define I2C1_BASE       (0x40005400UL)
#define I2C1            ((I2C_TypeDef *)I2C1_BASE)
#define I2C1_CR_PE_Pos  (0U)
#define I2C1_CR_PE_Msk  (0x1UL << I2C1_CR_PE_Pos)
```

## 3. Clock Tree

```
HSI16 (16 MHz) ─┬─→ PLL ──→ SYSCLK 160 MHz ──→ AHB 160 MHz
                 │                               └→ APB1 160 MHz
                 │                               └→ APB2 160 MHz
                 │
HSE (16 MHz) ────┘─→ PLL ──→ 48 MHz USB
                 │
LSE (32.768 kHz) ─┘─→ RTC ──→ 1 Hz calendar
                    └→ IWDG ──→ 25.6 s watchdog
```

## 4. USB CDC (Virtual COM Port)

- **Interface**: USB 2.0 Full Speed (12 Mbps), no external PHY needed
- **USB class**: Communications Device Class (CDC) — virtual COM port
- **Baud rate**: 115200, 8N1
- **Endpoints**: EP1 IN (64 bytes, interrupt for TX), EP1 OUT (64 bytes, interrupt for RX)
- **Commands**: ASCII-based protocol schema (same as BLE for consistency)

### USB Protocol

```
> STATUS           ← Query device state
< OK 0:1402:21.5:45.2   ← state:CO2_ppm:temp_C:flux_umol
> CONFIG GET       ← Read config parameter
< OK interval=1800
> CONFIG SET interval=900  ← Write config
< OK
> CAL ZERO         ← Start zero calibration
< OK calibrating_zero
> CAL SPAN 5000    ← Start span calibration at 5000 ppm
< OK calibrating_span
> LOG DUMP         ← Dump all log entries (returns base64)
> LOG CLEAR        ← Erase log
< OK
> RESET            ← Soft reset device
< OK
```

## 5. LoRaWAN Stack

- **Stack**: Custom lightweight implementation (not LoRaMAC-node — too large for 256 kB)
- **Region**: EU868 (default) or US915 (configurable)
- **Class**: A (Aloha-style uplink with two short RX windows)
- **Join**: OTAA (Over-the-Air Activation)
- **Data rate**: DR5 (SF7, 125 kHz BW, 5 kbps) — default, ADR adapts
- **Confirmed uplink**: 3 retries, ACK required
- **FPort**:
  - 1: Flux measurement (32 bytes, encrypted)
  - 2: Diagnostics (4 bytes, unconfirmed)

### LoRaWAN Keys (stored in secure flash)

- **DevEUI**: Derived from STM32U5 UID64 (unique)
- **AppEUI**: Provisioned during manufacturing
- **AppKey**: AES-128, provisioned during manufacturing, encrypted in flash

## 6. Critical Device Driver: Flux Engine

The flux engine runs a **linear least-squares regression** on CO₂ concentration vs. time data:

```c
typedef struct {
    uint32_t sample_count;    // Number of valid CO₂ samples
    float sum_x;              // Sum of time (seconds from closure)
    float sum_y;              // Sum of CO₂ (ppm)
    float sum_xy;             // Sum of time × CO₂
    float sum_xx;             // Sum of time²
    float intercept;          // CO₂ at t=0 (ppm)
    float slope;              // dC/dt (ppm/s)
    float r_squared;          // Goodness of fit
    float flux_umol;          // Computed flux (µmol·m⁻²·s⁻¹)
} flux_result_t;
```

The regression uses single-precision float (STM32U5 has FPU). Each new sample updates the running sums in O(1). After accumulation timer expires, the result is computed in a single batch step:

```c
void flux_compute(flux_result_t *res) {
    float n = (float)res->sample_count;
    float denom = n * res->sum_xx - res->sum_x * res->sum_x;
    if (fabsf(denom) < 1e-12f) {
        res->slope = 0.0f;
        res->r_squared = 0.0f;
        return;
    }
    res->slope = (n * res->sum_xy - res->sum_x * res->sum_y) / denom;
    res->intercept = (res->sum_y - res->slope * res->sum_x) / n;

    // R² computation
    float ss_reg = 0.0f, ss_tot = 0.0f, mean_y = res->sum_y / n;
    // (stored samples are iterated for R² — small set, <150 points)
    res->r_squared = (ss_tot > 0.0f) ? ss_reg / ss_tot : 0.0f;

    // Flux = (dC/dt × Vchamber × P) / (A × R × T)
    float V = CHAMBER_VOLUME_M3;  // 0.0045 m³
    float A = CHAMBER_AREA_M2;    // 0.0314 m² (π × 0.1²)
    float P = dps310_get_pressure_kpa();
    float T = tmp117_get_temp_k(); // air temperature in Kelvin
    float R = 8.314462f;          // L·kPa·mol⁻¹·K⁻¹

    // dC/dt (ppm/s) * P(kPa) * V(L) / (A(m²) * R * T(K)) → µmol·m⁻²·s⁻¹
    // ppm = µmol/mol, so (µmol/mol)/s * kPa * m³ / (m² * L·kPa/(mol·K) * K)
    // 1 m³ = 1000 L, so V in m³ * 1000 = V in L
    float V_L = V * 1000.0f;
    res->flux_umol = (res->slope * P * V_L) / (A * R * T);
}
```

## 7. Device Tree Bindings (Linux)

When connected via USB CDC, the device presents as a simple character device. A `udev` rule is provided:

```
SUBSYSTEM=="tty", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740", SYMLINK+="carbonflux"
```

## 8. Bootloader

A minimal USB DFU bootloader resides at the start of flash (0x08000000, 16 kB). It:
1. Checks BOOT0 pin — if HIGH, enters DFU mode
2. If LOW and valid application vector (stack pointer valid), jumps to 0x08008000
3. DFU mode: USB DFU 1.1, accepts .bin files up to 256 kB
4. Validates CRC32 before programming
5. Blinks green LED during programming

## 9. Build System (Makefile)

- **Toolchain**: arm-none-eabi-gcc 12.x
- **MCU**: STM32U5A9ZG
- **Optimization**: -Os (size optimization, critical for 256 kB)
- **Linker script**: Custom, with secure/non-secure region partitioning
- **Targets**: `all`, `clean`, `flash` (via st-flash), `dfu` (via dfu-util)