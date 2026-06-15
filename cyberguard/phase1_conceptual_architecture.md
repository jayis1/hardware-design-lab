# CyberGuard — Phase 1: Conceptual Architecture

## 1. System Purpose

CyberGuard is a next-generation multi-protocol hardware security token designed for enterprise and personal authentication. It combines FIDO2/WebAuthn, biometric fingerprint verification, NFC contactless, BLE 5.3 wireless, and USB-C wired connectivity into a single tamper-resistant key form factor (keychain dongle, 48mm × 24mm × 8mm).

**Primary use cases:**
- Passwordless login (FIDO2/WebAuthn) via USB-C or NFC
- Biometric-gated transaction signing (fingerprint required for high-value operations)
- BLE proximity authentication for mobile/desktop unlock
- Secure credential storage (up to 50 resident keys) in tamper-resistant secure element
- One-time password generation (HOTP/TOTP) over USB HID

## 2. Performance Targets

| Parameter | Target |
|---|---|
| FIDO2 authentication latency | < 250 ms (USB-C), < 500 ms (NFC) |
| Fingerprint enrollment | < 3 s per finger, 98% FAR < 0.002% |
| Fingerprint match | < 200 ms |
| BLE pairing | < 2 s |
| NFC transaction time | < 300 ms |
| Resident key capacity | ≥ 50 keys |
| Battery life (active use) | ≥ 90 days (CR2032 coin cell) |
| Standby current | < 15 µA |
| USB-C data rate | 12 Mbps (Full-Speed) |
| Operating temperature | −20 °C to +60 °C |
| Drop survival | 1.5 m onto concrete |
| Tamper detection | Voltage glitch, EM glitch, temperature, die shield |

## 3. Constraints

- **Form factor**: Keychain dongle, 48 × 24 × 8 mm, PCB 44 × 20 mm
- **Power**: CR2032 coin cell (3V, 225 mAh) primary; USB-C bus power (5V) when plugged
- **Cost target**: BOM < $12 @ 10K units
- **Regulatory**: FCC Part 15 (BLE/NFC), CE, RoHS
- **Security**: FIPS 140-3 Level 3 (physical), Level 4 (logical) target; Common Criteria EAL5+
- **Software**: Must work without drivers on Windows 10+, macOS 13+, Linux (libfido2), Android 12+, iOS 16+

## 4. High-Level Block Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                         CyberGuard Top-Level                     │
│                                                                  │
│  ┌─────────┐   ┌──────────────┐   ┌────────────┐               │
│  │ USB-C   │──▶│ ESD +       │──▶│ STM32L562  │               │
│  │ Conn    │   │ EMI Filter  │   │ (App MCU)  │               │
│  └─────────┘   └──────────────┘   │  QSPI/SPI──┼──▶ Secure    │
│       │                           │  I2C───────┼──▶ Element   │
│       │                           │  UART──────┼──▶ Fingerprint│
│       │                           │  SPI───────┼──▶ NFC        │
│       │                           │            │               │
│       │                           └──────┬─────┘               │
│       │                                  │                      │
│  ┌────┴─────┐   ┌──────────────┐   ┌────▼──────┐              │
│  │ Power    │◀──│ DC-DC +      │◀──│ nPMIC     │              │
│  │ Mux      │   │ LDO Tree     │   │ (STPMIC1) │              │
│  └────┬─────┘   └──────────────┘   └───────────┘              │
│       │                                                       │
│  ┌────▼──────────────────────────────────────────┐            │
│  │              Power Rails                       │            │
│  │  3.3V (MCU/SE/NFC)  │  1.8V (MCU core)       │            │
│  │  1.1V (BLE)         │  VBAT (CR2032 backup)   │            │
│  └─────────────────────────────────────────────────┘            │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ A71CH        │  │ FPC1025     │  │ PN7150      │          │
│  │ Secure       │  │ Fingerprint │  │ NFC         │          │
│  │ Element      │  │ Sensor      │  │ Controller  │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │ nRF52833    │  │ CR2032      │                            │
│  │ BLE 5.3     │  │ Coin Cell   │                            │
│  │ (Coprocessor│  │ Holder      │                            │
│  └──────────────┘  └──────────────┘                            │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐                            │
│  │ LEDs (×3)   │  │ Touch Button │                            │
│  │ Status      │  │ User Confirm │                            │
│  └──────────────┘  └──────────────┘                            │
└──────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 FIDO2 Registration (USB-C)

```
Host → CTAP2 CMD → STM32L562 (USB HID endpoint)
  → Validate request
  → Request user presence (touch button / fingerprint)
  → Forward keygen request to A71CH (I2C)
  → A71CH generates keypair in secure slot
  → A71CH returns attestation certificate
  → STM32L562 assembles CTAP2 response
  → Response → Host (USB HID)
```

### 5.2 FIDO2 Authentication (NFC)

```
Reader RF field → PN7150 (ISO 14443)
  → PN7150 → STM32L562 (SPI interrupt)
  → STM32L562 processes APDU
  → Request biometric verification from FPC1025 (UART)
  → FPC1025 match result → STM32L562
  → STM32L562 → A71CH (I2C) sign challenge
  → Signature → PN7150 → Reader
```

### 5.3 BLE Proximity Unlock

```
Phone (BLE) → nRF52833 (BLE central connection)
  → nRF52833 → STM32L562 (UART)
  → STM32L562 validates proximity credential
  → Trigger action (unlock, auth)
```

## 6. Bus Topology

| Bus | Master | Slaves | Protocol | Speed |
|---|---|---|---|---|
| I2C Bus 1 | STM32L562 | A71CH Secure Element | I2C (address 0x48) | 400 kHz |
| SPI Bus 1 | STM32L562 | PN7150 NFC Controller | SPI (Mode 0) | 10 MHz |
| SPI Bus 2 | STM32L562 | External QSPI Flash (MX25R1635F) | QSPI | 80 MHz |
| UART1 | STM32L562 | FPC1025 Fingerprint Sensor | UART 115200 | 115.2 kbps |
| UART2 | STM32L562 | nRF52833 BLE Coprocessor | UART 921600 | 921.6 kbps |
| USB | STM32L562 | Host (via USB-C connector) | USB 2.0 FS | 12 Mbps |
| SWD | Debug | STM32L562, nRF52833 | SWD | 4 MHz (debug only) |
| I2C Bus 2 | STM32L562 | STPMIC1 Power Mgmt IC | I2C (address 0x08) | 400 kHz |

### Topology Diagram

```
                    ┌─────────────────┐
                    │  STM32L562QE    │
                    │  (App MCU)      │
                    └──┬─┬─┬─┬─┬─┬─┬─┘
                       │ │ │ │ │ │ │
          ┌────────────┘ │ │ │ │ │ └──────────────┐
          │ I2C1        │ │ │ │ │ UART2           │ USB
          ▼             │ │ │ │ ▼                  ▼
    ┌──────────┐        │ │ │ ┌────────────┐  ┌─────────┐
    │  A71CH   │        │ │ │ │ nRF52833  │  │ USB-C   │
    │  (SE)    │        │ │ │ │ (BLE)     │  │ Conn    │
    └──────────┘        │ │ │ └────────────┘  └─────────┘
                        │ │ │
          ┌─────────────┘ │ └────────────┐
          │ SPI1          │ UART1        │ SPI2/QSPI
          ▼               ▼              ▼
    ┌──────────┐   ┌────────────┐  ┌──────────────┐
    │  PN7150  │   │ FPC1025   │  │ MX25R1635F  │
    │  (NFC)   │   │ (FP)      │  │ (Flash)     │
    └──────────┘   └────────────┘  └──────────────┘

          I2C2
          ▼
    ┌──────────┐
    │ STPMIC1  │
    │ (PMIC)   │
    └──────────┘
```

## 7. Security Architecture

- **Root of Trust**: A71CH secure element (ECC P-256/P-384, Ed25519, AES-128/256, SHA-256/384, TRNG)
- **Secure Boot**: STM32L562 OTFDEnc + secure boot with RSA-3072 signature verification
- **Debug Lock**: SWD permanently disabled in production (RDP Level 2)
- **Tamper Detection**: Active die shield on A71CH, voltage/temperature monitors on STM32L562
- **Key Isolation**: Private keys never leave A71CH; all signing happens inside SE
- **Biometric Liveness**: FPC1025 with capacitive + RF liveness detection
- **Firmware Updates**: Signed (Ed25519) OTA via BLE or USB; A/B partition scheme on QSPI flash