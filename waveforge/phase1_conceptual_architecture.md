# WaveForge — Phase 1: Conceptual Architecture

## 1. System Purpose

WaveForge is a **polyphonic digital audio synthesizer engine board** designed for electronic musicians, sound designers, and embedded audio developers. It is a self-contained voice-card that can operate standalone (with MIDI/CV input) or as a USB-connected audio peripheral controlled by a companion React Native patch-editor app.

The device implements a fully digital synthesis pipeline: MIDI → voice allocation → per-voice oscillator mix (WAVETABLE + FM + NOISE) → multimode filter → ADSR envelope → effects bus (delay, reverb, chorus) → stereo DAC output at up to 96 kHz / 24-bit.

### Target Users

- Electronic musicians building custom Eurorack-style instruments
- Sound designers prototyping new timbres
- Embedded audio developers seeking a hackable synthesis platform
- Live performers needing a rugged, USB-powered voice module

## 2. Performance Targets

| Parameter | Target |
|---|---|
| Maximum polyphony | 64 voices (WAVETABLE), 32 voices (FM), 8 voices (WAVETABLE+FM layered) |
| Sample rate | 48 kHz (default), 96 kHz (high-quality) |
| Bit depth | 24-bit DAC output, 32-bit internal processing |
| Latency (MIDI-in to DAC-out) | ≤ 2 ms at 48 kHz |
| Frequency response | 20 Hz – 20 kHz (±0.5 dB) |
| Dynamic range | ≥ 110 dB DAC SNR |
| THD+N | ≤ 0.001% at 1 kHz |
| USB audio class | UAC 2.0 stereo I/O |
| MIDI | DIN-5 IN/THRU, USB MIDI class-compliant |
| CV inputs | 4 × 0–5 V analog CV, 10-bit ADC |
| Patch storage | 256 patches in SPI flash |
| Boot time | ≤ 500 ms from power-on to audio ready |
| Power | USB-C 5 V / 500 mA, or external 7–12 V DC barrel jack |
| Form factor | 100 mm × 80 mm, 4-layer PCB |

## 3. Constraints

- **Cost**: BOM under $45 at 1000-unit volume
- **Thermal**: Fanless, ≤ 85 °C junction on all ICs, natural convection only
- **EMC**: Must pass CE/FCC Class B (shielded enclosure assumed)
- **Software**: All synthesis runs bare-metal on ARM Cortex-M7 (no OS dependency for audio path)
- **Boot**: No SD card or display required — headless operation with MIDI/USB control
- **Manufacturability**: 0402 passives minimum, 0.5 mm pitch ICs, no BGA below 0.65 mm pitch

## 4. High-Level Block Diagram

```
                              ┌─────────────────────────────────────────────────┐
                              │                   WAVEFORGE                     │
                              │                                                 │
 ┌─────────┐    MIDI DIN    ┌─┴─────────────┐   SPI/I2S   ┌──────────────────┐  │
 │  MIDI   │───────────────►│              │◄────────────►│ WM8776 Codec     │  │
 │  IN     │               │              │   I2C        │ (ADC+DAC)        │  │
 └─────────┘               │              │◄────────────►│                  │  │
 ┌─────────┐    USB       │ STM32H743    │              └──────┬───────────┘  │
 │ USB-C   │◄────────────►│ (Cortex-M7) │                     │ Line Out      │
 │ (Data+P)│              │  480 MHz     │              ┌──────┴───────────┐  │
 └─────────┘              │              │              │  Headphone Amp   │  │
 ┌─────────┐    CV 0-5V   │              │              │  TPA6130A2       │  │
 │ CV1–CV4 │──────────────►│              │              └──────┬───────────┘  │
 │ (ADC)   │              │              │                     │ HP Out       │
 └─────────┘              │              │              ┌──────┴───────────┐  │
 ┌─────────┐    SPI/QSPI  │              │              │  Line In         │  │
 │ W25Q256 │◄────────────►│              │◄─────────────│  (Codec ADC)     │  │
 │ (Flash) │              │              │   I2S IN     └──────────────────┘  │
 └─────────┘              │              │                                     │
 ┌─────────┐    I2C        │              │                                     │
 │ EEPROM  │◄────────────►│              │                                     │
 │ (Config)│              └──────┬───────┘                                     │
 └─────────┘                     │                                             │
                          ┌──────┴───────┐                                     │
                          │ Power Tree   │                                     │
                          │ 5V→3.3V      │                                     │
                          │ 5V→1.8V      │                                     │
                          │ 5V→1.2V      │                                     │
                          └──────────────┘                                     │
                              │                                                 │
                              └─────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Audio Synthesis Pipeline (Real-Time Path)

```
MIDI Note-On/CC
       │
       ▼
  ┌──────────┐    ┌──────────────┐    ┌────────────┐    ┌──────────┐
  │  Voice   │───►│  Oscillator  │───►│  VCF       │───►│  VCA     │───► I2S TX
  │ Allocator│    │  (WT/FM/NZ) │    │  (SVF/2LP) │    │ (Env×1)  │
  └──────────┘    └──────────────┘    └────────────┘    └──────────┘
                          │                                     │
                          ▼                                     ▼
                   ┌──────────────┐                     ┌──────────┐
                   │  Wavetable   │                     │ Effects  │───► I2S TX
                   │  ROM (SPI)   │                     │ (FX Bus) │
                   └──────────────┘                     └──────────┘
```

### 5.2 Patch Management Flow

```
Companion App (BLE/USB)
       │
       ▼
  ┌──────────┐    ┌──────────────┐    ┌──────────────┐
  │ Protocol │───►│  Patch RAM   │───►│  SPI Flash   │
  │ Parser   │    │  (SRAM)      │    │  (W25Q256)   │
  └──────────┘    └──────────────┘    └──────────────┘
```

### 5.3 CV Input Flow

```
CV1–CV4 (0–5 V) → ADC (STM32 internal, 12-bit) → Parameter Modulation Matrix
```

## 6. Bus Topology

| Bus | Master | Slaves | Protocol | Speed | Purpose |
|---|---|---|---|---|---|
| I2C1 | STM32H743 | WM8776 (codec ctrl), 24C256 (EEPROM), TPA6130A2 (HP amp ctrl) | I2C | 400 kHz | Control plane for codec/amp config |
| SPI1 | STM32H743 | W25Q256 (wavetable/patch flash) | SPI/QSPI | 80 MHz | Wavetable samples, patch storage |
| I2S1 | STM32H743 | WM8776 (audio data) | I2S | 3.072 MHz (48k×64) | Audio DAC data (TX) |
| I2S2 | WM8776 (ADC) | STM32H743 | I2S | 3.072 MHz | Audio ADC data (RX) for line-in passthrough |
| USB_OTG_FS | STM32H743 | Host (PC/phone) | USB 2.0 FS | 12 Mbps | MIDI + Audio Class 2.0 + Vendor |
| UART1 | STM32H743 | MIDI DIN OUT (opto-isolated) | UART 31250 baud | 31.25 kbps | Legacy MIDI output |
| ADC1 | STM32H743 | CV1–CV4 analog inputs | SAR | 12-bit, 2.5 Msps | CV control voltage sampling |

### Memory Map Summary

| Region | Address Range | Size | Device |
|---|---|---|---|
| ITCM | 0x0000_0000 – 0x0000_FFFF | 64 KB | ISR vectors + critical code |
| DTCM | 0x2000_0000 – 0x2000_FFFF | 128 KB | Audio buffers (zero-wait) |
| AXI SRAM | 0x2400_0000 – 0x2408_0FFF | 512 KB | Voice allocator, wavetable cache |
| SRAM1 | 0x3000_0000 – 0x3000_3FFF | 16 KB | DMA buffers |
| SRAM4 | 0x3800_0000 – 0x3800_0FFF | 4 KB | USB packet buffers |
| QSPI-mapped | 0x9000_0000 – 0x93FF_FFFF | 32 MB | XIP wavetable ROM (memory-mapped) |
| Flash | 0x0800_0000 – 0x081F_FFFF | 2 MB | Firmware |

## 7. Power Architecture

```
                        ┌───────────┐
   USB-C 5V or Barrel   │           │
   7-12V → 5V LDO      │   5V_RAIL │
                        └─────┬─────┘
                              │
                    ┌─────────┼──────────┐
                    │         │          │
              ┌─────┴────┐ ┌──┴───┐ ┌───┴────┐
              │ TLV62569 │ │LD39050│ │LD39050 │
              │ 5V→1.2V  │ │5V→3.3V│ │5V→1.8V │
              │  3A buck │ │ 500mA│ │ 500mA  │
              └─────┬────┘ └──┬───┘ └───┬────┘
                    │         │         │
                 VDD_CORE  VDD_IO   VDD_CODEC
                  1.2V      3.3V     1.8V
```

## 8. Key Design Decisions

1. **STM32H743** chosen for 480 MHz Cortex-M7 with hardware FPU, double-precision, I/D-cache, and generous SRAM — sufficient for 64-voice polyphony with per-voice oscillator + filter + envelope entirely in software.
2. **WM8776** codec chosen for 24-bit, 96 kHz, 110 dB SNR, with built-in headphone driver bypass — external TPA6130A2 adds 128 dB SNR headphone drive.
3. **QSPI-mapped wavetable flash** (W25Q256, 32 MB) allows zero-copy XIP access to wavetables from the synthesis engine.
4. **I2S full-duplex** via two I2S peripherals gives simultaneous DAC output and ADC input for effects-loop and line-in monitoring.
5. **No display on-board** — all UI handled via MIDI CC, CV, or companion app. This reduces cost, power, and firmware complexity.
6. **USB-C with BC1.2** negotiation allows the board to draw up to 1.5 A from a USB-C charger, sufficient for 5 V operation even with headphone loads.