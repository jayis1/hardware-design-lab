# Chronos-RTK — Phase 4: Software Stack

## 1. Boot Strategy

The STM32G474 boots from internal Flash (MAIN flash bank at 0x08000000). The boot sequence is:

```
Reset Vector (0x08000000)
  ├─ SystemInit() — RCC clock config, FPU enable, VTOR relocation
  ├─ spl_main() — early board init
  │    ├─ Power: enable 3.3 V rail (GPIO PB3 = HIGH)
  │    ├─ Clocks: HSE 32 MHz → PLL → 170 MHz SYSCLK
  │    ├─ GPIO: all pin muxes configured
  │    ├─ DMA: channels allocated for UART1, SPI1, SPI2
  │    ├− UART1: 460800 baud, DMA RX circular buffer
  │    ├─ I2C1: 400 kHz, probe SSD1306 + BME280
  │    ├─ SPI1: 50 MHz, probe W25Q128
  │    ├─ SPI2: 16 MHz, init SX1262
  │    └─ USB: CDC-ACM device class init
  ├─ gnss_init() — configure ZED-F9P (UBX-CFG messages)
  │    ├─ Enable dual-frequency L1/L2
  │    ├─ Set baud rate 460800
  │    ├─ Enable RTCM output on UART1
  │    └─ Set fix rate to 20 Hz
  ├─ lora_init() — configure SX1262
  │    ├─ Set frequency 868.0 MHz (EU) / 915.0 MHz (US)
  │    ├─ Set LoRa SF7, BW125, CR4/5
  │    ├─ Set TX power +22 dBm
  │    └─ Set sync word 0x12 (private network)
  ├─ oled_init() — configure SSD1306
  │    ├─ Display splash screen "Chronos-RTK v1.0"
  │    └─ Show "Acquiring..." on line 2
  └─ main_loop()
       ├─ Process UART1 DMA buffer (UBX + NMEA parser)
       ├─ Process LoRa incoming RTCM
       ├─ Forward RTCM to ZED-F9P (inject corrections)
       ├─ Format NMEA output to USB CDC-ACM
       ├─ Log raw obs to SPI flash
       ├─ Update OLED (10 Hz)
       └─ Handle button presses
```

## 2. Clock Configuration

### 2.1 Clock Tree

```
HSE = 32.000 MHz (Y2, external crystal)
  └─ PLL
       ├─ PLLM = /4  → 8 MHz
       ├─ PLLN = ×85 → 680 MHz VCO
       ├─ PLLP = /4  → 170 MHz SYSCLK  ✓
       ├─ PLLQ = /8  → 85  MHz USB (48 MHz → /2 × 85/85 not exact)
       │   └─ Use HSI48 for USB instead
       └─ PLLR = /2  → 340 MHz (not used)

SYSCLK = 170 MHz
  ├─ AHB  = /1 = 170 MHz (HCLK)
  ├─ APB1 = /1 = 170 MHz (SPI2, I2C1, USART1, TIM2-7)
  ├─ APB2 = /1 = 170 MHz (SPI1, ADC12, TIM1/8/15-17)
  └─ USB  = 48 MHz from HSI48 with CRS

LSE = 32.768 kHz (Y1, for RTC)
```

### 2.2 Clock Config Code

```c
// In SystemInit() / spl_main()
void clock_init(void) {
    // 1. Enable HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Configure PLL: HSE/4 * 85 / 4 = 170 MHz
    RCC->PLLCFGR = (4 << RCC_PLLCFGR_PLLM_Pos)   // PLLM = 4
                  | (85 << RCC_PLLCFGR_PLLN_Pos)   // PLLN = 85
                  | (0 << RCC_PLLCFGR_PLLP_Pos)     // PLLP = 2 (000=2)
                  | (RCC_PLLCFGR_PLLQEN)
                  | (RCC_PLLCFGR_PLLREN)
                  | (RCC_PLLCFGR_PLLSRC_HSE);

    // 3. Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 4. Configure flash latency (5 WS for 170 MHz @ VDD 3.3V)
    FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN
               | (5 << FLASH_ACR_LATENCY_Pos);

    // 5. Switch SYSCLK to PLL
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // 6. Enable HSI48 for USB
    RCC->CRRC |= RCC_CRRC_HSI48ON;
    while (!(RCC->CRRC & RCC_CRRC_HSI48RDY));

    // 7. LSE for RTC
    RCC->BDCR |= RCC_BDCR_LSEON;
    while (!(RCC->BDCR & RCC_BDCR_LSERDY));
}
```

## 3. MMIO Register Definitions

See `registers.h` in the firmware directory — full STM32G474 register map with base addresses and bit masks for RCC, GPIO, USART, SPI, I2C, DMA, USB, TIM, ADC, and CRC peripherals.

## 4. GPIO Initialization Code

```c
void gpio_init(void) {
    // Enable all GPIO port clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
                  | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN;

    // --- UART1 (PA2=TX, PA3=RX) ---
    // PA2: AF7 (USART1_TX), AF push-pull, no pull, high speed
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*2)))  | (2u << (2*2));  // AF
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*2))) | (7u << (4*2));  // AF7
    GPIOA->OSPEEDR |= (3u << (2*2));  // Very high speed
    // PA3: AF7 (USART1_RX), AF input, pull-up
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*3)))  | (2u << (2*3));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*3))) | (7u << (4*3));
    GPIOA->PUPDR   = (GPIOA->PUPDR   & ~(3u << (2*3)))  | (1u << (2*3));  // Pull-up

    // --- SPI1 (PA4=NSS, PA5=SCK, PA6=MISO, PA7=MOSI) ---
    // PA4: GPIO output (software NSS)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*4)))  | (1u << (2*4));
    GPIOA->OSPEEDR |= (3u << (2*4));
    // PA5: AF5 (SPI1_SCK)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*5)))  | (2u << (2*5));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*5))) | (5u << (4*5));
    GPIOA->OSPEEDR |= (3u << (2*5));
    // PA6: AF5 (SPI1_MISO)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*6)))  | (2u << (2*6));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*6))) | (5u << (4*6));
    // PA7: AF5 (SPI1_MOSI)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*7)))  | (2u << (2*7));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*7))) | (5u << (4*7));
    GPIOA->OSPEEDR |= (3u << (2*7));

    // --- SPI2 (PA0=SCK, PA1=NSS, PC3=MOSI, PB10/MISO not used - single wire) ---
    // PA0: AF5 (SPI2_SCK)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*0)))  | (2u << (2*0));
    GPIOA->AFR[0]  = (GPIOA->AFR[0]  & ~(0xF << (4*0))) | (5u << (4*0));
    GPIOA->OSPEEDR |= (3u << (2*0));
    // PA1: GPIO output (software NSS for SX1262)
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*1)))  | (1u << (2*1));
    GPIOA->OSPEEDR |= (3u << (2*1));
    // PC3: AF5 (SPI2_MOSI)
    GPIOC->MODER   = (GPIOC->MODER   & ~(3u << (2*3)))  | (2u << (2*3));
    GPIOC->AFR[0]  = (GPIOC->AFR[0]  & ~(0xF << (4*3))) | (5u << (4*3));

    // --- I2C1 (PB10=SCL, PB11=SDA) ---
    GPIOB->MODER   = (GPIOB->MODER   & ~(3u << (2*10))) | (2u << (2*10));  // AF
    GPIOB->MODER   = (GPIOB->MODER   & ~(3u << (2*11))) | (2u << (2*11));
    GPIOB->AFR[1]  = (GPIOB->AFR[1]  & ~(0xF << (4*(10-8)))) | (4u << (4*(10-8))); // AF4
    GPIOB->AFR[1]  = (GPIOB->AFR[1]  & ~(0xF << (4*(11-8)))) | (4u << (4*(11-8)));
    GPIOB->OSPEEDR |= (3u << (2*10)) | (3u << (2*11));  // High speed
    GPIOB->OTYPER  |= (1u << 10) | (1u << 11);  // Open-drain
    GPIOB->PUPDR   = (GPIOB->PUPDR & ~(3u << (2*10))) | (1u << (2*10));  // Pull-up
    GPIOB->PUPDR   = (GPIOB->PUPDR & ~(3u << (2*11))) | (1u << (2*11));

    // --- USB (PA11=DM, PA12=DP) ---
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*11))) | (2u << (2*11));  // AF10
    GPIOA->MODER   = (GPIOA->MODER   & ~(3u << (2*12))) | (2u << (2*12));
    GPIOA->AFR[1]  = (GPIOA->AFR[1]  & ~(0xF << (4*(11-8)))) | (0xAu << (4*(11-8)));
    GPIOA->AFR[1]  = (GPIOA->AFR[1]  & ~(0xF << (4*(12-8)))) | (0xAu << (4*(12-8)));

    // --- GPIO outputs ---
    // PB3: POWER_EN (active high)
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*3))) | (1u << (2*3));
    GPIOB->ODR   |= (1u << 3);  // Enable 3.3 V rail
    // PB7: OLED_RST (active low reset)
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*7))) | (1u << (2*7));
    GPIOB->ODR   |= (1u << 7);  // De-assert reset
    // PB12: LED_R, PB13: LED_G, PB14: LED_B
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*12))) | (1u << (2*12));
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*13))) | (1u << (2*13));
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*14))) | (1u << (2*14));

    // --- GPIO inputs ---
    // PC14: BTN_MODE, PC15: BTN_SEL, PC2: BTN_ACT (active low, external pull-ups)
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (2*14))) | (0u << (2*14));  // Input
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (2*15))) | (0u << (2*15));
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (2*2)))  | (0u << (2*2));
    // PB15: GNSS_PPS input
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*15))) | (0u << (2*15));

    // --- SX1262 control pins ---
    // PC4: LORA_BUSY (input), PC5: LORA_RST (output)
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (2*4))) | (0u << (2*4));
    GPIOC->MODER = (GPIOC->MODER & ~(3u << (2*5))) | (1u << (2*5));
    GPIOC->ODR   |= (1u << 5);  // De-assert reset
    // PB2: GNSS_RST (output)
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*2))) | (1u << (2*2));
    GPIOB->ODR   |= (1u << 2);  // De-assert reset
    // PB0: LORA_DIO1 (input, EXTI interrupt)
    GPIOB->MODER = (GPIOB->MODER & ~(3u << (2*0))) | (0u << (2*0));
    // EXTI configuration for PB0
    EXTI->IMR1  |= (1u << 0);   // Unmask EXTI line 0
    EXTI->RTSR1 |= (1u << 0);   // Rising edge trigger
    EXTI->FTSR1 |= (1u << 0);   // Falling edge trigger
    // Route EXTI0 to PB (port B = 1)
    SYSCFG->EXTICR[0] = (SYSCFG->EXTICR[0] & ~(0xFu << (4*0))) | (1u << (4*0));
}
```

## 5. Device Driver: SX1262 LoRa Transceiver

Full driver in `firmware/drivers/sx1262.h` and `firmware/drivers/sx1262.c`.

Key features:
- SPI command interface (read/write registers, write buffer, read buffer)
- Configuration: frequency, spreading factor, bandwidth, coding rate, TX power
- TX/RX with interrupt-driven flow (DIO1 → EXTI0 ISR)
- CRC-16 on LoRa payload frames
- RTCM v3.3 message encapsulation with sequence numbers
- Mesh relay mode: receive RTCM from ZED-F9P → pack → TX over LoRa → receive → forward to ZED-F9P

### Register Map (SX1262)

| Address | Name | Default | Access | Description |
|---|---|---|---|---|
| 0x0300 | LoRaSyncWord | 0x1424 | R/W | LoRa sync word (0x12 = private) |
| 0x0301 | LoRaModParam | — | R/W | SF, BW, CR, LDRO |
| 0x0302 | LoRaPktParam | — | R/W | Preamble, Header, CRC, IQ |
| 0x0800 | FreqReg | — | R/W | 4-byte frequency register |
| 0x0802 | PaConfig | — | R/W | Output power, PA duty cycle |
| 0x0889 | PaDutyCycle | 0x04 | R/W | PA duty cycle config |
| 0x088D | TxClampConfig | 0x00 | R/W | TX clamp config |
| 0x088F | OcpConfig | 0x38 | R/W | Over-current protection |
| 0x0891 | TcxoMode | 0x01 | R/W | TCXO control voltage/time |
| 0x0893 | RfFreqError | — | R | Frequency error readout |
| 0x0899 | RxGain | 0x94 | R/W | LNA gain |
| 0x0818 | IrqMask | 0x00 | R/W | Interrupt mask |

### SPI Command Interface

| Opcode | Name | Description |
|---|---|---|
| 0x01 | SetStandby | Enter standby mode (0: RC, 1: XOSC) |
| 0x02 | SetFS | Enter frequency synthesis mode |
| 0x03 | SetTx | Enter TX mode (timeout in 3-byte param) |
| 0x04 | SetRx | Enter RX mode (timeout in 3-byte param) |
| 0x05 | SetRxDutyCycle | RX duty-cycle mode |
| 0x07 | SetLoRaPacketType | Set packet type to LoRa |
| 0x08 | SetModulationParams | SF, BW, CR, LDRO |
| 0x09 | SetPacketParams | Preamble, header mode, CRC, IQ |
| 0x0D | SetFrequency | 4-byte frequency (Hz/0.1192) |
| 0x0E | SetTxParams | Power + ramp time |
| 0x0F | WriteRegister | Write single register |
| 0x10 | ReadRegister | Read single register |
| 0x12 | WriteBuffer | Write TX buffer (offset + data) |
| 0x13 | ReadBuffer | Read RX buffer (offset + data) |
| 0x16 | GetStatus | Read chip status |
| 0x17 | GetRssiInst | Read instantaneous RSSI |
| 0x1C | GetPacketStatus | Read last packet status |
| 0x1D | ClearIrqStatus | Clear interrupt flags |
| 0x20 | SetDioIrqParams | Configure DIO1 interrupt mask |

## 6. Device Driver: W25Q128 SPI Flash

Full driver in `firmware/drivers/w25q128.h` and `firmware/drivers/w25q128.c`.

Key features:
- SPI Mode 0 (CPOL=0, CPHA=0), 50 MHz max
- DMA-based read/write for bulk transfers
- Sector (4 KB) and block (64 KB) erase
- Write-protect regions
- JEDEC ID verification
- Circular log buffer for raw observation storage

### W25Q128 Command Set

| Opcode | Name | Description |
|---|---|---|
| 0x06 | WriteEnable | Set write-enable latch |
| 0x04 | WriteDisable | Clear write-enable latch |
| 0x05 | ReadStatusReg1 | Read status register 1 |
| 0x35 | ReadStatusReg2 | Read status register 2 |
| 0x0F | ReadStatusReg3 | Read status register 3 |
| 0x01 | WriteStatusReg1 | Write status register 1 |
| 0x31 | WriteStatusReg2 | Write status register 2 |
| 0x11 | WriteStatusReg3 | Write status register 3 |
| 0x9F | ReadJEDECID | Read manufacturer + device ID |
| 0x03 | ReadData | Read data (up to 33 MHz) |
| 0x0B | FastRead | Read data (up to 50 MHz, 1 dummy byte) |
| 0x02 | PageProgram | Program 1–256 bytes within a 256-byte page |
| 0x20 | SectorErase | Erase 4 KB sector |
| 0xD8 | BlockErase | Erase 64 KB block |
| 0xC7 | ChipErase | Erase entire chip |
| 0xAB | ReleasePowerDown | Wake from power-down |
| 0xB9 | PowerDown | Enter power-down |
| 0x4B | ReadUID | Read 64-bit unique ID |

## 7. ZED-F9P GNSS Interface

The ZED-F9P is configured via its binary UBX protocol over UART1 at 460800 baud. Key UBX messages used:

| Class | ID | Name | Purpose |
|---|---|---|---|
| 0x06 | 0x8A | CFG-VALSET | Set configuration keys |
| 0x06 | 0x8B | CFG-VALGET | Get configuration keys |
| 0x06 | 0x13 | CFG-TP5 | Time pulse config |
| 0x01 | 0x07 | NAV-PVT | Position/velocity/time solution |
| 0x01 | 0x35 | NAV-SAT | Satellite info |
| 0x01 | 0x30 | NAV-STATUS | Receiver status |
| 0x02 | 0x15 | RXM-RAWX | Raw measurement data |
| 0x02 | 0x13 | RXM-SFRBX | Raw subframe data |
| 0x02 | 0x30 | RXM-RLM | SAR/LRB message |
| 0xF5 | 0x01 | RTCM3x | RTCM v3.3 output messages |
| 0xF5 | 0x02 | RTCM3x-IN | RTCM v3.3 input messages |

### Configuration Sequence

```c
// Configure ZED-F9P via UBX-CFG-VALSET
const uint8_t ubx_cfg_keys[] = {
    // Enable dual-frequency L1/L2
    CFG_SIGNAL_GPS_ENA, 1,
    CFG_SIGNAL_GPS_L1_ENA, 1,
    CFG_SIGNAL_GPS_L2_ENA, 1,
    CFG_SIGNAL_GLO_ENA, 1,
    CFG_SIGNAL_GAL_ENA, 1,
    CFG_SIGNAL_BDS_ENA, 1,
    // Set UART1 baud rate to 460800
    CFG_UART1_BAUDRATE, 460800,
    // Enable RTCM output on UART1
    CFG_UART1OUTPROT_RTCM3X, 1,
    // Enable NMEA output on USB
    CFG_USBOUTPROT_NMEA, 1,
    // Set measurement rate to 20 Hz
    CFG_RATE_MEAS, 50,  // 50 ms = 20 Hz
    // Set dynamic model to "survey"
    CFG_NAVSPG_DYNMODEL, 8,
};
```

## 8. USB CDC-ACM Device Class

The STM32G474 USB FS peripheral enumerates as a CDC-ACM virtual COM port:

- VID: 0x0483 (STMicroelectronics)
- PID: 0x5740 (custom CDC device)
- Configuration: 1 config, 2 interfaces (CDC data + CDC comm)
- Endpoints:
  - EP0: Control (64 bytes)
  - EP1: Bulk IN (64 bytes) — NMEA/UBX data to host
  - EP2: Bulk OUT (64 bytes) — RTCM/commands from host
  - EP3: Interrupt IN (8 bytes) — CDC line state notifications

USB descriptors defined in `firmware/usb_descriptors.h`.

## 9. Device Tree Overlay (for Linux companion)

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "st,stm32g474";
    fragment@0 {
        target = <&usart1>;
        __overlay__ {
            status = "okay";
            pinctrl-0 = <&usart1_pins>;
            current-speed = <460800>;
        };
    };
    fragment@1 {
        target = <&spi1>;
        __overlay__ {
            status = "okay";
            cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>;
            flash@0 {
                compatible = "winbond,w25q128";
                reg = <0>;
                spi-max-frequency = <50000000>;
            };
        };
    };
    fragment@2 {
        target = <&spi2>;
        __overlay__ {
            status = "okay";
            cs-gpios = <&gpioa 1 GPIO_ACTIVE_LOW>;
            lora@0 {
                compatible = "semtech,sx1262";
                reg = <0>;
                spi-max-frequency = <16000000>;
                interrupt-parent = <&exti>;
                interrupts = <0 IRQ_TYPE_EDGE_BOTH>;
                reset-gpios = <&gpioc 5 GPIO_ACTIVE_LOW>;
                busy-gpios = <&gpioc 4 GPIO_ACTIVE_HIGH>;
            };
        };
    };
    fragment@3 {
        target = <&i2c1>;
        __overlay__ {
            status = "okay";
            clock-frequency = <400000>;
            oled@3c {
                compatible = "solomon,ssd1306fb";
                reg = <0x3c>;
                solomon,width = <128>;
                solomon,height = <64>;
                reset-gpios = <&gpiob 7 GPIO_ACTIVE_LOW>;
            };
            bme280@76 {
                compatible = "bosch,bme280";
                reg = <0x76>;
            };
        };
    };
};
```

## 10. Build Instructions

### Firmware Build (arm-none-eabi-gcc)

```bash
# Prerequisites
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# Clone and build
cd firmware/
make clean
make -j$(nproc)

# Output: build/chronos-rtk.bin (binary), build/chronos-rtk.elf (debug symbols)
# Flash via OpenOCD:
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
    -c "program build/chronos-rtk.elf verify reset exit"
```

### Companion App Build (React Native)

```bash
cd app/
npm install
npx react-native run-android    # or run-ios
```

### Firmware Update via USB

The board enumerates as a CDC-ACM device. Firmware updates are performed via STM32 built-in DFU bootloader (BOOT0 pin held high at reset):

```bash
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/chronos-rtk.bin
```