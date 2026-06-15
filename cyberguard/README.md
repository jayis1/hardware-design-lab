# CyberGuard — Multi-Protocol Hardware Security Token

A next-generation FIDO2/WebAuthn authenticator combining biometric fingerprint verification, NFC contactless, BLE 5.3 wireless, and USB-C wired connectivity in a keychain dongle form factor.

## Specifications

| Parameter | Value |
|---|---|
| **MCU** | STM32L562QE (Cortex-M33, 110 MHz, 512KB Flash, 256KB RAM, TrustZone) |
| **Secure Element** | NXP A71CH (ECC P-256/P-384, Ed25519, AES-256, TRNG, 100KB) |
| **NFC Controller** | NXP PN7150 (ISO 14443 A/B, 13.56 MHz) |
| **Fingerprint Sensor** | FPC1025A1 (Capacitive 160×160, Liveness Detection) |
| **BLE Coprocessor** | Nordic nRF52833 (BLE 5.3, Cortex-M4, 256KB/64KB) |
| **QSPI Flash** | Macronix MX25R1635F (16MB, Quad SPI) |
| **PMIC** | ST STPMIC1APQR (Buck + 2× LDO, I²C control) |
| **USB** | USB-C 2.0 Full-Speed (12 Mbps, HID + CDC) |
| **Power** | CR2032 coin cell (3V, 225mAh) + USB-C bus power |
| **Standby Current** | < 15 µA (STOP2 mode, coin cell only) |
| **Active Current** | ~70 mA peak (USB-C powered) |
| **Battery Life** | 90+ days (typical use) |
| **Form Factor** | 48 × 24 × 8 mm keychain dongle |
| **PCB** | 44 × 20 mm, 6-layer, 1.0 mm, ENIG finish |
| **Security** | FIPS 140-3 Level 3 (physical), TrustZone, Secure Boot, RDP Level 2 |
| **Protocols** | FIDO2/WebAuthn, CTAP2, CTAP1/U2F, TOTP/HOTP |
| **Fingerprint Slots** | 10 templates |
| **Resident Keys** | 50 (in A71CH secure element) |
| **Operating Temp** | −20 °C to +60 °C |

## Directory Structure

```
cyberguard/
├── README.md                              # This file
├── phase1_conceptual_architecture.md      # System purpose, blocks, data flow
├── phase2_component_selection_schematics.md # BOM, pinouts, netlists, decoupling
├── phase3_pcb_blueprints_layout.md         # Stackup, routing rules, thermal, DFM
├── phase4_software_stack.md               # Boot strategy, MMIO, drivers, DT
├── kicad/
│   ├── device.kicad_pro                   # KiCad project file
│   ├── device.kicad_sch                   # Full schematic
│   └── device.kicad_pcb                   # Board outline, placement, stackup
├── firmware/
│   ├── Makefile                           # Cross-compile for ARM Cortex-M33
│   ├── main.c                             # SPL/board init, main loop
│   ├── board.h                            # Pin definitions, macros
│   ├── registers.h                        # MMIO register map
│   ├── usb_descriptors.h                  # USB VID/PID, HID/CDC descriptors
│   └── drivers/
│       ├── i2c_se.h / i2c_se.c          # A71CH secure element (I²C driver)
│       ├── spi_nfc.h / spi_nfc.c         # PN7150 NFC controller (SPI driver)
│       ├── uart_fp.h / uart_fp.c         # FPC1025 fingerprint (UART driver)
│       ├── spi_flash.h / spi_flash.c     # MX25R1635F QSPI flash (SPI driver)
│       └── pmic.h / pmic.c               # STPMIC1 power management (I²C driver)
└── app/
    ├── package.json                       # React Native project config
    ├── App.js                             # Navigation and app entry
    ├── screens/
    │   ├── MainScreen.js                  # Connection dashboard, quick actions
    │   ├── SettingsScreen.js               # Security & connectivity settings
    │   └── CredentialScreen.js            # FIDO2 credential management
    ├── components/
    │   └── StatusCard.js                  # Reusable status card component
    └── utils/
        └── protocol.js                    # Binary wire protocol, CRC-16, frame builder/parser
```

## Quick Start

### Firmware Build

```bash
# Install ARM toolchain
sudo apt install gcc-arm-none-eabi

# Build firmware
cd firmware
make

# Flash via OpenOCD (CMSIS-DAP)
make flash

# Or flash via STM32CubeProgrammer
STM32_Programmer_CLI -c port=SWD -d cyberguard.elf
```

### Companion App

```bash
# Install dependencies
cd app
npm install

# Run on Android
npx react-native run-android

# Run on iOS
npx react-native run-ios
```

### KiCad

```bash
# Open project in KiCad 8+
kicad kicad/device.kicad_pro

# Generate Gerbers
kicad-cli pcb export gerbers kicad/device.kicad_pcb

# Generate BOM
kicad-cli sch export bom kicad/device.kicad_sch
```

## Security Architecture

1. **Root of Trust**: A71CH secure element stores all private keys; they never leave the chip
2. **Secure Boot**: STM32L562 RSA-3072 boot verification with A/B partition scheme
3. **TrustZone**: Secure/non-secure world partitioning on Cortex-M33
4. **Debug Lock**: RDP Level 2 permanently disables SWD in production
5. **Biometric Gating**: FPC1025 with capacitive+RF liveness detection gates high-value operations
6. **Tamper Resistance**: Voltage/temperature monitors, die shield on A71CH
7. **Firmware Updates**: Ed25519-signed OTA via BLE or USB; rollback protection

## License

MIT License — See [LICENSE](../LICENSE) for details.